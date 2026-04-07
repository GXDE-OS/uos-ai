#include "qqchannel.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logQQ, "uos-ai.chatbot.qq")

using namespace uos_ai::chatbot;

const char *QQChannel::kTokenUrl   = "https://bots.qq.com/app/getAppAccessToken";
const char *QQChannel::kGatewayUrl = "https://api.sgroup.qq.com/gateway/bot";
const char *QQChannel::kApiBase    = "https://api.sgroup.qq.com";

QQChannel::QQChannel(QObject *parent)
    : AbstractChannel(parent)
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_http(new QNetworkAccessManager(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_tokenTimer(new QTimer(this))
{
    m_heartbeatTimer->setInterval(kHeartbeatInterval);
    m_reconnectTimer->setSingleShot(true);
    m_tokenTimer->setSingleShot(true);

    connect(m_ws, &QWebSocket::connected,    this, &QQChannel::onWsConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &QQChannel::onWsDisconnected);
    connect(m_ws, &QWebSocket::textMessageReceived,
            this, &QQChannel::onWsTextMessageReceived);
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &QQChannel::onWsError);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &QQChannel::sendHeartbeat);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this] { fetchToken(); });
    connect(m_tokenTimer,     &QTimer::timeout, this, [this] { fetchToken(); });
}

// ============================================================
// 公共接口
// ============================================================

void QQChannel::start(const QJsonObject &config)
{
    m_appId  = config.value("app_id").toString();
    m_secret = config.value("token").toString();

    if (m_appId.isEmpty() || m_secret.isEmpty()) {
        qCWarning(logQQ) << "Missing app_id or token";
        return;
    }

    m_running = true;
    fetchToken();
}

void QQChannel::stop()
{
    m_running = false;
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();
    m_tokenTimer->stop();
    m_canResume = false;
    m_pendingActions.clear();

    if (m_ws->state() != QAbstractSocket::UnconnectedState)
        m_ws->close();
}

void QQChannel::sendMessage(const QString &to, const QString &content,
                             const QString &conversationType)
{
    ensureToken([this, to, content, conversationType] {
        doSendMessage(to, content, conversationType);
    });
}

// ============================================================
// Token 管理
// ============================================================

void QQChannel::ensureToken(std::function<void()> action)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (m_tokenState == TokenState::Valid && now < m_tokenExpiry) {
        action();
        return;
    }
    m_pendingActions.append(std::move(action));
    if (m_tokenState != TokenState::Refreshing)
        fetchToken();
}

void QQChannel::fetchToken()
{
    if (m_tokenState == TokenState::Refreshing)
        return;

    m_tokenState = TokenState::Refreshing;
    qCInfo(logQQ) << "Fetching QQ access token...";

    QNetworkRequest req((QUrl(kTokenUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["appId"]        = m_appId;
    body["clientSecret"] = m_secret;

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logQQ) << "Token fetch failed:" << reply->errorString();
            m_tokenState = TokenState::Invalid;
            scheduleReconnect();
            return;
        }
        onTokenReply(reply->readAll());
    });
}

void QQChannel::onTokenReply(const QByteArray &data)
{
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    QString token   = obj.value("access_token").toString();
    int expiresIn   = obj.value("expires_in").toString("600").toInt();

    if (token.isEmpty()) {
        qCWarning(logQQ) << "Invalid token response:" << data;
        m_tokenState = TokenState::Invalid;
        scheduleReconnect();
        return;
    }

    m_accessToken  = token;
    m_tokenExpiry  = QDateTime::currentSecsSinceEpoch() + expiresIn - 60;
    m_tokenState   = TokenState::Valid;
    qCInfo(logQQ) << "Token obtained, expires in" << expiresIn << "s";

    // 在过期前 60s 预刷新
    m_tokenTimer->setInterval((expiresIn - 120) * 1000);
    m_tokenTimer->start();

    flushPendingTokenActions();

    // 如果 WS 未连接，继续连接流程
    if (m_running && m_ws->state() == QAbstractSocket::UnconnectedState)
        fetchGateway();
}

void QQChannel::flushPendingTokenActions()
{
    auto actions = std::move(m_pendingActions);
    m_pendingActions.clear();
    for (auto &f : actions)
        f();
}

