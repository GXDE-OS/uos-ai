#include "dingtalkchannel.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QUuid>

Q_LOGGING_CATEGORY(logDT, "uos-ai.chatbot.dingtalk")

using namespace uos_ai::chatbot;

const char *DingTalkChannel::kApiBase            = "https://api.dingtalk.com";
const char *DingTalkChannel::kOpenConnectionPath = "/v1.0/gateway/connections/open";
const char *DingTalkChannel::kTokenPath          = "/v1.0/oauth2/accessToken";
const char *DingTalkChannel::kGroupMsgPath       = "/v1.0/robot/groupMessages/send";
const char *DingTalkChannel::kSingleMsgPath      = "/v1.0/robot/oToMessages/batchSend";
const char *DingTalkChannel::kCardInstancePath   = "/v1.0/card/instances";
const char *DingTalkChannel::kInteractiveCardPath= "/v1.0/im/interactiveCards/send";
const char *DingTalkChannel::kCardUpdateFmt      = "/v1.0/card/streaming";

DingTalkChannel::DingTalkChannel(QObject *parent)
    : AbstractChannel(parent)
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_http(new QNetworkAccessManager(this))
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_pingTimer->setInterval(kPingInterval);
    m_reconnectTimer->setSingleShot(true);

    connect(m_ws, &QWebSocket::connected,    this, &DingTalkChannel::onWsConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &DingTalkChannel::onWsDisconnected);
    connect(m_ws, &QWebSocket::textMessageReceived,
            this, &DingTalkChannel::onWsTextMessageReceived);
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &DingTalkChannel::onWsError);
    connect(m_pingTimer,     &QTimer::timeout, this, [this] { m_ws->ping(); });
    connect(m_reconnectTimer, &QTimer::timeout, this, [this] { openConnection(); });
}

// ============================================================
// 公共接口
// ============================================================

void DingTalkChannel::start(const QJsonObject &config)
{
    m_clientId       = config.value("client_id").toString();
    m_clientSecret   = config.value("client_secret").toString();
    m_cardTemplateId = config.value("card_template_id").toString();

    if (m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
        qCWarning(logDT) << "Missing client_id or client_secret";
        return;
    }

    m_running = true;
    openConnection();
}

void DingTalkChannel::stop()
{
    m_running = false;
    m_pingTimer->stop();
    m_reconnectTimer->stop();
    m_pendingTokenCallbacks.clear();

    if (m_ws->state() != QAbstractSocket::UnconnectedState)
        m_ws->close();
}

// ============================================================
// 连接流程
// ============================================================

void DingTalkChannel::openConnection()
{
    qCInfo(logDT) << "Opening DingTalk Stream connection...";

    QNetworkRequest req(QUrl(QStringLiteral("%1%2").arg(kApiBase, kOpenConnectionPath)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 获取本机 IP（best-effort）
    QString localIp;
    const QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress &a : addrs) {
        if (a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback()) {
            localIp = a.toString();
            break;
        }
    }

    QJsonObject body;
    body["clientId"]     = m_clientId;
    body["clientSecret"] = m_clientSecret;
    body["subscriptions"] = QJsonArray{
        QJsonObject{{"type", "CALLBACK"}, {"topic", "/v1.0/im/bot/messages/get"}}
    };
    body["ua"]      = "uos-ai-chatbot/1.0";
    body["localIp"] = localIp;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logDT) << "openConnection failed:" << reply->errorString();
            scheduleReconnect();
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        QString endpoint = obj.value("endpoint").toString();
        QString ticket   = obj.value("ticket").toString();

        if (endpoint.isEmpty() || ticket.isEmpty()) {
            qCWarning(logDT) << "Invalid openConnection response:" << obj;
            scheduleReconnect();
            return;
        }

        QString wsUrl = QStringLiteral("%1?ticket=%2").arg(endpoint, ticket);
        qCInfo(logDT) << "Connecting DingTalk WebSocket...";
        m_ws->open(QUrl(wsUrl));
    });
}

void DingTalkChannel::scheduleReconnect()
{
    if (!m_running)
        return;
    qCInfo(logDT) << "Scheduling reconnect in" << kReconnectInterval << "ms";
    m_reconnectTimer->start(kReconnectInterval);
}

void DingTalkChannel::onWsConnected()
{
    qCInfo(logDT) << "DingTalk WebSocket connected";
    m_pingTimer->start();
}

