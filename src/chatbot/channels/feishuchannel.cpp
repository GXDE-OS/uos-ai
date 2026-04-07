#include "feishuchannel.h"

// protobuf 生成代码（build 目录下）
#include "pbbp2.pb.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QLoggingCategory>
#include <QNetworkInterface>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QUuid>

Q_LOGGING_CATEGORY(logFS, "uos-ai.chatbot.feishu")

using namespace uos_ai::chatbot;

const char *FeishuChannel::kEndpointPath   = "/callback/ws/endpoint";
const char *FeishuChannel::kTokenPath      = "/open-apis/auth/v3/tenant_access_token/internal";
const char *FeishuChannel::kSendMsgPath    = "/open-apis/im/v1/messages";
const char *FeishuChannel::kCreateCardPath = "/open-apis/cardkit/v1/cards";
const char *FeishuChannel::kCardElementFmt = "/open-apis/cardkit/v1/cards/%1/elements/ai_reply/content";
const char *FeishuChannel::kCardSettingsFmt= "/open-apis/cardkit/v1/cards/%1/settings";
const char *FeishuChannel::kStreamElementId= "ai_reply";

// Frame::method 枚举值（来自 lark_oapi/ws/enum.py: CONTROL=0, DATA=1）
static constexpr int kMethodControl = 0;
static constexpr int kMethodData    = 1;

// Header key 常量
static const char *kHdrType      = "type";
static const char *kHdrMsgId     = "message_id";
static const char *kHdrSum       = "sum";
static const char *kHdrSeq       = "seq";
static const char *kHdrBizRt     = "biz_rt";

// ============================================================
// 辅助：从 Frame 的 headers 中按 key 取 value
// ============================================================
static QString getFrameHeader(const pbbp2::Frame &frame, const char *key)
{
    for (int i = 0; i < frame.headers_size(); ++i) {
        const pbbp2::Header &h = frame.headers(i);
        if (h.key() == key)
            return QString::fromStdString(h.value());
    }
    return QString();
}

// ============================================================
// 构造
// ============================================================

FeishuChannel::FeishuChannel(QObject *parent)
    : AbstractChannel(parent)
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_http(new QNetworkAccessManager(this))
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_fragmentCleanTimer(new QTimer(this))
    , m_dedupeCleanTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    m_fragmentCleanTimer->setInterval(10000);   // 每 10s 清理超时分片
    m_dedupeCleanTimer->setInterval(kDedupeCleanIntervalMs);

    connect(m_ws, &QWebSocket::connected,    this, &FeishuChannel::onWsConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &FeishuChannel::onWsDisconnected);
    connect(m_ws, &QWebSocket::binaryMessageReceived,
            this, &FeishuChannel::onWsBinaryMessageReceived);
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &FeishuChannel::onWsError);

    connect(m_pingTimer,         &QTimer::timeout, this, &FeishuChannel::sendPing);
    connect(m_reconnectTimer,    &QTimer::timeout, this, [this] { fetchEndpoint(); });
    connect(m_fragmentCleanTimer,&QTimer::timeout, this, &FeishuChannel::cleanupStaleFragments);
    connect(m_dedupeCleanTimer,  &QTimer::timeout, this, [this] {
        m_processedMsgIds.clear();
    });
}

// ============================================================
// 公共接口
// ============================================================

void FeishuChannel::start(const QJsonObject &config)
{
    m_appId     = config.value("app_id").toString();
    m_appSecret = config.value("app_secret").toString();
    QString domain = config.value("domain").toString("feishu");
    m_domain = (domain == QStringLiteral("lark"))
               ? QStringLiteral("https://open.larksuite.com")
               : QStringLiteral("https://open.feishu.cn");

    if (m_appId.isEmpty() || m_appSecret.isEmpty()) {
        qCWarning(logFS) << "Missing app_id or app_secret";
        return;
    }

    m_running = true;
    m_fragmentCleanTimer->start();
    m_dedupeCleanTimer->start();
    fetchEndpoint();
}

