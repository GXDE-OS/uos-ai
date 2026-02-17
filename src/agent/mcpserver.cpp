#include "mcpserver.h"
#include "util.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFileInfo>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

MCPServer::MCPServer(const QString &agentName, QObject *parent)
    : QObject(parent)
    , m_agentName(agentName)
{
    // 设置自定义配置文件路径
    QString configDir = QString("%1/deepin/uos-ai-agent/%2/mcp-servers")
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .arg(agentName);

    QFileInfo configFile(configDir);
    if (!configFile.exists()) {
        qCDebug(logAgent) << "Creating mcp server directory:" << configDir;
        QDir(configFile.absoluteFilePath()).mkpath(".");
    }

    m_customConfigPath = configDir + "/uosai-user-mcp.json";
}

bool MCPServer::isRuntimeReady() const
{
    return QFileInfo::exists("/usr/bin/uos-aiagent-mcp");
}

void MCPServer::scanServers()
{
    m_servers.clear();
    
    // 扫描系统目录
    QString systemPath = QString("/usr/share/uos-ai-agent/%1/mcp-servers").arg(m_agentName);
    scanDirectory(systemPath);
    
    // 扫描用户目录
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    userPath = QString("%1/deepin/uos-ai-agent/%2/mcp-servers")
                  .arg(userPath)
                  .arg(m_agentName);
    scanDirectory(userPath);
       
    return;
}

QStringList MCPServer::serverNames() const
{
    return m_servers.keys();
}

QPair<bool, QString> MCPServer::updateCustomServers(const QString &cfgContent)
{
    QJsonDocument doc = QJsonDocument::fromJson(cfgContent.toUtf8());
    auto root = doc.object();
    if (!root.contains("mcpServers"))
        return qMakePair(false, tr("The 'mcpServers' field is missing."));

    QJsonObject objs = root.value("mcpServers").toObject();

    QMap<QString, QJsonObject> names;

    for (const QString &name : objs.keys()) {
        // 移除旧的配置
        if (!removeCustomServer(name).first)
            return qMakePair(false, tr("Remove old custom server config failed, server name: %0").arg(name));

        auto serverInfo = objs.value(name).toObject();
        // 验证服务信息
        auto validation = validateServerInfo(serverInfo);
        if (!validation.first) {
            return validation;
        }

        names.insert(name, serverInfo);
    }

    if (names.isEmpty()) {
        return qMakePair(false, tr("no valid mcp server."));
    }

    for (auto it = names.begin(); it != names.end(); ++it)
        addCustomServer(it.key(), it.value());

    return qMakePair(true, QString());
}

void MCPServer::scanDirectory(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }
    
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : jsonFiles) {
        QString filePath = dir.filePath(file);
        QFile jsonFile(filePath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            jsonFile.close();
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains("mcpServers") && root["mcpServers"].isObject()) {
                    QJsonObject servers = root["mcpServers"].toObject();
                    for (const QString &name : servers.keys()) {
                        if (m_servers.contains(name)) {
                            qCWarning(logAgent) << "Duplicate MCP server name found:" << name 
                                              << "in file:" << filePath;
                        } else {
                            auto info = servers.value(name).toObject().toVariantHash();
                            info.insert("uosai-file-path", filePath);
                            info.insert("uosai-removable", filePath == m_customConfigPath);
                            m_servers.insert(name, info);
                        }
                    }
                }
            }
        }
    }
}

QPair<bool, QString> MCPServer::importCustomServers(const QString &cfgContent)
{
    QJsonDocument doc = QJsonDocument::fromJson(cfgContent.toUtf8());
    auto root = doc.object();
    if (!root.contains("mcpServers"))
        return qMakePair(false, tr("The 'mcpServers' field is missing."));

    QJsonObject objs = root.value("mcpServers").toObject();

    QMap<QString, QJsonObject> names;

    for (const QString &name : objs.keys()) {
        if (names.contains(name))
            return qMakePair(false, tr("Duplicate MCP server name: %0.").arg(name));

        if (isServerNameExist(name))
            return qMakePair(false, tr("MCP server name '%1' already exists").arg(name));

        auto serverInfo = objs.value(name).toObject();
        // 验证服务信息
        auto validation = validateServerInfo(serverInfo);
        if (!validation.first) {
            return validation;
        }

        names.insert(name, serverInfo);
    }

    if (names.isEmpty()) {
        return qMakePair(false, tr("no valid mcp server."));
    }

    for (auto it = names.begin(); it != names.end(); ++it)
        addCustomServer(it.key(), it.value());

    return qMakePair(true, QString());
}