void DingTalkChannel::onWsDisconnected()
{
    m_pingTimer->stop();
    qCInfo(logDT) << "DingTalk WebSocket disconnected";
    if (m_running)
        scheduleReconnect();
}

void DingTalkChannel::onWsError(QAbstractSocket::SocketError)
{
    qCWarning(logDT) << "DingTalk WebSocket error:" << m_ws->errorString();
}

// ============================================================
// WS 消息处理
// ============================================================

void DingTalkChannel::onWsTextMessageReceived(const QString &message)
{
    QJsonObject msg = QJsonDocument::fromJson(message.toUtf8()).object();
    QString type    = msg.value("type").toString();

    if (type == QStringLiteral("SYSTEM")) {
        QJsonObject headers = msg.value("headers").toObject();
        if (headers.value("topic").toString() == QStringLiteral("disconnect")) {
            qCInfo(logDT) << "Received disconnect from server";
            m_ws->close();
        }
        return;
    }

    if (type != QStringLiteral("CALLBACK"))
        return;

    QJsonObject headers = msg.value("headers").toObject();
    QString messageId   = headers.value("messageId").toString();
    QString topic       = headers.value("topic").toString();

    // 必须先发 ACK，否则钉钉会重试
    sendAck(messageId);

    if (topic != QStringLiteral("/v1.0/im/bot/messages/get"))
        return;

    // data 是 JSON 字符串，需二次解析
    QByteArray dataStr = msg.value("data").toString().toUtf8();
    QJsonObject d      = QJsonDocument::fromJson(dataStr).object();

    handleCallback(d);
}

void DingTalkChannel::handleCallback(const QJsonObject &d)
{
    QString msgtype = d.value("msgtype").toString();

    // 只处理文本消息
    QString textContent;
    if (msgtype == QStringLiteral("text")) {
        textContent = d.value("text").toObject().value("content").toString().trimmed();
    } else if (msgtype == QStringLiteral("richText")) {
        QJsonArray richText = d.value("content").toObject().value("richText").toArray();
        for (const QJsonValue &item : richText) {
            QJsonObject obj = item.toObject();
            if (obj.contains("text"))
                textContent += obj.value("text").toString();
        }
        textContent = textContent.trimmed();
    }

    if (textContent.isEmpty())
        return;

    QString convType = (d.value("conversationType").toString() == QStringLiteral("2"))
                       ? QStringLiteral("group") : QStringLiteral("user");

    QString conversationId = d.value("conversationId").toString();
    QString senderId       = d.value("senderStaffId").toString();
    if (senderId.isEmpty())
        senderId = d.value("senderId").toString();

    // 单聊时 conversationId 用 senderId，保持与 sendMessage 的 to 一致
    QString replyTo = (convType == QStringLiteral("user")) ? senderId : conversationId;

    // 存储 sessionWebhook（有效期约 5 分钟，字段单位是 Unix 毫秒）
    QString webhook = d.value("sessionWebhook").toString();
    qint64 expiry   = static_cast<qint64>(d.value("sessionWebhookExpiredTime").toDouble(0));
    if (!webhook.isEmpty() && expiry > 0)
        m_webhooks[replyTo] = { webhook, expiry };

    QString msgId = d.value("msgId").toString();

    QJsonObject payload;
    payload["platform"]   = platformName();
    payload["message_id"] = msgId;
    payload["sender"]     = QJsonObject{
        {"id",   senderId},
        {"name", d.value("senderNick").toString()},
        {"type", "user"}
    };
    payload["conversation"] = QJsonObject{{"id", replyTo}, {"type", convType}};
    payload["content"]      = QJsonObject{{"type", "text"}, {"text", textContent}};
    payload["timestamp"]    = QDateTime::currentSecsSinceEpoch();

    emit messageReceived(payload);
}

void DingTalkChannel::sendAck(const QString &messageId)
{
    QJsonObject headers;
    headers["messageId"]   = messageId;
    headers["contentType"] = "application/json";

    QJsonObject ack;
    ack["code"]    = 200;
    ack["headers"] = headers;
    ack["message"] = "OK";
    ack["data"]    = "{}";

    m_ws->sendTextMessage(QString::fromUtf8(
        QJsonDocument(ack).toJson(QJsonDocument::Compact)));
}

// ============================================================
// 发送消息
// ============================================================

