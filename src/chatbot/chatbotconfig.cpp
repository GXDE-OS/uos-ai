#include "chatbotconfig.h"

#include <QJsonDocument>

using namespace uos_ai::chatbot;

// ============================================================
// ChannelConfig
// ============================================================

ChannelConfig ChannelConfig::fromJson(const QString &platformName, const QJsonObject &obj)
{
    ChannelConfig cfg;
    cfg.platform = platformName;
    cfg.enabled  = obj.value("enabled").toBool(false);
    cfg.m_raw    = obj;
    return cfg;
}

QString ChannelConfig::appId()        const { return m_raw.value("app_id").toString(); }
QString ChannelConfig::appSecret()    const { return m_raw.value("app_secret").toString(); }
QString ChannelConfig::clientId()     const { return m_raw.value("client_id").toString(); }
QString ChannelConfig::clientSecret() const { return m_raw.value("client_secret").toString(); }
QString ChannelConfig::secret()       const { return m_raw.value("secret").toString(); }

// ============================================================
// ChatBotConfig
// ============================================================

std::optional<ChatBotConfig> ChatBotConfig::fromJson(const QJsonObject &obj, QString *error)
{
    if (obj.isEmpty()) {
        if (error) *error = QStringLiteral("JSON object is empty");
        return std::nullopt;
    }

    ChatBotConfig cfg;
    cfg.enabled = obj.value("enabled").toBool(false);

    const QJsonObject platforms = obj.value("platforms").toObject();
    for (auto it = platforms.constBegin(); it != platforms.constEnd(); ++it) {
        const QString     name    = it.key();
        const QJsonObject platObj = it.value().toObject();
        if (platObj.isEmpty()) {
            if (error)
                *error = QStringLiteral("Platform '%1' config is not an object").arg(name);
            continue; // 跳过非法条目，不整体失败
        }
        cfg.platforms.insert(name, ChannelConfig::fromJson(name, platObj));
    }

    return cfg;
}

QJsonObject ChatBotConfig::toJson() const
{
    QJsonObject platformsObj;
    for (auto it = platforms.constBegin(); it != platforms.constEnd(); ++it)
        platformsObj.insert(it.key(), it.value().raw());

    QJsonObject root;
    root["enabled"]   = enabled;
    root["platforms"] = platformsObj;
    return root;
}