void FeishuChannel::stop()
{
    m_running = false;
    m_pingTimer->stop();
    m_reconnectTimer->stop();
    m_fragmentCleanTimer->stop();
    m_dedupeCleanTimer->stop();
    m_pendingActions.clear();
    m_fragments.clear();
    m_processedMsgIds.clear();
    m_streamingReplies.clear();

    if (m_ws->state() != QAbstractSocket::UnconnectedState)
        m_ws->close();
}

// ============================================================
// 连接流程
// ============================================================

void FeishuChannel::fetchEndpoint()
{
    qCInfo(logFS) << "Fetching Feishu WS endpoint...";

    QNetworkRequest req(QUrl(m_domain + kEndpointPath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("locale", "zh");

    QJsonObject body;
    body["AppID"]     = m_appId;
    body["AppSecret"] = m_appSecret;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logFS) << "Endpoint fetch failed:" << reply->errorString();
            scheduleReconnect();
            return;
        }

        QJsonObject obj  = QJsonDocument::fromJson(reply->readAll()).object();
        int code         = obj.value("code").toInt(-1);
        QJsonObject data = obj.value("data").toObject();
        QString wsUrl    = data.value("URL").toString();

        if (code != 0 || wsUrl.isEmpty()) {
            qCWarning(logFS) << "Invalid endpoint response (code=" << code << "):" << obj;
            scheduleReconnect();
            return;
        }

        // 解析 ClientConfig（可能含 ping interval 等参数）
        QJsonObject cfg = data.value("ClientConfig").toObject();
        if (cfg.contains("PingInterval"))
            m_pingIntervalMs = cfg.value("PingInterval").toInt(120) * 1000;

        // 从 URL query 中提取 service_id
        QUrl url(wsUrl);
        m_serviceId = QUrlQuery(url).queryItemValue("service_id").toInt();

        qCInfo(logFS) << "Connecting Feishu WebSocket...";
        m_ws->open(QUrl(wsUrl));
    });
}

void FeishuChannel::scheduleReconnect()
{
    if (!m_running)
        return;
    qCInfo(logFS) << "Scheduling reconnect in" << kReconnectInterval << "ms";
    m_reconnectTimer->start(kReconnectInterval);
}

void FeishuChannel::onWsConnected()
{
    qCInfo(logFS) << "Feishu WebSocket connected";
    m_pingTimer->setInterval(m_pingIntervalMs);
    m_pingTimer->start();
}

void FeishuChannel::onWsDisconnected()
{
    m_pingTimer->stop();
    qCInfo(logFS) << "Feishu WebSocket disconnected";
    if (m_running)
        scheduleReconnect();
}

void FeishuChannel::onWsError(QAbstractSocket::SocketError)
{
    qCWarning(logFS) << "Feishu WebSocket error:" << m_ws->errorString();
}

// ============================================================
// 二进制帧处理
// ============================================================

void FeishuChannel::onWsBinaryMessageReceived(const QByteArray &data)
{
    handleBinaryFrame(data);
}

void FeishuChannel::handleBinaryFrame(const QByteArray &data)
{
    pbbp2::Frame frame;
    if (!frame.ParseFromArray(data.constData(), data.size())) {
        qCWarning(logFS) << "Failed to parse Protobuf frame";
        return;
    }

    if (frame.method() == kMethodControl) {
        // 服务端不会向客户端发 PING；这里只处理服务端回的 PONG（含 ClientConfig 更新）
        if (getFrameHeader(frame, kHdrType) == QStringLiteral("pong") && frame.has_payload()) {
            QJsonObject cfg = QJsonDocument::fromJson(
                QByteArray::fromStdString(frame.payload())).object();
            if (cfg.contains("PingInterval")) {
                m_pingIntervalMs = cfg.value("PingInterval").toInt(120) * 1000;
                m_pingTimer->setInterval(m_pingIntervalMs);
            }
        }
        return;
    }

    if (frame.method() != kMethodData)
        return;

    QString msgId = getFrameHeader(frame, kHdrMsgId);
    QString sumStr = getFrameHeader(frame, kHdrSum);
    int sum        = sumStr.isEmpty() ? 1 : sumStr.toInt();
    QString seqStr = getFrameHeader(frame, kHdrSeq);
    int seq        = seqStr.isEmpty() ? 0 : seqStr.toInt();
    QString type  = getFrameHeader(frame, kHdrType);

    QByteArray payload;
    if (frame.has_payload())
        payload = QByteArray::fromStdString(frame.payload());

    if (sum > 1) {
        // 分片消息：缓存并等待全部到达
        payload = assemblePayload(msgId, sum, seq, payload);
        if (payload.isNull())
            return;  // 尚未收齐
    }

    handleDataFrame(data, payload, msgId, sum, seq, type);
}