void DingTalkChannel::sendMessage(const QString &to, const QString &content,
                                   const QString &conversationType)
{
    // 优先使用 sessionWebhook
    if (m_webhooks.contains(to)) {
        const auto &entry = m_webhooks[to];
        qint64 nowMs      = QDateTime::currentMSecsSinceEpoch();
        if (entry.expiredAt > nowMs) {
            doSendViaWebhook(entry.url, content);
            return;
        }
        m_webhooks.remove(to);
    }

    // Fallback：token API
    fetchToken([this, to, content, conversationType](const QString &token) {
        doSendViaToken(token, to, content, conversationType);
    });
}

void DingTalkChannel::fetchToken(std::function<void(const QString &)> callback)
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (m_tokenState == TokenState::Valid && now < m_tokenExpiry) {
        callback(m_accessToken);
        return;
    }

    m_pendingTokenCallbacks.append(std::move(callback));

    if (m_tokenState == TokenState::Refreshing)
        return;

    m_tokenState = TokenState::Refreshing;
    qCInfo(logDT) << "Fetching DingTalk access token...";

    QNetworkRequest req(QUrl(QStringLiteral("%1%2").arg(kApiBase, kTokenPath)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["appKey"]    = m_clientId;
    body["appSecret"] = m_clientSecret;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logDT) << "Token fetch failed:" << reply->errorString();
            m_tokenState = TokenState::Invalid;
            auto cbs = std::move(m_pendingTokenCallbacks);
            m_pendingTokenCallbacks.clear();
            for (auto &cb : cbs) cb(QString());
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_accessToken   = obj.value("accessToken").toString();
        int expireIn    = obj.value("expireIn").toInt(7200);
        m_tokenExpiry   = QDateTime::currentSecsSinceEpoch() + expireIn - 60;
        m_tokenState    = TokenState::Valid;

        auto cbs = std::move(m_pendingTokenCallbacks);
        m_pendingTokenCallbacks.clear();
        for (auto &cb : cbs) cb(m_accessToken);
    });
}

void DingTalkChannel::doSendViaWebhook(const QString &webhookUrl, const QString &content)
{
    QNetworkRequest req((QUrl(webhookUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["msgtype"] = "text";
    body["text"]    = QJsonObject{{"content", content}};

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logDT) << "Webhook send failed:" << reply->errorString();
    });
}

void DingTalkChannel::doSendViaToken(const QString &token, const QString &to,
                                      const QString &content, const QString &conversationType)
{
    if (token.isEmpty()) {
        qCWarning(logDT) << "No valid token for sending";
        return;
    }

    QString msgParam = QString::fromUtf8(
        QJsonDocument(QJsonObject{{"content", content}}).toJson(QJsonDocument::Compact));

    QJsonObject body;
    body["robotCode"] = m_clientId;
    body["msgKey"]    = "sampleText";
    body["msgParam"]  = msgParam;

    QString path;
    if (conversationType == QStringLiteral("group")) {
        path = kGroupMsgPath;
        body["openConversationId"] = to;
    } else {
        path = kSingleMsgPath;
        body["userIds"] = QJsonArray{to};
    }

    QNetworkRequest req(QUrl(QStringLiteral("%1%2").arg(kApiBase, path)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-acs-dingtalk-access-token", token.toUtf8());

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logDT) << "Token send failed:" << reply->errorString()
                             << reply->readAll();
    });
}

// ============================================================
// 流式卡片（打字机效果）
// ============================================================

QString DingTalkChannel::beginStreamingReply(const QString &to,
                                              const QString &conversationType)
{
    if (m_cardTemplateId.isEmpty())
        return {};   // 未配置卡片模板 ID，不支持流式，回退到普通消息

    QString handle = QUuid::createUuid().toString(QUuid::WithoutBraces);

    StreamingReply sr;
    sr.to               = to;
    sr.conversationType = conversationType;
    m_streamingReplies.insert(handle, sr);

    // 获取 token 后创建卡片实例
    fetchToken([this, handle](const QString &token) {
        if (!m_streamingReplies.contains(handle))
            return;
        if (token.isEmpty()) {
            m_streamingReplies[handle].failed = true;
            return;
        }
        doCreateCardInstance(handle);
    });

    return handle;
}