// ============================================================
// WebSocket 连接流程
// ============================================================

void QQChannel::fetchGateway()
{
    qCInfo(logQQ) << "Fetching QQ gateway URL...";

    QNetworkRequest req((QUrl(kGatewayUrl)));
    req.setRawHeader("Authorization",
                     QStringLiteral("QQBot %1").arg(m_accessToken).toUtf8());

    QNetworkReply *reply = m_http->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logQQ) << "Gateway fetch failed:" << reply->errorString();
            scheduleReconnect();
            return;
        }
        onGatewayReply(reply->readAll());
    });
}

void QQChannel::onGatewayReply(const QByteArray &data)
{
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    QString wsUrl   = obj.value("url").toString();

    if (wsUrl.isEmpty()) {
        qCWarning(logQQ) << "Empty gateway URL:" << data;
        scheduleReconnect();
        return;
    }

    qCInfo(logQQ) << "Gateway URL:" << wsUrl;
    m_ws->open(QUrl(wsUrl));
}

void QQChannel::connectWs()
{
    // 由 onGatewayReply 直接调用 m_ws->open，此处保留备用
}

void QQChannel::onWsConnected()
{
    qCInfo(logQQ) << "QQ WebSocket connected (waiting for Hello)";
    // 等待 op:10 Hello，在 onWsTextMessageReceived 中处理
}

void QQChannel::onWsDisconnected()
{
    m_heartbeatTimer->stop();
    qCInfo(logQQ) << "QQ WebSocket disconnected";

    if (m_running)
        scheduleReconnect();
}

void QQChannel::onWsError(QAbstractSocket::SocketError)
{
    qCWarning(logQQ) << "QQ WebSocket error:" << m_ws->errorString();
}

void QQChannel::scheduleReconnect()
{
    if (!m_running)
        return;
    qCInfo(logQQ) << "Scheduling reconnect in" << kReconnectInterval << "ms";
    m_reconnectTimer->start(kReconnectInterval);
}

// ============================================================
// WS 消息处理
// ============================================================

void QQChannel::onWsTextMessageReceived(const QString &message)
{
    QJsonObject msg = QJsonDocument::fromJson(message.toUtf8()).object();
    int op          = msg.value("op").toInt(-1);
    qint64 seq      = static_cast<qint64>(msg.value("s").toDouble(0));

    if (seq > 0)
        m_lastSeq = seq;

    switch (op) {
    case 10: { // Hello
        qCInfo(logQQ) << "Received Hello, authenticating...";
        if (!m_sessionId.isEmpty() && m_canResume)
            sendResume();
        else
            sendIdentify();
        m_heartbeatTimer->start();
        break;
    }
    case 11: // Heartbeat ACK
        break;
    case 7: // Reconnect
        m_canResume = true;
        m_ws->close();
        break;
    case 9: // Invalid Session
        qCWarning(logQQ) << "Invalid session, clearing state";
        m_sessionId.clear();
        m_lastSeq  = 0;
        m_canResume = false;
        m_ws->close();
        break;
    case 0: // Dispatch
        handleDispatch(msg);
        break;
    default:
        break;
    }
}

void QQChannel::handleDispatch(const QJsonObject &msg)
{
    QString t    = msg.value("t").toString();
    QJsonObject d = msg.value("d").toObject();

    if (t == QStringLiteral("READY")) {
        m_sessionId = d.value("session_id").toString();
        m_canResume = true;
        qCInfo(logQQ) << "QQ Bot ready, session:" << m_sessionId;
        return;
    }

    if (t == QStringLiteral("AT_MESSAGE_CREATE")) {
        handleMessage(d, QStringLiteral("guild"));
    } else if (t == QStringLiteral("GROUP_AT_MESSAGE_CREATE")) {
        handleMessage(d, QStringLiteral("group"));
    } else if (t == QStringLiteral("C2C_MESSAGE_CREATE")) {
        handleMessage(d, QStringLiteral("user"));
    }
}