void FeishuChannel::handleDataFrame(const QByteArray &frameBytes, const QByteArray &payload,
                                     const QString & /*msgId*/, int /*sum*/, int /*seq*/,
                                     const QString &type)
{
    if (type == QStringLiteral("event")) {
        handleEventPayload(frameBytes, payload);
    }
    // CARD 类型暂不处理，但仍需 ACK
    else {
        sendAck(frameBytes);
    }
}

void FeishuChannel::handleEventPayload(const QByteArray &frameBytes, const QByteArray &payload)
{
    QJsonObject event = QJsonDocument::fromJson(payload).object();

    QString eventType = event.value("header").toObject().value("event_type").toString();

    // 只处理消息接收事件
    if (eventType != QStringLiteral("im.message.receive_v1")) {
        sendAck(frameBytes);
        return;
    }

    QJsonObject ev      = event.value("event").toObject();
    QJsonObject msgObj  = ev.value("message").toObject();
    QString messageId   = msgObj.value("message_id").toString();

    // 消息去重
    if (m_processedMsgIds.contains(messageId)) {
        sendAck(frameBytes);
        return;
    }
    m_processedMsgIds.insert(messageId);

    QString msgType = msgObj.value("message_type").toString();
    QString contentRaw = msgObj.value("content").toString();

    QString textContent;
    if (msgType == QStringLiteral("text")) {
        QJsonObject contentObj = QJsonDocument::fromJson(contentRaw.toUtf8()).object();
        textContent = contentObj.value("text").toString().trimmed();
        // 去掉飞书 @ 语法 <at ...>...</at>
        static QRegularExpression atTag(QStringLiteral("<at[^>]*>[^<]*</at>"),
                                        QRegularExpression::CaseInsensitiveOption);
        textContent.remove(atTag);
        textContent = textContent.trimmed();
    } else {
        // 非文本消息，忽略
        sendAck(frameBytes);
        return;
    }

    if (textContent.isEmpty()) {
        sendAck(frameBytes);
        return;
    }

    // 发送者与会话信息
    QJsonObject sender     = ev.value("sender").toObject();
    QJsonObject senderId   = sender.value("sender_id").toObject();
    QString openId         = senderId.value("open_id").toString();
    QString chatId         = msgObj.value("chat_id").toString();
    QString chatType       = msgObj.value("chat_type").toString(); // p2p | group

    QString convType   = (chatType == QStringLiteral("p2p"))
                         ? QStringLiteral("user") : QStringLiteral("group");
    QString convId     = (convType == QStringLiteral("user")) ? openId : chatId;
    QString replyTo    = convId;   // sendMessage 的 to 参数

    QJsonObject payload_obj;
    payload_obj["platform"]   = platformName();
    payload_obj["message_id"] = messageId;
    payload_obj["sender"]     = QJsonObject{{"id", openId}, {"name", openId}, {"type", "user"}};
    payload_obj["conversation"] = QJsonObject{{"id", replyTo}, {"type", convType}};
    payload_obj["content"]    = QJsonObject{{"type", "text"}, {"text", textContent}};
    payload_obj["timestamp"]  = QDateTime::currentSecsSinceEpoch();

    sendAck(frameBytes);
    emit messageReceived(payload_obj);
}

// ============================================================
// ACK
// ============================================================

