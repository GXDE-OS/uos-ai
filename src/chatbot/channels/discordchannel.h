#pragma once

#include "abstractchannel.h"

#include <functional>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QWebSocket>

namespace uos_ai {
namespace chatbot {

class DiscordChannel : public AbstractChannel
{
public:
    explicit DiscordChannel(QObject *parent = nullptr);

    void    start(const QJsonObject &config) override;
    void    stop() override;
    void    sendMessage(const QString &to, const QString &content,
                        const QString &conversationType) override;
    bool    isRunning() const override { return m_running; }
    QString platformName() const override { return QStringLiteral("discord"); }

    QString beginStreamingReply(const QString &to,
                                const QString &conversationType) override;
    void    updateStreamingReply(const QString &streamHandle,
                                 const QString &content) override;
    void    finalizeStreamingReply(const QString &streamHandle) override;

private:
    void applyCurrentProxy();
    void handleProxySettingsChanged();

    struct StreamingReply {
        QString targetId;
        QString conversationType;
        QString channelId;
        QString messageId;
        QString interactionId;
        QString interactionToken;
        bool    interactionMode = false;
        QString pendingContent;
        QString lastSentContent;
        bool    ready = false;
        bool    failed = false;
        bool    pendingFinalize = false;
        bool    requestInFlight = false;
    };

    struct InteractionContext {
        QString interactionId;
        QString interactionToken;
        QString channelId;
    };

    void fetchGateway();
    void connectGateway();
    void scheduleReconnect(int delayMs = 5000);
    void sendGatewayPayload(int op, const QJsonValue &data);
    void sendHeartbeat();
    void handleGatewayMessage(const QString &message);
    void handleDispatch(const QString &eventType, const QJsonObject &data);
    void handleMessageCreate(const QJsonObject &data);
    void handleInteractionCreate(const QJsonObject &data);
    void syncCommands();
    QJsonObject buildCommandDefinition(const QString &commandName) const;
    void resolveChannelId(const QString &to, const QString &conversationType,
                          std::function<void(const QString &channelId)> onReady,
                          std::function<void(const QString &error)> onError);
    void createDmChannel(const QString &userId,
                         std::function<void(const QString &channelId)> onReady,
                         std::function<void(const QString &error)> onError);
    void sendChannelMessage(const QString &channelId, const QString &content,
                            std::function<void(const QJsonObject &result)> onSuccess,
                            std::function<void(const QString &error)> onError);
    void editChannelMessage(const QString &channelId, const QString &messageId,
                            const QString &content,
                            std::function<void()> onSuccess,
                            std::function<void(const QString &error)> onError);
    void deferInteractionResponse(const QString &interactionId, const QString &interactionToken,
                                  std::function<void()> onSuccess,
                                  std::function<void(const QString &error)> onError);
    void editInteractionResponse(const QString &interactionToken, const QString &content,
                                 std::function<void()> onSuccess,
                                 std::function<void(const QString &error)> onError);
    void flushStreamingReply(const QString &handle);
    void failStreamingReply(const QString &handle, const QString &error);
    void requestJsonValue(const QString &method, const QString &path,
                          const QJsonValue &body,
                          std::function<void(const QJsonValue &result)> onSuccess,
                          std::function<void(const QString &error)> onError);
    void requestJson(const QString &method, const QString &path,
                     const QJsonObject &body,
                     std::function<void(const QJsonObject &result)> onSuccess,
                     std::function<void(const QString &error)> onError);

    QString m_botToken;
    QString m_applicationId;
    QString m_guildId;
    QString m_gatewayUrl;
    QString m_resumeGatewayUrl;
    QString m_sessionId;
    bool    m_running = false;
    bool    m_gatewayFetchInFlight = false;
    bool    m_commandsSynced = false;
    bool    m_commandsSyncInFlight = false;
    bool    m_heartbeatAcked = true;
    qint64  m_lastSequence = 0;
    int     m_heartbeatIntervalMs = 0;
    bool    m_proxyWatcherConnected = false;

    QWebSocket            *m_ws = nullptr;
    QNetworkAccessManager *m_http = nullptr;
    QTimer                *m_heartbeatTimer = nullptr;
    QTimer                *m_reconnectTimer = nullptr;

    QMap<QString, QString> m_dmChannels;
    QMap<QString, QList<InteractionContext>> m_pendingInteractionContexts;
    QMap<QString, StreamingReply> m_streamingReplies;

    static constexpr int kIntentGuilds = 1 << 0;
    static constexpr int kIntentGuildMessages = 1 << 9;
    static constexpr int kIntentDirectMessages = 1 << 12;
};

} // namespace chatbot
} // namespace uos_ai
