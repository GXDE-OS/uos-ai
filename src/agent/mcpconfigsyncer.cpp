#include "mcpconfigsyncer.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"
#include "global_define.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFileInfo>

Q_DECLARE_LOGGING_CATEGORY(logAgent)
using namespace uos_ai;

McpConfigSyncer *McpConfigSyncer::instance()
{
    static McpConfigSyncer ins;
    return &ins;
}

McpConfigSyncer::McpConfigSyncer(QObject *parent)
    :QObject(parent)
{
    initServerAddress();
    m_status = Idle;

    // 每小时从服务器同步一次mcp配置
    m_syncTimer.setInterval(3600000);
    m_syncTimer.setSingleShot(true);
    connect(&m_syncTimer, &QTimer::timeout, this, [ this ] {
        fetchConfigFromServerAsync();
    });

}

void McpConfigSyncer::initServerAddress()
{
    QString testServerJsonPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/TestServer.json";
    if (QFileInfo::exists(testServerJsonPath)) {
        QFile dataInfoFile(testServerJsonPath);
        if (dataInfoFile.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = dataInfoFile.readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonObject jsonObj = jsonDoc.object();
            m_serverAddress = jsonObj["TestServer"].toString();
            qCWarning(logAgent) << "using test server:" << m_serverAddress;
            return;
        }
    }
    m_serverAddress = AIServer::ServerAPIAddress;
}

McpConfigSyncer::~McpConfigSyncer()
{
}

QNetworkReply::NetworkError McpConfigSyncer::fetchConfigFromServer(QJsonObject &configData)
{
    if (m_status == Syncing) {
        qCWarning(logAgent) << "Anthor sync task is running now, cancel this req.";
        return QNetworkReply::OperationCanceledError;
    }

    QString url = m_serverAddress + "/mcp-service-config";
    QNetworkReply::NetworkError error = QNetworkReply::NetworkError::NoError;

    QSharedPointer<HttpAccessmanager> httpAccessManager
        = QSharedPointer<HttpAccessmanager>(new HttpAccessmanager(""));
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (httpAccessManager != nullptr) {
        auto reply = httpAccessManager->get(req);
        HttpEventLoop loop(reply, "McpConfigSyncer::syncConfigFromServer");
        loop.setHttpOutTime(15000);
        loop.exec();

        auto resp = getHttpResponse(httpAccessManager, &loop);

        auto &obj = resp.second;
        if (resp.first == QNetworkReply::NetworkError::NoError) {
            if (obj.contains("datas"))
                obj = obj["datas"].toObject();

            configData = obj;
        } else {
            qCWarning(logAgent) << "Failed to sync MCP config, error:" << resp.first;
            error = resp.first;
        }
    } else {
        qCCritical(logAgent) << "HttpAccessmanager is null";
        error = QNetworkReply::NetworkError::UnknownServerError;
    }

    return error;
}

void McpConfigSyncer::fetchConfigFromServerAsync()
{
#ifndef ENABLE_MCP
    return; // mcp不可用时, 不需要去远程拉取mcp配置
#endif

    m_syncTimer.start();
    if (m_status == Syncing) {
        qCWarning(logAgent) << "Anthor sync task is running now, cancel this req.";
        return;
    }

    // Start async operation
    m_syncWatcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
    QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ this ]() {
        QJsonObject configData;
        auto ret = fetchConfigFromServer(configData);

        if (ret == QNetworkReply::NetworkError::NoError) {
            auto configs = configParse(configData);
            this->m_asyncConfigs = configs;
        }

        return ret;
    });

    m_syncWatcher->setFuture(future);
    connect(m_syncWatcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, &McpConfigSyncer::onAsyncFetchFinished);
}

