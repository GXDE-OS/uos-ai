#include "chatbotconfig.h"
#include "chatbot_key_define.h"
#include "global_key_define.h"

#include <QJsonDocument>

using namespace uos_ai;
using namespace uos_ai::chatbot;

// ============================================================
// ChannelConfig
// ============================================================

ChannelConfig ChannelConfig::fromJson(const QString &platformName, const QJsonObject &obj)
{
    ChannelConfig cfg;
    cfg.platform = platformName;
    cfg.enabled  = obj.value(STR_KEY_ENABLED).toBool(false);
    cfg.m_raw    = obj;
    return cfg;
}

QString ChannelConfig::appId()        const { return m_raw.value(STR_KEY_APP_ID).toString(); }
QString ChannelConfig::appSecret()    const { return m_raw.value(STR_KEY_APP_SECRET).toString(); }
QString ChannelConfig::clientId()     const { return m_raw.value(STR_KEY_CLIENT_ID).toString(); }
QString ChannelConfig::clientSecret() const { return m_raw.value(STR_KEY_CLIENT_SECRET).toString(); }
QString ChannelConfig::secret()       const { return m_raw.value(STR_KEY_SECRET).toString(); }
QString ChannelConfig::botToken()     const { return m_raw.value(STR_KEY_BOT_TOKEN).toString(); }
QString ChannelConfig::applicationId() const { return m_raw.value(STR_KEY_APPLICATION_ID).toString(); }
QString ChannelConfig::guildId()      const { return m_raw.value(STR_KEY_GUILD_ID).toString(); }
QString ChannelConfig::apiBase()      const { return m_raw.value(STR_KEY_API_BASE).toString(); }

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
    cfg.enabled = obj.value(STR_KEY_ENABLED).toBool(false);

    const QJsonObject platforms = obj.value(STR_KEY_PLATFORMS).toObject();
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
    root[STR_KEY_ENABLED]   = enabled;
    root[STR_KEY_PLATFORMS] = platformsObj;
    return root;
}