QPair<bool, QString> MCPServer::addCustomServer(const QString &name, const QJsonObject &serverInfo)
{
    // 检查服务名是否已存在
    if (isServerNameExist(name)) {
        return qMakePair(false, tr("Server name '%1' already exists").arg(name));
    }
    
    // 验证服务信息
    auto validation = validateServerInfo(serverInfo);
    if (!validation.first) {
        return validation;
    }
    
    // 读取或创建自定义配置文件
    QJsonObject customConfig;
    QFile customFile(m_customConfigPath);
    if (customFile.exists() && customFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(customFile.readAll());
        customFile.close();
        if (!doc.isNull() && doc.isObject()) {
            customConfig = doc.object();
        }
    }
    
    // 确保有mcpServers对象
    if (!customConfig.contains("mcpServers")) {
        customConfig["mcpServers"] = QJsonObject();
    }
    
    // 添加新服务
    QJsonObject servers = customConfig["mcpServers"].toObject();
    servers[name] = serverInfo;
    customConfig["mcpServers"] = servers;

    // 写入文件
    QDir().mkpath(QFileInfo(m_customConfigPath).absolutePath());
    if (customFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(customConfig);
        customFile.write(doc.toJson());
        customFile.close();
        
        // 更新内存中的服务列表
        {
            auto info = serverInfo.toVariantHash();
            info.insert("uosai-file-path", m_customConfigPath);
            info.insert("uosai-removable", true);
            m_servers.insert(name, info);
        }

        return qMakePair(true, QString());
    } else {
        return qMakePair(false, tr("Failed to open custom config file for writing"));
    }
}

QPair<bool, QString> MCPServer::removeCustomServer(const QString &name)
{
    // 检查是否是自定义服务
    if (!isRemovable(name)) {
        return qMakePair(false, tr("Server '%1' is not a custom server or does not exist").arg(name));
    }
    
    // 读取自定义配置文件
    QJsonObject customConfig;
    QFile customFile(m_customConfigPath);
    if (customFile.exists() && customFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(customFile.readAll());
        customFile.close();
        if (!doc.isNull() && doc.isObject()) {
            customConfig = doc.object();
        }
    }
    
    if (!customConfig.contains("mcpServers")) {
        return qMakePair(true, tr("No custom server found."));
    }
    
    // 移除服务
    QJsonObject servers = customConfig["mcpServers"].toObject();
    if (!servers.contains(name)) {
        return qMakePair(true, tr("Server '%1' not found.").arg(name));
    }
    
    servers.remove(name);
    customConfig["mcpServers"] = servers;
    
    // 写入文件
    if (customFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(customConfig);
        customFile.write(doc.toJson());
        customFile.close();
        
        // 更新内存中的服务列表
        m_servers.remove(name);
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, tr("Failed to open custom config file for writing"));
    }
}

bool MCPServer::isRemovable(const QString &name) const
{
    return m_servers.value(name).value("uosai-removable", false).toBool();
}

QString MCPServer::description(const QString &name) const
{
    auto info = m_servers.value(name);
    auto des = info.value("descriptions").toHash();

    const QString locale = QLocale::system().name().simplified();
    const QString shortLocale = Util::splitLocaleName(locale);
    QString ret = des.value(locale).toString();
    if (ret.isEmpty())
        ret = des.value(shortLocale).toString();
    if (ret.isEmpty())
        ret = des.value("generic").toString();

    return ret;
}

bool MCPServer::isBuiltin(const QString &name) const
{
    return false;
}

QJsonObject MCPServer::serverConfig(const QString &name) const
{
    if (!m_servers.contains(name))
        return QJsonObject();

    QJsonObject config = QJsonDocument::fromVariant(m_servers[name]).object();
    QStringList keys = config.keys();
    for (auto key : keys) {
        if (key.startsWith("uosai"))
            config.remove(key);
    }

    QJsonObject serverConfig;
    serverConfig[name] = config;

    return serverConfig;
}

bool MCPServer::isServerNameExist(const QString &name) const
{
    return m_servers.contains(name);
}

QPair<bool, QString> MCPServer::validateServerInfo(const QJsonObject &serverInfo) const
{
    if (serverInfo.contains("url")) {
        QString url = serverInfo["url"].toString();
        if (url.isEmpty())
            return qMakePair(false, tr("url field cannot be empty."));
        return qMakePair(true, QString());
    }

    if (serverInfo.contains("command")) {
        QString cmd = serverInfo["command"].toString();
        if (cmd.isEmpty())
            return qMakePair(false, tr("command field cannot be empty."));
        return qMakePair(true, QString());
    }

    return qMakePair(false, tr("The 'command' or 'url' field is missing"));
}