void FeishuChannel::sendAck(const QByteArray &originalFrameBytes, int code)
{
    pbbp2::Frame frame;
    if (!frame.ParseFromArray(originalFrameBytes.constData(), originalFrameBytes.size()))
        return;

    // 记录处理耗时（毫秒）
    pbbp2::Header *brt = frame.add_headers();
    brt->set_key(kHdrBizRt);
    brt->set_value("0");

    // 构造响应 payload
    QJsonObject resp;
    resp["code"]    = code;
    resp["headers"] = QJsonObject();
    resp["data"]    = "";
    frame.set_payload(QJsonDocument(resp).toJson(QJsonDocument::Compact).toStdString());

    std::string out;
    frame.SerializeToString(&out);
    m_ws->sendBinaryMessage(QByteArray::fromStdString(out));
}

// ============================================================
// Protobuf PING 保活
// ============================================================

void FeishuChannel::sendPing()
{
    if (m_ws->state() != QAbstractSocket::ConnectedState)
        return;

    pbbp2::Frame frame;
    frame.set_seqid(0);
    frame.set_logid(0);
    frame.set_service(m_serviceId);
    frame.set_method(kMethodControl);

    pbbp2::Header *h = frame.add_headers();
    h->set_key(kHdrType);
    h->set_value("ping");

    std::string out;
    frame.SerializeToString(&out);
    m_ws->sendBinaryMessage(QByteArray::fromStdString(out));
}

// ============================================================
// 分片合并
// ============================================================

QByteArray FeishuChannel::assemblePayload(const QString &msgId, int sum, int seq,
                                           const QByteArray &chunk)
{
    if (!m_fragments.contains(msgId)) {
        FragmentCache cache;
        cache.parts.resize(sum);
        cache.createdAt = QDateTime::currentSecsSinceEpoch();
        m_fragments[msgId] = std::move(cache);
    }

    FragmentCache &cache = m_fragments[msgId];
    if (seq < cache.parts.size())
        cache.parts[seq] = chunk;

    // 检查是否全部到达
    for (const QByteArray &p : cache.parts) {
        if (p.isNull())
            return QByteArray();  // 还有缺失分片
    }

    // 合并
    QByteArray assembled;
    for (const QByteArray &p : cache.parts)
        assembled.append(p);

    m_fragments.remove(msgId);
    return assembled;
}

void FeishuChannel::cleanupStaleFragments()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto it = m_fragments.begin(); it != m_fragments.end();) {
        if (now - it.value().createdAt > kFragmentTtlSecs)
            it = m_fragments.erase(it);
        else
            ++it;
    }
}

// ============================================================
// Token 管理
// ============================================================

void FeishuChannel::ensureToken(std::function<void()> action)
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (m_tokenState == TokenState::Valid && now < m_tokenExpiry) {
        action();
        return;
    }
    m_pendingActions.append(std::move(action));
    if (m_tokenState != TokenState::Refreshing)
        fetchToken();
}

void FeishuChannel::fetchToken()
{
    m_tokenState = TokenState::Refreshing;
    qCInfo(logFS) << "Fetching Feishu tenant_access_token...";

    QNetworkRequest req(QUrl(m_domain + kTokenPath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["app_id"]     = m_appId;
    body["app_secret"] = m_appSecret;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logFS) << "Token fetch failed:" << reply->errorString();
            m_tokenState = TokenState::Invalid;
            m_pendingActions.clear();
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.value("code").toInt(-1) != 0) {
            qCWarning(logFS) << "Token response error:" << obj;
            m_tokenState = TokenState::Invalid;
            m_pendingActions.clear();
            return;
        }

        m_accessToken = obj.value("tenant_access_token").toString();
        int expire    = obj.value("expire").toInt(7200);
        m_tokenExpiry = QDateTime::currentSecsSinceEpoch() + expire - 60;
        m_tokenState  = TokenState::Valid;

        flushPendingTokenActions();
    });
}

void FeishuChannel::flushPendingTokenActions()
{
    auto actions = std::move(m_pendingActions);
    m_pendingActions.clear();
    for (auto &f : actions)
        f();
}

// ============================================================
// 发送消息
// ============================================================

void FeishuChannel::sendMessage(const QString &to, const QString &content,
                                 const QString &conversationType)
{
    ensureToken([this, to, content, conversationType] {
        doSendMessage(to, content, conversationType);
    });
}