void QQChannel::handleMessage(const QJsonObject &d, const QString &convType)
{
    // 提取发送者
    QJsonObject author = d.value("author").toObject();
    QString senderId   = author.value("id").toString();
    if (senderId.isEmpty())
        senderId = author.value("user_openid").toString();
    if (senderId.isEmpty())
        senderId = author.value("member_openid").toString();

    // 提取会话 ID
    QString conversationId;
    if (convType == QStringLiteral("group"))
        conversationId = d.value("group_openid").toString();
    else if (convType == QStringLiteral("guild"))
        conversationId = d.value("channel_id").toString();
    else
        conversationId = senderId;   // user: 用 openid 作为 conv id

    // 存储 msg_id 供被动回复
    QString msgId = d.value("id").toString();
    if (!msgId.isEmpty() && !conversationId.isEmpty()) {
        m_lastMsgIds[conversationId] = { msgId, QDateTime::currentSecsSinceEpoch() };
    }

    // 提取内容（去除 @bot 前缀）
    QString content = d.value("content").toString().trimmed();
    // QQ @ 消息 content 含 "<@bot_id> " 前缀，去掉
    if (content.startsWith('<')) {
        int end = content.indexOf('>');
        if (end != -1)
            content = content.mid(end + 1).trimmed();
    }

    QJsonObject payload;
    payload["platform"]   = platformName();
    payload["message_id"] = msgId;
    payload["sender"]     = QJsonObject{{"id", senderId}, {"name", "User"}, {"type", "user"}};
    payload["conversation"] = QJsonObject{{"id", conversationId}, {"type", convType}};
    payload["content"]    = QJsonObject{{"type", "text"}, {"text", content}};
    payload["timestamp"]  = QDateTime::currentSecsSinceEpoch();

    emit messageReceived(payload);
}

void QQChannel::sendIdentify()
{
    QJsonObject payload;
    payload["op"] = 2;
    payload["d"]  = QJsonObject{
        {"token",   QStringLiteral("QQBot %1").arg(m_accessToken)},
        {"intents", kIntents},
        {"shard",   QJsonArray{0, 1}}
    };
    sendWsJson(payload);
    qCInfo(logQQ) << "Sent Identify (intents=" << kIntents << ")";
}

void QQChannel::sendResume()
{
    QJsonObject payload;
    payload["op"] = 6;
    payload["d"]  = QJsonObject{
        {"token",      QStringLiteral("QQBot %1").arg(m_accessToken)},
        {"session_id", m_sessionId},
        {"seq",        m_lastSeq}
    };
    sendWsJson(payload);
    qCInfo(logQQ) << "Sent Resume (session=" << m_sessionId << ", seq=" << m_lastSeq << ")";
}

void QQChannel::sendHeartbeat()
{
    QJsonObject payload;
    payload["op"] = 1;
    payload["d"]  = m_lastSeq;
    sendWsJson(payload);
}

void QQChannel::sendWsJson(const QJsonObject &obj)
{
    if (m_ws->state() != QAbstractSocket::ConnectedState)
        return;
    m_ws->sendTextMessage(QString::fromUtf8(
        QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

// ============================================================
// HTTP 发送消息
// ============================================================

void QQChannel::doSendMessage(const QString &to, const QString &content,
                               const QString &conversationType)
{
    QString url;
    if (conversationType == QStringLiteral("group"))
        url = QStringLiteral("%1/v2/groups/%2/messages").arg(kApiBase, to);
    else if (conversationType == QStringLiteral("user"))
        url = QStringLiteral("%1/v2/users/%2/messages").arg(kApiBase, to);
    else
        url = QStringLiteral("%1/channels/%2/messages").arg(kApiBase, to);

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("QQBot %1").arg(m_accessToken).toUtf8());

    QJsonObject body;
    body["content"]  = content;
    body["msg_type"] = 0;

    // 被动回复：带 msg_id（5 分钟内有效）
    if (m_lastMsgIds.contains(to)) {
        const auto &entry = m_lastMsgIds[to];
        qint64 age = QDateTime::currentSecsSinceEpoch() - entry.storedAt;
        if (age < kMsgIdValidSecs)
            body["msg_id"] = entry.msgId;
        else
            m_lastMsgIds.remove(to);
    }

    QNetworkReply *reply = m_http->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qCWarning(logQQ) << "Send failed:" << reply->errorString()
                             << reply->readAll();
    });
}
