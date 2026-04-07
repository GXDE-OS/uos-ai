#pragma once

#include "abstractchannel.h"

#include <QWebSocket>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QJsonObject>
#include <QMap>

namespace uos_ai {
namespace chatbot {

/**
 * @brief QQChannel - QQ 官方机器人频道（公域群/C2C/频道）
 *
 * 通信协议：
 *   接收：WebSocket Gateway（botpy 协议，JSON 帧，op 码驱动）
 *   发送：HTTPS REST API（api.sgroup.qq.com）
 *
 * 连接流程：
 *   fetchToken() → fetchGateway() → connectWs()
 *   WS 收到 op:10 Hello → sendIdentify / sendResume
 *   收到 op:0 READY  → 开始心跳 30s
 *
 * 重连：优先 Resume（保留 session_id + last_seq），降级 Identify。
 */
class QQChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit QQChannel(QObject *parent = nullptr);

    void    start(const QJsonObject &config) override;
    void    stop() override;
    void    sendMessage(const QString &to, const QString &content,
                        const QString &conversationType) override;
    bool    isRunning() const override { return m_running; }
    QString platformName() const override { return QStringLiteral("qq"); }

private Q_SLOTS:
    void onWsConnected();
    void onWsDisconnected();
    void onWsTextMessageReceived(const QString &message);
    void onWsError(QAbstractSocket::SocketError error);
    void sendHeartbeat();

private:
    // --- 连接流程 ---
    void fetchToken();
    void onTokenReply(const QByteArray &data);
    void fetchGateway();
    void onGatewayReply(const QByteArray &data);
    void connectWs();
    void scheduleReconnect();

    // --- WS 帧处理 ---
    void handleDispatch(const QJsonObject &msg);
    void handleMessage(const QJsonObject &d, const QString &convType);
    void sendIdentify();
    void sendResume();
    void sendWsJson(const QJsonObject &obj);

    // --- HTTP 发送 ---
    void doSendMessage(const QString &to, const QString &content,
                       const QString &conversationType);
    void ensureToken(std::function<void()> action);
    void flushPendingTokenActions();

    // --- Token ---
    QString m_appId;
    QString m_secret;
    QString m_accessToken;
    qint64  m_tokenExpiry = 0;      // Unix 秒

    enum class TokenState { Invalid, Refreshing, Valid };
    TokenState m_tokenState = TokenState::Invalid;
    QList<std::function<void()>> m_pendingActions;

    // --- WS 状态 ---
    QString m_sessionId;
    qint64  m_lastSeq = 0;
    bool    m_running = false;
    bool    m_canResume = false;

    // --- msg_id 缓存（QQ 被动消息，5 分钟有效）---
    struct MsgIdEntry { QString msgId; qint64 storedAt; };
    QMap<QString, MsgIdEntry> m_lastMsgIds;   // conversationId → entry

    // --- Qt 对象 ---
    QWebSocket             *m_ws = nullptr;
    QNetworkAccessManager  *m_http = nullptr;
    QTimer                 *m_heartbeatTimer = nullptr;
    QTimer                 *m_reconnectTimer = nullptr;
    QTimer                 *m_tokenTimer = nullptr;     // token 预刷新

    static constexpr int kReconnectInterval = 5000;    // ms
    static constexpr int kHeartbeatInterval = 30000;   // ms
    static constexpr int kMsgIdValidSecs    = 290;     // QQ 规定 5min，保守取 290s

    // --- QQ API 常量 ---
    static constexpr int kIntents = (1 << 25) | (1 << 30); // public_messages | public_guild_messages
    static const char *kTokenUrl;
    static const char *kGatewayUrl;
    static const char *kApiBase;
};

} // namespace chatbot
} // namespace uos_ai