void FeishuChannel::doSendMessage(const QString &to, const QString &content,
                                   const QString &conversationType)
{
    // receive_id_type: p2p → open_id, group → chat_id
    QString receiveIdType = (conversationType == QStringLiteral("user"))
                            ? QStringLiteral("open_id") : QStringLiteral("chat_id");

    QString url = QStringLiteral("%1%2?receive_id_type=%3")
                  .arg(m_domain, kSendMsgPath, receiveIdType);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());

    // content 字段本身也是 JSON 字符串
    QString contentJson = QString::fromUtf8(
        QJsonDocument(QJsonObject{{"text", content}}).toJson(QJsonDocument::Compact));

    QJsonObject body;
    body["receive_id"] = to;
    body["msg_type"]   = "text";
    body["content"]    = contentJson;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logFS) << "Send failed:" << reply->errorString() << reply->readAll();
    });
}

// ============================================================
// 流式卡片
// ============================================================

QString FeishuChannel::beginStreamingReply(const QString &to,
                                            const QString &conversationType)
{
    QString handle = QUuid::createUuid().toString(QUuid::WithoutBraces);

    StreamingReply sr;
    sr.to               = to;
    sr.conversationType = conversationType;
    m_streamingReplies.insert(handle, sr);

    // 异步：获取 Token → 创建卡片 → 发送卡片消息
    ensureToken([this, handle] { doCreateStreamingCard(handle); });

    return handle;
}

void FeishuChannel::doCreateStreamingCard(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    // 构造 streaming_mode=true 的卡片 JSON（schema 2.0 要求有 header）
    QJsonObject cardJson;
    cardJson["schema"] = QStringLiteral("2.0");
    // 打字机速度配置
    QJsonObject printFreq{{"default", 30}, {"android", 30}, {"ios", 30}, {"pc", 30}};
    QJsonObject printStep{{"default", 2},  {"android", 2},  {"ios", 2},  {"pc", 2}};
    cardJson["config"] = QJsonObject{
        {"streaming_mode", true},
        {"streaming_config", QJsonObject{
            {"print_frequency_ms", printFreq},
            {"print_step",         printStep},
            {"print_strategy",     QStringLiteral("fast")}
        }}
    };

    QJsonObject textElem;
    textElem["tag"]        = QStringLiteral("markdown");
    textElem["content"]    = tr("Thinking...");
    textElem["element_id"] = QLatin1String(kStreamElementId);

    cardJson["body"] = QJsonObject{
        {"elements", QJsonArray{textElem}}
    };

    QString cardData = QString::fromUtf8(
        QJsonDocument(cardJson).toJson(QJsonDocument::Compact));

    QNetworkRequest req(QUrl(m_domain + kCreateCardPath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());

    QJsonObject body;
    body["type"] = QStringLiteral("card_json");
    body["data"] = cardData;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle] {
        reply->deleteLater();
        if (!m_streamingReplies.contains(handle))
            return;

        if (reply->error() != QNetworkReply::NoError) {
            QByteArray respBody = reply->readAll();
            qCWarning(logFS) << "Create streaming card failed:" << reply->errorString()
                             << respBody;
            // 降级：卡片创建失败（如缺少 cardkit:card:write 权限），
            // 保留 handle 条目并标记 failed，等 updateStreamingReply 到来时发普通消息
            if (m_streamingReplies.contains(handle)) {
                m_streamingReplies[handle].failed = true;
                StreamingReply &sr = m_streamingReplies[handle];
                if (!sr.pendingContent.isEmpty())
                    doSendMessage(sr.to, sr.pendingContent, sr.conversationType);
                if (sr.pendingContent.isEmpty() && sr.pendingFinalize)
                    m_streamingReplies.remove(handle);
            }
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.value("code").toInt(-1) != 0) {
            qCWarning(logFS) << "Create card error:" << obj;
            m_streamingReplies.remove(handle);
            return;
        }

        QString cardId = obj.value("data").toObject().value("card_id").toString();
        if (cardId.isEmpty()) {
            qCWarning(logFS) << "Empty card_id in response";
            m_streamingReplies.remove(handle);
            return;
        }

        m_streamingReplies[handle].cardId = cardId;

        // 发送卡片消息给用户（此时用户看到 "[生成中...]"）
        doSendCardMessage(handle);

        // 处理卡片创建前已到达的内容/finalize
        StreamingReply &sr = m_streamingReplies[handle];
        if (!sr.pendingContent.isEmpty()) {
            doUpdateCardContent(handle, sr.pendingContent);
            sr.pendingContent.clear();
        }
        if (sr.pendingFinalize)
            doFinalizeCard(handle);
    });
}

