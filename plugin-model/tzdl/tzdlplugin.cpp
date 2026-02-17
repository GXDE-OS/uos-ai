// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tzdlplugin.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>

// 添加日志声明
Q_DECLARE_LOGGING_CATEGORY(logTzdl)

using namespace uos_ai;
using namespace tzdl;

TzdlPlugin::TzdlPlugin(QObject *parent)
    : QObject(parent),
      LLMPlugin()
{
    m_baseConfigPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + "/" + qApp->organizationName()
                        + "/" + qApp->applicationName() 
                        + "/"  + "plugin-model/";
    qCDebug(logTzdl) << "TzdlPlugin base config path:" << m_baseConfigPath;
    initConfigFile();
    loadFAQ();
}

QStringList TzdlPlugin::modelList() const
{
    qCDebug(logTzdl) << "Getting model list";
    // 循环m_config，返回所有TzdlLLM::modelID(configFileName)
    QStringList modelList;
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        QString configFileName = it.key();
        modelList.append(TzdlLLM::modelID(configFileName));
    }
    return modelList;
}

QStringList TzdlPlugin::roles(const QString &model) const
{
    // 循环m_config，根据model参数返回匹配的role
    QStringList roleList;
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        QString configFileName = it.key();
        if (model == TzdlLLM::modelID(configFileName)) {
            roleList.append(TzdlLLM::role(configFileName));
        }
    }
    return roleList;
}

QVariant TzdlPlugin::queryInfo(const QString &query, const QString &id)
{
    qCDebug(logTzdl) << "Querying info for:" << query << "with id:" << id;

    if (query == QUERY_DISPLAY_NAME)
    {
        for (auto it = m_config.begin(); it != m_config.end(); ++it) {
            QString configFileName = it.key();
            if (id == TzdlLLM::modelID(configFileName))
            {
                qCDebug(logTzdl) << "Returning display name for model";
                return it.value().agentLlmConfig.llmDisplayName;
            }
            else if (id == TzdlLLM::role(configFileName))
            {
                qCDebug(logTzdl) << "Returning display name for role";
                return it.value().agentLlmConfig.agentDisplayName;
            }
        }
    }
    else if (query == QUERY_ICON_NAME)
    {
        for (auto it = m_config.begin(); it != m_config.end(); ++it) {
            QString configFileName = it.key();
            if (id == TzdlLLM::role(configFileName)) {
                return it.value().agentLlmConfig.agentIcon;
            }else if (id == TzdlLLM::modelID(configFileName)) {
                return it.value().agentLlmConfig.iconPrefix + it.value().agentLlmConfig.llmIcon + ".svg";
            }
        }
    }
    else if (query == QUERY_DESCRIPTION)
    {
        for (auto it = m_config.begin(); it != m_config.end(); ++it) {
            QString configFileName = it.key();
            if (id == TzdlLLM::role(configFileName)) {
                return it.value().agentLlmConfig.description;
            }
        }
    }
    else if (query == QUERY_QUESTIONS)
    {
        for (auto it = m_config.begin(); it != m_config.end(); ++it) {
            QString configFileName = it.key();
            {
                qCDebug(logTzdl) << "Returning FAQ questions for role";
                QJsonArray jsonArray;
                for (const QJsonObject &obj : m_faq)
                    jsonArray.append(obj);

                QJsonDocument doc(jsonArray);
                QByteArray faqData = doc.toJson(QJsonDocument::Compact);
                return QVariant(faqData);
            }
        }
    }
    else if (query == QUERY_ICON_PREFIX)  //助手用
    {
        for (auto it = m_config.begin(); it != m_config.end(); ++it) {
            QString configFileName = it.key();
            if (id == TzdlLLM::role(configFileName)) {
                qCDebug(logTzdl) << "Returning icon prefix";
                return "file://" + it.value().agentLlmConfig.iconPrefix;
            }
        }
    }

    qCDebug(logTzdl) << "Unknown query type or id combination";
    return QVariant();
}

