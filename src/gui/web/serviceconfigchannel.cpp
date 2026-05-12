#include "serviceconfigchannel.h"

#include "database/appdatabase.h"
#include "dconfigmanager.h"
#include "global_define.h"
#include "mcpserver.h"
#include "dbus/localmodelserver.h"
#include "dbus/embeddingserver.h"
#include "agent/mcp/defaultagent.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

static QSharedPointer<MCPServer> getDefaultMcpServer()
{
    return DefaultAgent().mcpServer();
}

static QStringList enabledMcpServices()
{
    return DConfigManager::instance()->value(MCP_GROUP, MCP_ENABLED_LIST).toStringList();
}

static void setEnabledMcpServices(const QStringList &enabledList)
{
    DConfigManager::instance()->setValue(MCP_GROUP, MCP_ENABLED_LIST, enabledList);
}

static QString mcpCategory(const QString &serviceName, bool isBuiltIn)
{
    if (serviceName.compare("uos-mcp", Qt::CaseInsensitive) == 0)
        return QStringLiteral("systemBuiltIn");

    if (isBuiltIn)
        return QStringLiteral("thirdPartyBuiltIn");

    return QStringLiteral("custom");
}

static QJsonObject wrappedServerConfig(MCPServer *mcpServer, const QString &serviceName)
{
    QJsonObject root;
    root.insert("mcpServers", mcpServer->serverConfig(serviceName));
    return root;
}

static QString normalizeJsonString(const QJsonObject &jsonObject)
{
    return QString::fromUtf8(QJsonDocument(jsonObject).toJson(QJsonDocument::Indented));
}

static QJsonObject buildMcpServiceItem(MCPServer *mcpServer,
                                const QString &serviceName,
                                const QStringList &enabledList)
{
    const bool isBuiltIn = mcpServer->isBuiltin(serviceName);
    const bool removable = mcpServer->isRemovable(serviceName);
    const bool editable = !isBuiltIn && removable;

    QJsonObject item {
        {"id", serviceName},
        {"name", serviceName},
        {"description", mcpServer->description(serviceName)},
        {"category", mcpCategory(serviceName, isBuiltIn)},
        {"enabled", enabledList.contains(serviceName)},
        {"isBuiltIn", isBuiltIn},
        {"editable", editable},
        {"removable", removable},
    };

    if (editable)
        item.insert("jsonConfig", normalizeJsonString(wrappedServerConfig(mcpServer, serviceName)));

    return item;
}

static QJsonArray buildMcpServicesArray(MCPServer *mcpServer)
{
    mcpServer->scanServers();
    QStringList enabledList;

    // 未同意协议，关闭所有服务。同意了才去获取开启了哪些
    if (AppDatabase::instance()->getConfigBool(CONFIG_THIRD_PARTY_MCP))
        enabledList = enabledMcpServices();

    const QStringList serverNames = mcpServer->serverNames();

    QJsonArray services;
    for (const QString &serviceName : serverNames)
        services.append(buildMcpServiceItem(mcpServer, serviceName, enabledList));

    return services;
}

static QJsonObject buildResponse(bool success,
                          const QString &error = QString(),
                          const QJsonArray &services = QJsonArray())
{
    QJsonObject response {
        {"success", success},
        {"error", error},
        {"services", services},
        {"runtimeReady", getDefaultMcpServer() ? getDefaultMcpServer()->isRuntimeReady() : false},
        {"thirdPartyAgreementAccepted", AppDatabase::instance()->getConfigBool(CONFIG_THIRD_PARTY_MCP)},
    };

    return response;
}

static QJsonObject buildErrorResponse(const QString &error)
{
    return buildResponse(false, error);
}

static bool validateSingleMcpServiceConfig(const QJsonObject &root, QString *error)
{
    if (!root.contains("mcpServers") || !root.value("mcpServers").isObject()) {
        if (error)
            *error = QCoreApplication::translate("ServiceConfigChannel","The 'mcpServers' field is missing.");
        return false;
    }

    const QJsonObject mcpServers = root.value("mcpServers").toObject();
    const QStringList serviceNames = mcpServers.keys();

    if (serviceNames.isEmpty()) {
        if (error)
            *error = QCoreApplication::translate("ServiceConfigChannel","no valid mcp server.");
        return false;
    }

    if (serviceNames.size() > 1) {
        if (error)
            *error = QCoreApplication::translate("ServiceConfigChannel","Only one MCP service can be edited at a time.");
        return false;
    }

    const QJsonObject serviceInfo = mcpServers.value(serviceNames.first()).toObject();
    if (serviceInfo.isEmpty()) {
        if (error)
            *error = QCoreApplication::translate("ServiceConfigChannel","no valid mcp server.");
        return false;
    }

    if (serviceInfo.contains("url")) {
        if (serviceInfo.value("url").toString().trimmed().isEmpty()) {
            if (error)
                *error = QCoreApplication::translate("ServiceConfigChannel","url field cannot be empty.");
            return false;
        }
        return true;
    }

    if (serviceInfo.contains("command")) {
        if (serviceInfo.value("command").toString().trimmed().isEmpty()) {
            if (error)
                *error = QCoreApplication::translate("ServiceConfigChannel","command field cannot be empty.");
            return false;
        }
        return true;
    }

    if (error)
        *error = QCoreApplication::translate("ServiceConfigChannel","The 'command' or 'url' field is missing");
    return false;
}