void FeishuChannel::doSendCardMessage(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    const StreamingReply &sr = m_streamingReplies[handle];
    QString receiveIdType = (sr.conversationType == QStringLiteral("user"))
                            ? QStringLiteral("open_id") : QStringLiteral("chat_id");

    QString url = QStringLiteral("%1%2?receive_id_type=%3")
                  .arg(m_domain, kSendMsgPath, receiveIdType);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());

    // content 字段是 JSON 字符串，内含 card_id
    QJsonObject cardRef;
    cardRef["type"] = QStringLiteral("card");
    cardRef["data"] = QJsonObject{{"card_id", sr.cardId}};

    QJsonObject body;
    body["receive_id"] = sr.to;
    body["msg_type"]   = QStringLiteral("interactive");
    body["content"]    = QString::fromUtf8(
        QJsonDocument(cardRef).toJson(QJsonDocument::Compact));

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logFS) << "Send card message failed:" << reply->errorString()
                             << reply->readAll();
    });
}

void FeishuChannel::updateStreamingReply(const QString &handle, const QString &content)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];

    if (sr.failed) {
        // 卡片创建失败，降级为普通消息
        doSendMessage(sr.to, content, sr.conversationType);
        m_streamingReplies.remove(handle);
        return;
    }

    if (sr.cardId.isEmpty()) {
        // 卡片尚未创建完成，暂存内容
        sr.pendingContent = content;
        return;
    }
    doUpdateCardContent(handle, content);
}

void FeishuChannel::doUpdateCardContent(const QString &handle, const QString &content)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];
    int seq = ++sr.sequence;
    ++sr.inflightUpdates;

    QString url = m_domain + QString::fromLatin1(kCardElementFmt).arg(sr.cardId);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());

    QJsonObject body;
    body["content"]  = content;
    body["sequence"] = seq;

    QNetworkReply *reply = m_http->sendCustomRequest(
        req, "PUT", QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle] {
        reply->deleteLater();
        if (!m_streamingReplies.contains(handle))
            return;
        StreamingReply &sr = m_streamingReplies[handle];
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logFS) << "Update card content failed:" << reply->errorString()
                             << reply->readAll();
        if (--sr.inflightUpdates == 0 && sr.pendingFinalize)
            doFinalizeCard(handle);
    });
}

void FeishuChannel::finalizeStreamingReply(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];
    if (sr.cardId.isEmpty()) {
        // 卡片还没创建完成，等卡片就绪后再 finalize
        sr.pendingFinalize = true;
        return;
    }
    doFinalizeCard(handle);
}

void FeishuChannel::doFinalizeCard(const QString &handle)
{
    if (!m_streamingReplies.contains(handle))
        return;

    StreamingReply &sr = m_streamingReplies[handle];

    // 若仍有进行中的内容更新请求，等其全部完成后再 finalize，
    // 避免 PATCH(finalize) 先于 PUT(content) 到达服务端导致最后内容被丢弃
    if (sr.inflightUpdates > 0) {
        sr.pendingFinalize = true;
        return;
    }

    int seq = ++sr.sequence;

    QString url = m_domain + QString::fromLatin1(kCardSettingsFmt).arg(sr.cardId);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());

    // settings 字段本身是 JSON 字符串
    QString settingsStr = QStringLiteral("{\"config\":{\"streaming_mode\":false}}");

    QJsonObject body;
    body["settings"] = settingsStr;
    body["sequence"] = seq;

    QNetworkReply *reply = m_http->sendCustomRequest(
        req, "PATCH", QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handle] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logFS) << "Finalize card failed:" << reply->errorString()
                             << reply->readAll();
        m_streamingReplies.remove(handle);
    });
}
