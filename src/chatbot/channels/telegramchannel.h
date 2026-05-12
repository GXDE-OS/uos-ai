#pragma once

#include "abstractchannel.h"

#include <functional>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QSet>
#include <QTimer>

namespace uos_ai {
namespace chatbot {

class TelegramChannel : public AbstractChannel
{
public:
    explicit TelegramChannel(QObject *parent = nullptr);

    void    start(const QJsonObject &config) override;
    void    stop() override;
    void    sendMessage(const QString &to, const QString &content,
                        const QString &conversationType) override;
    bool    isRunning() const override { return m_running; }
    QString platformName() const override { return QStringLiteral("telegram"); }

    QString beginStreamingReply(const QString &to,
                                const QString &conversationType) override;
    void    updateStreamingReply(const QString &streamHandle,
                                 const QString &content) override;
    void    finalizeStreamingReply(const QString &streamHandle) override;

private:
    void applyCurrentProxy();

    struct StreamingReply {
        QString to;
        QString conversationType;
        QString messageId;
        QString pendingContent;
        QString lastSentContent;
        bool    ready = false;
        bool    failed = false;
        bool    pendingFinalize = false;
        bool    requestInFlight = false;
    };

    void initializeOffset();
    void pollUpdates();
    void scheduleNextPoll(int delayMs = 0);
    void handleUpdates(const QJsonArray &updates);
    void handleUpdate(const QJsonObject &update);
    void flushStreamingReply(const QString &handle);
    void failStreamingReply(const QString &handle, const QString &error);

    QString apiUrl(const QString &method) const;
    void postJson(const QString &method, const QJsonObject &body,
                  std::function<void(const QJsonValue &result)> onSuccess,
                  std::function<void(const QString &error)> onError);
    void appendProcessedUpdateId(qint64 updateId);
    bool hasProcessedUpdateId(qint64 updateId) const;

    QString m_botToken;
    QString m_apiBase;
    bool    m_running = false;
    bool    m_pollInFlight = false;
    qint64  m_offset = 0;
    bool    m_proxyWatcherConnected = false;

    QNetworkAccessManager *m_http = nullptr;
    QTimer                *m_pollTimer = nullptr;
    QSet<qint64>           m_processedUpdateIds;
    QList<qint64>          m_processedUpdateOrder;
    QMap<QString, StreamingReply> m_streamingReplies;

    static constexpr int kPollTimeoutSecs = 30;
    static constexpr int kRetryDelayMs = 3000;
    static constexpr int kMaxProcessedIds = 256;
};

} // namespace chatbot
} // namespace uos_ai