LLMModel *TzdlPlugin::createModel(const QString &name)
{
    qCDebug(logTzdl) << "Creating model:" << name;

    // 循环m_config，根据name匹配创建TzdlLLM实例
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        QString configFileName = it.key();
        TzdlConfig config = it.value();
        QString modelId = TzdlLLM::modelID(configFileName);
        if (name == modelId) {
            qCInfo(logTzdl) << "Creating new TzdlLLM instance: " << modelId;
            return new TzdlLLM(configFileName, config);
        }
    }

    qCWarning(logTzdl) << "Unknown model requested:" << name;
    return nullptr;
}

void TzdlPlugin::loadFAQ()
{
    QString faqPath = QString(ASSETS_INSTALL_DIR) + "assistant-ztbbd-inquiry.json";
    qCDebug(logTzdl) << "Loading FAQ from:" << faqPath;

    QFile file(faqPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(logTzdl) << "Failed to open ztbbd FAQ file:" << file.errorString();
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray())
    {
        qCWarning(logTzdl) << "ztbbd FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    qCDebug(logTzdl) << "Loaded" << jsonArray.size() << "FAQ items";

    foreach (const QJsonValue &value, jsonArray)
    {
        if (value.isObject())
        {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_faq.append(faqObject);
        }
    }

    qCInfo(logTzdl) << "Successfully loaded FAQ data";
}

void TzdlPlugin::initConfigFile()
{
    // 获取配置文件数量
    QDir dir(m_baseConfigPath);
    QStringList fileList = dir.entryList(QDir::Files);
    // 遍历fileList，如果文件名以"Tzdl"开头，则读取文件内容
    foreach (const QString &fileName, fileList)
    {
        if (fileName.startsWith("Tzdl"))
        {
            qCWarning(logTzdl) << "Config file:" << fileName;
            m_configFileLists << fileName;
            m_configFileCount++;
        }
    }

    if(m_configFileLists.length() == 0){
        // 如果文件不存在，则创建文件
        QString configFileName = "TzdlHttpServerConfig-0.json";
        QString configPath = m_baseConfigPath + configFileName;
        qCInfo(logTzdl) << "Not found TaiZhouDianLi config file path, create new config file:" << configPath;
        m_configFileLists << configFileName;
        TzdlConfig config;
        QString key = configFileName;
        if (key.endsWith(".json")) key.chop(5);
        m_config[key] = config;
        writeConfigToFile();
    } else {
        readConfigFromFile();
    }
}

void TzdlPlugin::writeConfigToFile()
{
    // 循环m_config，按文件名创建json配置文件
    for (auto it = m_config.begin(); it != m_config.end(); ++it)
    {
        QString key = it.key();
        TzdlConfig config = it.value();
        // 构建完整的文件路径（加回.json后缀）
        QString filePath = m_baseConfigPath + key + ".json";
        qCDebug(logTzdl) << "Writing config to file:" << filePath;
        
        // 创建QJsonObject来存储配置
        QJsonObject configObject;
        configObject["serverRootUrl"] = config.serverRootUrl;
        configObject["agentCode"] = config.agentCode;
        configObject["agentVersion"] = config.agentVersion;
        configObject["tokenID"] = config.tokenID;
        configObject["createSessionRoute"] = config.createSessionRoute;
        configObject["runSessionRoute"] = config.runSessionRoute;
        configObject["clearSessionRoute"] = config.clearSessionRoute;
        // 嵌套写入agentLlmConfig
        QJsonObject agentLlmConfigObj;
        agentLlmConfigObj["agentDisplayName"] = config.agentLlmConfig.agentDisplayName;
        agentLlmConfigObj["llmDisplayName"] = config.agentLlmConfig.llmDisplayName;
        agentLlmConfigObj["description"] = config.agentLlmConfig.description;
        agentLlmConfigObj["iconPrefix"] = config.agentLlmConfig.iconPrefix;
        agentLlmConfigObj["agentIcon"] = config.agentLlmConfig.agentIcon;
        agentLlmConfigObj["llmIcon"] = config.agentLlmConfig.llmIcon;
        configObject["agentLlmConfig"] = agentLlmConfigObj;
        // 创建QJsonDocument
        QJsonDocument doc(configObject);
        // 写入文件
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            qCInfo(logTzdl) << "Successfully wrote config to:" << filePath;
        }
        else
        {
            qCWarning(logTzdl) << "Failed to write config to:" << filePath << ", Error:" << file.errorString();
        }
    }
}