static QJsonObject applyDescriptions(const QJsonObject &config, const QString &description)
{
    const QString trimmedDescription = description.trimmed();
    if (trimmedDescription.isEmpty())
        return config;

    if (!config.contains("mcpServers") || !config.value("mcpServers").isObject())
        return config;

    QJsonObject updatedConfig = config;
    QJsonObject updatedServers;
    const QJsonObject mcpServers = config.value("mcpServers").toObject();
    const QString locale = QLocale::system().name().simplified();
    const QString shortLocale = locale.section('_', 0, 0);

    for (auto it = mcpServers.begin(); it != mcpServers.end(); ++it) {
        if (!it.value().isObject()) {
            updatedServers.insert(it.key(), it.value());
            continue;
        }

        QJsonObject serverConfig = it.value().toObject();
        QJsonObject descriptions = serverConfig.value("descriptions").toObject();

        if (!descriptions.contains("generic"))
            descriptions.insert("generic", trimmedDescription);
        if (!descriptions.contains(shortLocale))
            descriptions.insert(shortLocale, trimmedDescription);
        descriptions.insert(locale, trimmedDescription);

        serverConfig.insert("descriptions", descriptions);
        updatedServers.insert(it.key(), serverConfig);
    }

    updatedConfig.insert("mcpServers", updatedServers);
    return updatedConfig;
}

static QJsonObject parseAndPrepareConfig(const QString &jsonConfig,
                                  const QString &description,
                                  QString *serviceName,
                                  QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonConfig.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QCoreApplication::translate("ServiceConfigChannel","JSON configuration format is invalid.");
        return QJsonObject();
    }

    const QJsonObject preparedConfig = applyDescriptions(doc.object(), description);
    if (!validateSingleMcpServiceConfig(preparedConfig, error))
        return QJsonObject();

    if (serviceName)
        *serviceName = preparedConfig.value("mcpServers").toObject().keys().first();

    return preparedConfig;
}

static bool rollbackImport(MCPServer *mcpServer, const QJsonObject &backupConfig)
{
    if (backupConfig.isEmpty())
        return true;

    const QJsonDocument backupDoc(backupConfig);
    return mcpServer->importCustomServers(QString::fromUtf8(backupDoc.toJson())).first;
}

ServiceConfigChannel::ServiceConfigChannel(QObject *parent)
    : QObject(parent)
{
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged, this, [this](const QStringList &files, int status){
        if (status != 1)
            return;

        qCInfo(logAIGUI) << "Knowledge base status changed, files:" << files;
        emit knowledgeBaseChanged(true);
    });

    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted, this, [this](const QStringList &files){
        if (files.isEmpty())
            return;

        emit knowledgeBaseChanged(checkKnowledgeBase());
    });

    connect(&LocalModelServer::getInstance(), &LocalModelServer::pluginStatusChanged, this, [this](const QString &app, bool isExist) {
        if (app == PLUGINSNAME)
            emit this->embeddingPluginsChanged(isExist);
        else if (app == UOSAIAGENTNAME)
            emit this->mcpPluginChanged(isExist);
    });
}

ServiceConfigChannel::~ServiceConfigChannel()
{
}

QJsonObject ServiceConfigChannel::getMcpServices()
{
    auto mcpServer = getDefaultMcpServer();
    if (!mcpServer)
        return buildErrorResponse(tr("Failed to get MCP server instance."));

    return buildResponse(true, QString(), buildMcpServicesArray(mcpServer.get()));
}