void McpConfigSyncer::onAsyncFetchFinished()
{
    if (QNetworkReply::NoError == m_syncWatcher.data()->future().result()) {
        m_status = Success;

        auto configs = m_asyncConfigs;
        saveConfigToLocal(m_asyncConfigs);

        Q_EMIT asyncFetchSuccess();
    } else {
        m_status = Failed;
        qWarning() << "Fetch mcp configs from server failed! err code: " << m_syncWatcher.data()->future().result();
        Q_EMIT asyncFetchFailed();
    }
}

QPair<QNetworkReply::NetworkError, QJsonObject> McpConfigSyncer::getHttpResponse(const QSharedPointer<HttpAccessmanager> hacc, const HttpEventLoop *loop)
{
    QNetworkReply::NetworkError netReplyError = QNetworkReply::NetworkError::NoError;
    QJsonObject obj = {};

    bool isAuthError = hacc->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop->getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop->getNetWorkError();

    if (netReplyError == QNetworkReply::NetworkError::NoError) {
        QJsonDocument respJson = QJsonDocument::fromJson(loop->getHttpResult());
        if (respJson.isObject()) {
            obj = respJson.object();
            if (obj.contains("status")) {
                switch (obj["status"].toInt()) {
                case 200:
                    netReplyError = QNetworkReply::NetworkError::NoError;
                    break;
                case 201:
                    netReplyError = QNetworkReply::NetworkError::OperationNotImplementedError;
                    break;
                case 401:
                    netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
                    break;
                case 403:
                    netReplyError = QNetworkReply::NetworkError::ContentAccessDenied;
                    break;
                case 404:
                    netReplyError = QNetworkReply::NetworkError::HostNotFoundError;
                    break;
                default:
                    netReplyError = QNetworkReply::NetworkError::ServiceUnavailableError;
                    break;
                }
            }
        }
    }
    return QPair<QNetworkReply::NetworkError, QJsonObject>(netReplyError, obj);
}

QMap<QString, QJsonObject> McpConfigSyncer::configParse(const QJsonObject &configData) const
{
    QMap<QString, QJsonObject> configs {};

    auto jsonContent = QJsonDocument::fromJson(configData["jsonContent"].toString().toUtf8());
    if (!jsonContent.isObject())
        return configs;

    auto jsonObject = jsonContent.object();
    auto servers = jsonObject["mcpServers"].toObject();

    // 遍历配置数据中的所有键（服务名称）
    for (auto it = servers.begin(); it != servers.end(); ++it) {
        QString serviceName = it.key();
        QJsonValue configValue = it.value();
        
        // 检查配置值是否为对象
        if (configValue.isObject()) {
            QJsonObject serviceConfig = configValue.toObject();
            configs.insert(serviceName, serviceConfig);
        } else {
            qCWarning(logAgent) << "Invalid MCP service config for" << serviceName << ": expected object but got" << configValue.type();
        }
    }
    
    qCInfo(logAgent) << "Parsed" << configs.size() << "MCP service configurations";
    return configs;
}

bool McpConfigSyncer::ensureLocalConfigDirectory(const QString &path) const
{
    QDir dir(path);
    return dir.exists() || dir.mkpath(".");
}

void McpConfigSyncer::saveConfigToLocal(const QMap<QString, QJsonObject> &configs)
{
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    userPath = QString("%1/deepin/uos-ai-agent/%2/mcp-servers")
                   .arg(userPath)
                   .arg(kDefaultAgentName);

    if (!ensureLocalConfigDirectory(userPath)) {
        qCWarning(logAgent) << "Failed to create local config directory!";
        return;
    }

    // 创建包含mcpServers字段的根对象
    QJsonObject rootObject;
    QJsonObject mcpServers;

    // 将当前配置数据添加到mcpServers对象中
    for (auto it = configs.begin(); it != configs.end(); ++it) {
        mcpServers[it.key()] = it.value();
    }

    // 将mcpServers对象添加到根对象中
    rootObject["mcpServers"] = mcpServers;

    QFile file(userPath + "/" + MCP_BUILTIN_SERVERS_FILE);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(rootObject);
        qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        if (bytesWritten > 0)
            return;
    }
}
