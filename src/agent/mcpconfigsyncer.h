#ifndef MCPCONFIGSYNCER_H
#define MCPCONFIGSYNCER_H

#include "tasdef.h"

#include <QJsonObject>
#include <QNetworkReply>
#include <QMutex>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#define MCP_BUILTIN_SERVERS_FILE "uosai-builtin-mcp.json"

class HttpAccessmanager;
class HttpEventLoop;
namespace uos_ai {
class McpConfigSyncer : public QObject
{
    Q_OBJECT
    enum SyncStatus {
        Idle,
        Syncing,
        Success,
        Failed
    };
public:
    static McpConfigSyncer* instance();
    ~McpConfigSyncer();

    // Synchronize MCP configuration from server
    QNetworkReply::NetworkError fetchConfigFromServer(QJsonObject &configData);
    void fetchConfigFromServerAsync();

    QMap<QString, QJsonObject> takeFetchResult();

    QString getLastError() const;
    int getErrorCode() const;

public Q_SLOTS:
    void onAsyncFetchFinished();

Q_SIGNALS:
    void asyncFetchSuccess();
    void asyncFetchFailed();

private:
    McpConfigSyncer(QObject *parent = nullptr);
    void initServerAddress();

    QPair<QNetworkReply::NetworkError, QJsonObject> getHttpResponse(const QSharedPointer<HttpAccessmanager> hacc, const HttpEventLoop *loop);
    QMap<QString, QJsonObject> configParse(const QJsonObject &configData) const;

    bool ensureLocalConfigDirectory(const QString &path) const;
    void saveConfigToLocal(const QMap<QString, QJsonObject> &configs);

private:
    QTimer m_syncTimer;
    QString m_agentName;

    SyncStatus m_status;
    QString m_serverAddress;

    QMap<QString, QJsonObject> m_asyncConfigs {};
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_syncWatcher;

    mutable QMutex m_configMutex;
};
}   // namespace uos_ai

#endif // MCPCONFIGSYNCER_H
