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
 * @brief DingTalkChannel - 钉钉机器人频道（Stream Mode 长连接）
 *
 * 接收：WebSocket Stream（JSON 帧）
 *   POST /v1.0/gateway/connections/open → 获取 endpoint + ticket
 *   连接 wss://{endpoint}?ticket={ticket}
 *   收到 CALLBACK 消息 → 解析 ChatbotMessage → 发 ACK
 *
 * 发送：优先 sessionWebhook（无需 token），过期后降级到 token API：
 *   POST /v1.0/robot/groupMessages/send     (群)
 *   POST /v1.0/robot/oToMessages/batchSend  (单聊)
 */
class DingTalkChannel : public AbstractChannel
{
    Q_OBJECT

public:
    explicit DingTalkChannel(QObject *parent = nullptr);

    void    start(const QJsonObject &config) override;
    void    stop() override;
    void    sendMessage(const QString &to, const QString &content,
                        const QString &conversationType) override;
    bool    isRunning() const override { return m_running; }
    QString platformName() const override { return QStringLiteral("dingtalk"); }

    QString beginStreamingReply(const QString &to,
                                const QString &conversationType) override;
    void    updateStreamingReply(const QString &streamHandle,
                                 const QString &content) override;
    void    finalizeStreamingReply(const QString &streamHandle) override;

private Q_SLOTS:
    void onWsConnected();
    void onWsDisconnected();
    void onWsTextMessageReceived(const QString &message);
    void onWsError(QAbstractSocket::SocketError error);

private:
    // --- 连接流程 ---
    void openConnection();
    void scheduleReconnect();

    // --- WS 帧处理 ---
    void handleCallback(const QJsonObject &msg);
    void sendAck(const QString &messageId);

    // --- Token（发送 fallback 用）---
    void fetchToken(std::function<void(const QString &)> callback);

    // --- 发送 ---
    void doSendViaWebhook(const QString &webhookUrl, const QString &content);
    void doSendViaToken(const QString &token, const QString &to,
                        const QString &content, const QString &conversationType);

    // --- 流式卡片 ---
    void doCreateCardInstance(const QString &handle);
    void doSendInteractiveCard(const QString &handle);
    void doUpdateCardStreaming(const QString &handle, const QString &content, bool isFull);

    struct StreamingReply {
        QString to;
        QString conversationType;
        QString pendingContent;   // 卡片发送前到达的内容
        QString lastContent;      // 最后一次 update 的内容（finalize 时使用）
        bool    pendingFinalize = false;
        bool    ready   = false;  // 卡片已发送给用户
        bool    failed  = false;  // 卡片创建失败，降级为普通消息
    };
    QMap<QString, StreamingReply> m_streamingReplies;

    // --- 凭据 ---
    QString m_clientId;
    QString m_clientSecret;
    QString m_cardTemplateId;  // 可选，流式 AI 卡片模板 ID；空则不启用流式

    // --- Token 缓存 ---
    QString m_accessToken;
    qint64  m_tokenExpiry = 0;   // Unix 秒

    enum class TokenState { Invalid, Refreshing, Valid };
    TokenState m_tokenState = TokenState::Invalid;
    QList<std::function<void(const QString &)>> m_pendingTokenCallbacks;

    // --- sessionWebhook 缓存（钉钉被动回复）---
    struct WebhookEntry { QString url; qint64 expiredAt; };  // expiredAt: Unix 毫秒
    QMap<QString, WebhookEntry> m_webhooks;   // conversationId → entry

    // --- 状态 ---
    bool m_running = false;

    QWebSocket            *m_ws = nullptr;
    QNetworkAccessManager *m_http = nullptr;
    QTimer                *m_pingTimer = nullptr;
    QTimer                *m_reconnectTimer = nullptr;

    static constexpr int kReconnectInterval = 10000;  // ms
    static constexpr int kPingInterval      = 60000;  // ms

    static const char *kApiBase;
    static const char *kOpenConnectionPath;
    static const char *kTokenPath;
    static const char *kGroupMsgPath;
    static const char *kSingleMsgPath;
    static const char *kCardInstancePath;    // POST 创建卡片实例
    static const char *kInteractiveCardPath; // POST 发送互动卡片
    static const char *kCardUpdateFmt;       // PUT  流式更新 /v1.0/card/streaming
};

} // namespace chatbot
} // namespace uos_ai
