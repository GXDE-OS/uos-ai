#include "chatbotservice.h"
#include "chatbotconfig.h"
#include "chatbotpaths.h"
#include "channelmanager.h"
#include "chatbotrequesthandler.h"
#include "session.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logChatBot, "uos-ai.chatbot")

using namespace uos_ai::chatbot;

ChatBotService::ChatBotService(QObject *parent)
    : QObject(parent)
    , m_channelManager(new ChannelManager(this))
    , m_handler(new ChatBotRequestHandler(this))
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ChatBotService::onConfigFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &ChatBotService::onConfigFileChanged);
}

ChatBotService::~ChatBotService()
{
    m_channelManager->stopAll();
}

void ChatBotService::initialize(const QSharedPointer<Session> &session)
{
    m_session = session;

    m_handler->setSession(session.get());
    m_handler->setChannelManager(m_channelManager);

    loadAndApply();
}

void ChatBotService::applyConfig(const QJsonObject &config)
{
    const QString filePath = ChatBotPaths::instance().configFile();
    const QString dirPath  = ChatBotPaths::instance().configDir();
    QDir().mkpath(dirPath);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(logChatBot) << "Failed to write chatbot config:" << filePath;
        return;
    }
    file.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
    qCInfo(logChatBot) << "Chatbot config saved to" << filePath;

    if (!m_watcher.directories().contains(dirPath))
        m_watcher.addPath(dirPath);
    if (!m_watcher.files().contains(filePath))
        m_watcher.addPath(filePath);
}

QJsonObject ChatBotService::config() const
{
    return m_config;
}

void ChatBotService::onConfigFileChanged(const QString &path)
{
    qCInfo(logChatBot) << "Chatbot config changed:" << path;

    const QString filePath = ChatBotPaths::instance().configFile();
    if (!m_watcher.files().contains(filePath) && QFile::exists(filePath))
        m_watcher.addPath(filePath);

    loadAndApply();
}

// ============================================================
// private
// ============================================================

void ChatBotService::loadAndApply()
{
    const QString filePath = ChatBotPaths::instance().configFile();
    const QString dirPath  = ChatBotPaths::instance().configDir();
    if (QDir(dirPath).exists() && !m_watcher.directories().contains(dirPath))
        m_watcher.addPath(dirPath);
    if (QFile::exists(filePath) && !m_watcher.files().contains(filePath))
        m_watcher.addPath(filePath);

    QString parseError;
    auto configOpt = ChatBotConfig::fromJson(readConfigFile(), &parseError);
    if (!configOpt) {
        qCWarning(logChatBot) << "Config missing or invalid"
                              << (parseError.isEmpty() ? QString() : (": " + parseError));

        // 有上一次有效配置则回滚，否则停止所有 channel
        if (m_lastValidConfig) {
            qCInfo(logChatBot) << "Rolling back to last valid config";
            if (m_lastValidConfig->enabled)
                m_channelManager->applyConfig(*m_lastValidConfig);
        } else {
            qCInfo(logChatBot) << "No previous config to roll back to, stopping all channels";
            m_config = {};
            m_channelManager->stopAll();
        }
        return;
    }

    const ChatBotConfig &config = *configOpt;
    m_lastValidConfig = config;
    m_config          = config.toJson();
    emit configChanged(m_config);

    if (!config.enabled) {
        qCInfo(logChatBot) << "Chatbot disabled, stopping all channels";
        m_channelManager->stopAll();
        return;
    }

    qCInfo(logChatBot) << "Applying channel config for platforms:" << config.platforms.keys();
    m_channelManager->applyConfig(config);
}

QJsonObject ChatBotService::readConfigFile() const
{
    const QString filePath = ChatBotPaths::instance().configFile();
    QFile file(filePath);
    if (!file.exists()) {
        qCDebug(logChatBot) << "Chatbot config not found:" << filePath;
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logChatBot) << "Cannot open chatbot config:" << filePath;
        return {};
    }

    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(file.readAll(), &err).object();
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logChatBot) << "Chatbot config parse error:" << err.errorString();
        return {};
    }

    return obj;
}