void DingTalkChannel::doCreateCardInstance(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    // outTrackId 与 handle 相同（客户端生成的 UUID）
    QJsonObject cardParamMap;
    cardParamMap["content"] = QStringLiteral("生成中...");

    QJsonObject body;
    body["cardTemplateId"] = m_cardTemplateId;
    body["outTrackId"]     = handle;
    body["cardData"]       = QJsonObject{{"cardParamMap", cardParamMap}};

    QNetworkRequest req(QUrl(QStringLiteral("%1%2").arg(kApiBase, kCardInstancePath)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-acs-dingtalk-access-token", m_accessToken.toUtf8());

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle] {
        reply->deleteLater();
        if (!m_streamingReplies.contains(handle))
            return;

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logDT) << "Create card instance failed:" << reply->errorString()
                             << reply->readAll();
            // 降级：卡片创建失败，标记 failed；有 pending 内容则发普通消息
            StreamingReply &sr = m_streamingReplies[handle];
            sr.failed = true;
            if (!sr.pendingContent.isEmpty()) {
                sendMessage(sr.to, sr.pendingContent, sr.conversationType);
                m_streamingReplies.remove(handle);
            } else if (sr.pendingFinalize) {
                m_streamingReplies.remove(handle);
            }
            return;
        }

        // 卡片实例创建成功，发送互动卡片消息给用户
        doSendInteractiveCard(handle);
    });
}

void DingTalkChannel::doSendInteractiveCard(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    const StreamingReply &sr = m_streamingReplies[handle];

    QJsonObject cardParamMap;
    cardParamMap["content"] = QStringLiteral("生成中...");

    QJsonObject body;
    body["robotCode"]      = m_clientId;
    body["outTrackId"]     = handle;
    body["cardTemplateId"] = m_cardTemplateId;
    body["cardData"]       = QJsonObject{{"cardParamMap", cardParamMap}};

    if (sr.conversationType == QStringLiteral("group")) {
        body["conversationType"]   = 2;
        body["openConversationId"] = sr.to;
    } else {
        body["conversationType"] = 1;
        body["userIds"]          = QJsonArray{sr.to};
    }

    QNetworkRequest req(QUrl(QStringLiteral("%1%2").arg(kApiBase, kInteractiveCardPath)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-acs-dingtalk-access-token", m_accessToken.toUtf8());

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle] {
        reply->deleteLater();
        if (!m_streamingReplies.contains(handle))
            return;

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logDT) << "Send interactive card failed:" << reply->errorString()
                             << reply->readAll();
            m_streamingReplies.remove(handle);
            return;
        }

        // 卡片已发送给用户，标记 ready
        m_streamingReplies[handle].ready = true;

        // 处理卡片发送前已到达的 update / finalize
        StreamingReply &sr = m_streamingReplies[handle];
        if (!sr.pendingContent.isEmpty()) {
            doUpdateCardStreaming(handle, sr.pendingContent, sr.pendingFinalize);
            sr.pendingContent.clear();
        } else if (sr.pendingFinalize) {
            doUpdateCardStreaming(handle, sr.lastContent, true);
        }
    });
}

void DingTalkChannel::updateStreamingReply(const QString &handle, const QString &content)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];

    if (sr.failed) {
        // 卡片创建失败，降级为普通消息
        sendMessage(sr.to, content, sr.conversationType);
        m_streamingReplies.remove(handle);
        return;
    }

    sr.lastContent = content;

    if (!sr.ready) {
        // 卡片尚未发送给用户，暂存内容
        sr.pendingContent = content;
        return;
    }

    doUpdateCardStreaming(handle, content, false);
}

void DingTalkChannel::finalizeStreamingReply(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];

    if (!sr.ready) {
        sr.pendingFinalize = true;
        return;
    }

    doUpdateCardStreaming(handle, sr.lastContent, true);
}

void DingTalkChannel::doUpdateCardStreaming(const QString &handle,
                                             const QString &content, bool isFull)
{
    if (!m_streamingReplies.contains(handle))
        return;

    // guid：每次调用的唯一标志，用于幂等
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonObject body;
    body["outTrackId"]  = handle;
    body["guid"]        = guid;
    body["key"]         = QStringLiteral("content");  // 卡片模板中流式变量名
    body["content"]     = content;
    body["isFull"]      = true;       // markdown 变量必须全量更新
    body["isFinalize"]  = isFull;     // isFull 参数复用为"是否最后一帧"语义
    body["isError"]     = false;

    QString url = QStringLiteral("%1%2").arg(kApiBase, kCardUpdateFmt);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-acs-dingtalk-access-token", m_accessToken.toUtf8());

    QNetworkReply *reply = m_http->sendCustomRequest(
        req, "PUT", QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle, isFull] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logDT) << "Update card streaming failed:" << reply->errorString()
                             << reply->readAll();
        if (isFull)
            m_streamingReplies.remove(handle);
    });
}
