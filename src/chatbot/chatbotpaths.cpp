#include "chatbotpaths.h"

#include <QFileInfo>
#include <QStandardPaths>

using namespace uos_ai::chatbot;

ChatBotPaths &ChatBotPaths::instance()
{
    static ChatBotPaths s;
    return s;
}

void ChatBotPaths::setOverrideDataDir(const QString &dir)
{
    m_overrideDataDir = dir;
}

QString ChatBotPaths::configFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + "/deepin/uos-ai-assistant/chatbot.json";
}

QString ChatBotPaths::configDir() const
{
    return QFileInfo(configFile()).absolutePath();
}

QString ChatBotPaths::dataDir() const
{
    if (!m_overrideDataDir.isEmpty())
        return m_overrideDataDir;
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
           + "/chatbot";
}

QString ChatBotPaths::profileFile() const
{
    return dataDir() + "/memory.json";
}

QString ChatBotPaths::conversationsDir(const QString &platform) const
{
    return dataDir() + "/" + platform + "/conversations";
}

QString ChatBotPaths::conversationFile(const QString &platform, const QString &encodedKey) const
{
    return conversationsDir(platform) + "/" + encodedKey + ".json";
}

QString ChatBotPaths::profileMdFile(const QString &platform) const
{
    return dataDir() + "/" + platform + "/PROFILE.md";
}

QString ChatBotPaths::soulMdFile(const QString &platform) const
{
    return dataDir() + "/" + platform + "/SOUL.md";
}