QJsonObject ServiceConfigChannel::saveMcpService(const QString &jsonConfig,
                                                 const QString &description,
                                                 const QString &editingServiceId)
{
    auto mcpServer = getDefaultMcpServer();
    if (!mcpServer)
        return buildErrorResponse(tr("Failed to get MCP server instance."));

    QString error;
    QString serviceName;
    const QJsonObject preparedConfig = parseAndPrepareConfig(jsonConfig, description, &serviceName, &error);
    if (preparedConfig.isEmpty())
        return buildErrorResponse(error);

    const QJsonDocument preparedDoc(preparedConfig);

    mcpServer->scanServers();

    QPair<bool, QString> result;
    const QString trimmedEditingId = editingServiceId.trimmed();
    if (trimmedEditingId.isEmpty()) {
        result = mcpServer->importCustomServers(QString::fromUtf8(preparedDoc.toJson()));
    } else if (trimmedEditingId == serviceName) {
        result = mcpServer->updateCustomServers(QString::fromUtf8(preparedDoc.toJson()));
    } else {
        const QJsonObject oldConfig = wrappedServerConfig(mcpServer.get(), trimmedEditingId);
        const bool oldExists = !mcpServer->serverConfig(trimmedEditingId).isEmpty();
        if (!oldExists || !mcpServer->isRemovable(trimmedEditingId))
            return buildErrorResponse(tr("Server '%1' is not a custom server or does not exist").arg(trimmedEditingId));

        if (mcpServer->serverNames().contains(serviceName))
            return buildErrorResponse(tr("MCP server name '%1' already exists").arg(serviceName));

        const auto removeResult = mcpServer->removeCustomServer(trimmedEditingId);
        if (!removeResult.first)
            return buildErrorResponse(removeResult.second);

        result = mcpServer->importCustomServers(QString::fromUtf8(preparedDoc.toJson()));
        if (!result.first)
            rollbackImport(mcpServer.get(), oldConfig);
    }

    if (!result.first)
        return buildErrorResponse(result.second);
    else if (trimmedEditingId.isEmpty())
        setMcpServiceEnabled(serviceName, true);

    return buildResponse(true, QString(), buildMcpServicesArray(mcpServer.get()));
}

QJsonObject ServiceConfigChannel::deleteMcpService(const QString &serviceId)
{
    auto mcpServer = getDefaultMcpServer();
    if (!mcpServer)
        return buildErrorResponse(tr("Failed to get MCP server instance."));

    mcpServer->scanServers();
    const auto result = mcpServer->removeCustomServer(serviceId);
    if (!result.first)
        return buildErrorResponse(result.second);

    QStringList enabledList = enabledMcpServices();
    enabledList.removeAll(serviceId);
    setEnabledMcpServices(enabledList);

    return buildResponse(true, QString(), buildMcpServicesArray(mcpServer.get()));
}

QJsonObject ServiceConfigChannel::setMcpServiceEnabled(const QString &serviceId, bool enabled)
{
    auto mcpServer = getDefaultMcpServer();
    if (!mcpServer)
        return buildErrorResponse(tr("Failed to get MCP server instance."));

    mcpServer->scanServers();
    if (!mcpServer->serverNames().contains(serviceId))
        return buildErrorResponse(tr("Server '%1' does not exist").arg(serviceId));

    QStringList enabledList = enabledMcpServices();
    if (enabled) {
        if (!enabledList.contains(serviceId))
            enabledList.append(serviceId);
    } else {
        enabledList.removeAll(serviceId);
    }

    setEnabledMcpServices(enabledList);
    return buildResponse(true, QString(), buildMcpServicesArray(mcpServer.get()));
}

bool ServiceConfigChannel::isMcpRuntimeReady()
{
    auto mcpServer = getDefaultMcpServer();
    return mcpServer ? mcpServer->isRuntimeReady() : false;
}

bool ServiceConfigChannel::getMcpThirdPartyAgreement()
{
    return AppDatabase::instance()->getConfigBool(CONFIG_THIRD_PARTY_MCP);
}

void ServiceConfigChannel::setMcpThirdPartyAgreement(bool agreed)
{
    return AppDatabase::instance()->saveConfigBool(CONFIG_THIRD_PARTY_MCP, agreed);
}

void ServiceConfigChannel::installApp(const QString &app)
{
    LocalModelServer::getInstance().openInstallWidgetOnTimer(app);
}

bool ServiceConfigChannel::checkKnowledgeBase()
{
    return EmbeddingServer::getInstance().getDocFiles().size();
}

bool ServiceConfigChannel::checkEmbeddingPlugins()
{
    return LocalModelServer::getInstance().checkInstallStatus(PLUGINSNAME);
}

bool ServiceConfigChannel::checkDocumentConversionCapability()
{
    return LocalModelServer::getInstance().checkInstallStatus(UOSAIAGENTNAME)
           && QFile::exists("/usr/lib/uos-ai-agent/deepresearch-servers/deepresearch-servers");
}