void TzdlPlugin::readConfigFromFile()
{
    // 先清空m_config
    m_config.clear();
    qCDebug(logTzdl) << "Reading config files from base path:" << m_baseConfigPath;
    // 按照m_configFileLists读取配置文件
    foreach (const QString &fileName, m_configFileLists)
    {
        QString filePath = m_baseConfigPath + fileName;
        qCDebug(logTzdl) << "Reading config file:" << filePath;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qCWarning(logTzdl) << "Failed to open config file:" << filePath << "Error:" << file.errorString();
            continue;
        }
        QJsonDocument configDoc = QJsonDocument::fromJson(file.readAll());
        QJsonObject configObj = configDoc.object();
        file.close();
        // 创建TzdlConfig对象并填充数据，参考tzdlllm.cpp的逻辑
        TzdlConfig config;
        if (configObj.contains("serverRootUrl")) {
            config.serverRootUrl = configObj.value("serverRootUrl").toString();
            qCDebug(logTzdl) << "Found serverRootUrl in config:" << config.serverRootUrl;
        } else {
            qCWarning(logTzdl) << "Missing serverRootUrl in config, using default:" << config.serverRootUrl;
        }
        if (configObj.contains("agentCode")) {
            config.agentCode = configObj.value("agentCode").toString();
            qCDebug(logTzdl) << "Found agentCode in config:" << config.agentCode;
        } else {
            qCWarning(logTzdl) << "Missing agentCode in config, using default:" << config.agentCode;
        }
        if (configObj.contains("agentVersion")) {
            config.agentVersion = configObj.value("agentVersion").toString();
            qCDebug(logTzdl) << "Found agentVersion in config:" << config.agentVersion;
        } else {
            qCWarning(logTzdl) << "Missing agentVersion in config, using default:" << config.agentVersion;
        }
        if (configObj.contains("tokenID")) {
            config.tokenID = configObj.value("tokenID").toString();
            qCDebug(logTzdl) << "Found tokenID in config:" << config.tokenID;
        } else {
            qCWarning(logTzdl) << "Missing tokenID in config, using default:" << config.tokenID;
        }
        if (configObj.contains("createSessionRoute")) {
            config.createSessionRoute = configObj.value("createSessionRoute").toString();
            qCDebug(logTzdl) << "Found createSessionRoute in config:" << config.createSessionRoute;
        } else {
            qCWarning(logTzdl) << "Missing createSessionRoute in config, using default:" << config.createSessionRoute;
        }
        if (configObj.contains("runSessionRoute")) {
            config.runSessionRoute = configObj.value("runSessionRoute").toString();
            qCDebug(logTzdl) << "Found runSessionRoute in config:" << config.runSessionRoute;
        } else {
            qCWarning(logTzdl) << "Missing runSessionRoute in config, using default:" << config.runSessionRoute;
        }
        if (configObj.contains("clearSessionRoute")) {
            config.clearSessionRoute = configObj.value("clearSessionRoute").toString();
            qCDebug(logTzdl) << "Found clearSessionRoute in config:" << config.clearSessionRoute;
        } else {
            qCWarning(logTzdl) << "Missing clearSessionRoute in config, using default:" << config.clearSessionRoute;
        }
        // 读取agentLlmConfig
        if (configObj.contains("agentLlmConfig") && configObj.value("agentLlmConfig").isObject()) {
            QJsonObject agentLlmConfigObj = configObj.value("agentLlmConfig").toObject();
            if (agentLlmConfigObj.contains("agentDisplayName")) {
                config.agentLlmConfig.agentDisplayName = agentLlmConfigObj.value("agentDisplayName").toString();
                qCDebug(logTzdl) << "Found agentDisplayName in agentLlmConfig:" << config.agentLlmConfig.agentDisplayName;
            } else {
                qCWarning(logTzdl) << "Missing agentDisplayName in agentLlmConfig, using default:" << config.agentLlmConfig.agentDisplayName;
            }
            if (agentLlmConfigObj.contains("llmDisplayName")) {
                config.agentLlmConfig.llmDisplayName = agentLlmConfigObj.value("llmDisplayName").toString();
                qCDebug(logTzdl) << "Found llmDisplayName in agentLlmConfig:" << config.agentLlmConfig.llmDisplayName;
            } else {
                qCWarning(logTzdl) << "Missing llmDisplayName in agentLlmConfig, using default:" << config.agentLlmConfig.llmDisplayName;
            }
            if (agentLlmConfigObj.contains("description")) {
                config.agentLlmConfig.description = agentLlmConfigObj.value("description").toString();
                qCDebug(logTzdl) << "Found description in agentLlmConfig:" << config.agentLlmConfig.description;
            } else {
                qCWarning(logTzdl) << "Missing description in agentLlmConfig, using default:" << config.agentLlmConfig.description;
            }
            if (agentLlmConfigObj.contains("iconPrefix")) {
                QString iconPrefix = agentLlmConfigObj.value("iconPrefix").toString();
                QDir iconDir(iconPrefix);
                if (iconDir.exists()) {
                    // 保证iconPrefix以/结尾
                    if (!iconPrefix.endsWith("/")) {
                        iconPrefix += "/";
                    }
                    config.agentLlmConfig.iconPrefix = iconPrefix;
                    qCDebug(logTzdl) << "Found iconPrefix in agentLlmConfig and directory exists:" << config.agentLlmConfig.iconPrefix;
                    // agentIcon 处理
                    if (agentLlmConfigObj.contains("agentIcon")) {
                        QString agentIcon = agentLlmConfigObj.value("agentIcon").toString();
                        qCDebug(logTzdl) << "Found agentIcon in agentLlmConfig:" << agentIcon;
                        QString agentIconPath = iconPrefix + agentIcon + ".svg";
                        QFileInfo iconFileInfo(agentIconPath);
                        if (iconFileInfo.exists()) {
                            // 拷贝三份
                            QStringList sizes = {"16", "32", "110"};
                            for (const QString &size : sizes) {
                                QString newIconPath = iconPrefix + agentIcon + "-" + size + ".svg";
                                if (!QFile::exists(newIconPath)) {
                                    QFile::copy(agentIconPath, newIconPath);
                                    qCWarning(logTzdl) << "Copied agentIcon to:" << newIconPath;
                                }
                            }
                            config.agentLlmConfig.agentIcon = agentIcon;
                            qCDebug(logTzdl) << "Set agentIcon in config.agentLlmConfig:" << config.agentLlmConfig.agentIcon;
                        } else {
                            qCWarning(logTzdl) << "agentIcon file not found, use default (not set):" << agentIconPath;
                        }
                    }
                    // llmIcon 处理
                    if (agentLlmConfigObj.contains("llmIcon")) {
                        QString llmIcon = agentLlmConfigObj.value("llmIcon").toString();
                        qCDebug(logTzdl) << "Found llmIcon in agentLlmConfig:" << llmIcon;
                        QString llmIconPath = iconPrefix + llmIcon + ".svg";
                        QFileInfo llmIconFileInfo(llmIconPath);
                        if (llmIconFileInfo.exists()) {
                            config.agentLlmConfig.llmIcon = llmIcon;
                            qCDebug(logTzdl) << "Set llmIcon in config.agentLlmConfig:" << config.agentLlmConfig.llmIcon;
                        } else {
                            qCWarning(logTzdl) << "llmIcon file not found, use default (not set):" << llmIconPath;
                        }
                    }
                } else {
                    qCWarning(logTzdl) << "iconPrefix directory does not exist, skip icon related config:" << iconPrefix;
                }
            } else {
                qCWarning(logTzdl) << "Missing iconPrefix in agentLlmConfig, using default:" << config.agentLlmConfig.iconPrefix;
            }
        } else {
            qCWarning(logTzdl) << "Missing agentLlmConfig in config, using default values.";
        }
        // 将配置添加到m_config中
        QString key = fileName;
        if (key.endsWith(".json")) key.chop(5);
        m_config[key] = config;
        qCInfo(logTzdl) << "Successfully loaded config from:" << filePath;
    }
    qCWarning(logTzdl) << "Loaded" << m_config.size() << "config files,"  << m_config.keys();
}
