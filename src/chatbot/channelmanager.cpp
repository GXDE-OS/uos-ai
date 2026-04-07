#include "channelmanager.h"
#include "channels/abstractchannel.h"
#include "channels/qqchannel.h"
#include "channels/dingtalkchannel.h"
#include "channels/feishuchannel.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <functional>

Q_LOGGING_CATEGORY(logCM, "uos-ai.chatbot.channelmanager")

using namespace uos_ai::chatbot;

ChannelManager::ChannelManager(QObject *parent)
    : QObject(parent)
{
}

ChannelManager::~ChannelManager()
{
    stopAll();
}

void ChannelManager::applyConfig(const ChatBotConfig &config)
{
    const QMap<QString, ChannelConfig> &platforms = config.platforms;

    // 停掉不再出现或 enabled=false 的 channel
    for (auto it = m_channels.begin(); it != m_channels.end();) {
        const QString &name = it.key();
        bool shouldStop = !platforms.contains(name) || !platforms.value(name).enabled;
        if (shouldStop) {
            qCInfo(logCM) << "Stopping channel:" << name;
            it.value()->stop();
            it.value()->deleteLater();
            m_configHashes.remove(name);
            it = m_channels.erase(it);
        } else {
            ++it;
        }
    }

    // 创建或更新 enabled 的 channel
    for (auto it = platforms.constBegin(); it != platforms.constEnd(); ++it) {
        const QString        &name       = it.key();
        const ChannelConfig  &channelCfg = it.value();

        if (!channelCfg.enabled)
            continue;

        // 计算本次配置的紧凑 JSON 哈希，与上次对比
        const QByteArray newHash = QJsonDocument(channelCfg.raw()).toJson(QJsonDocument::Compact);

        if (m_channels.contains(name)) {
            if (m_configHashes.value(name) == newHash)
                continue; // 配置未变，跳过
            // 配置变化：重启
            qCInfo(logCM) << "Restarting channel (config changed):" << name;
            m_channels[name]->stop();
            m_channels[name]->deleteLater();
            m_channels.remove(name);
            m_configHashes.remove(name);
        }

        AbstractChannel *ch = createChannel(name);
        if (!ch) {
            qCWarning(logCM) << "Unknown platform:" << name;
            continue;
        }

        connect(ch, &AbstractChannel::messageReceived,
                this, &ChannelManager::messageReceived);
        connect(ch, &AbstractChannel::errorOccurred,
                this, [this, name](const QString &err) {
            qCWarning(logCM) << "Channel error" << name << ":" << err;
            emit channelError(name, err);
        });

        m_channels.insert(name, ch);
        m_configHashes.insert(name, newHash);
        qCInfo(logCM) << "Starting channel:" << name;
        ch->start(channelCfg.raw());
    }
}

void ChannelManager::stopAll()
{
    for (AbstractChannel *ch : m_channels)
        ch->stop();
    qDeleteAll(m_channels);
    m_channels.clear();
    m_configHashes.clear();
}

void ChannelManager::sendMessage(const QString &platform, const QString &to,
                                  const QString &content, const QString &conversationType)
{
    AbstractChannel *ch = m_channels.value(platform, nullptr);
    if (!ch || !ch->isRunning()) {
        qCWarning(logCM) << "Channel not running:" << platform;
        return;
    }
    ch->sendMessage(to, content, conversationType);
}

QString ChannelManager::beginStreamingReply(const QString &platform, const QString &to,
                                             const QString &conversationType)
{
    AbstractChannel *ch = m_channels.value(platform, nullptr);
    if (!ch || !ch->isRunning())
        return {};
    return ch->beginStreamingReply(to, conversationType);
}

void ChannelManager::updateStreamingReply(const QString &platform,
                                           const QString &streamHandle,
                                           const QString &content)
{
    AbstractChannel *ch = m_channels.value(platform, nullptr);
    if (ch && ch->isRunning())
        ch->updateStreamingReply(streamHandle, content);
}

void ChannelManager::finalizeStreamingReply(const QString &platform,
                                             const QString &streamHandle)
{
    AbstractChannel *ch = m_channels.value(platform, nullptr);
    if (ch && ch->isRunning())
        ch->finalizeStreamingReply(streamHandle);
}

AbstractChannel *ChannelManager::createChannel(const QString &platform)
{
    // 注册表：平台名 → 工厂函数。新增平台只需在此添加一行，不修改其他逻辑。
    using FactoryFn = std::function<AbstractChannel *(QObject *)>;
    static const QMap<QString, FactoryFn> kRegistry = {
        { QStringLiteral("qq"),       [](QObject *p) { return new QQChannel(p); } },
        { QStringLiteral("dingtalk"), [](QObject *p) { return new DingTalkChannel(p); } },
        { QStringLiteral("feishu"),   [](QObject *p) { return new FeishuChannel(p); } },
        { QStringLiteral("lark"),     [](QObject *p) { return new FeishuChannel(p); } },
    };
    auto it = kRegistry.find(platform);
    return it != kRegistry.end() ? it.value()(this) : nullptr;
}
