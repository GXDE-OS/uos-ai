#pragma once

#include "chatbotconfig.h"

#include <QByteArray>
#include <QMap>
#include <QObject>

namespace uos_ai {
namespace chatbot {

class AbstractChannel;

/**
 * @brief ChannelManager - 多平台 Channel 聚合管理器
 *
 * 替代原 ChatBotManager，直接管理各平台 Channel 实例，
 * 无需 Python 中间进程。
 */
class ChannelManager : public QObject
{
    Q_OBJECT

public:
    explicit ChannelManager(QObject *parent = nullptr);
    ~ChannelManager() override;

    /**
     * @brief 根据 ChatBotConfig 差量创建/更新/停止各 Channel
     *
     * 对每个平台计算配置的紧凑 JSON 哈希，仅在哈希变化时重启对应 Channel，
     * 避免无关平台因整体热重载而中断服务。
     */
    void applyConfig(const ChatBotConfig &config);

    /** 停止并销毁所有 Channel */
    void stopAll();

    /** 发送消息到指定平台 */
    void sendMessage(const QString &platform, const QString &to,
                     const QString &content, const QString &conversationType);

    /**
     * @brief 开始流式回复（委托给平台 Channel）。
     * @return 非空 handle 表示流式已启动；空字符串表示平台不支持，应回退到 sendMessage
     */
    QString beginStreamingReply(const QString &platform, const QString &to,
                                const QString &conversationType);

    /** 以新内容更新流式回复（全量文本，非增量） */
    void updateStreamingReply(const QString &platform, const QString &streamHandle,
                              const QString &content);

    /** 结束流式回复，关闭"生成中"状态 */
    void finalizeStreamingReply(const QString &platform, const QString &streamHandle);

Q_SIGNALS:
    /** 聚合所有平台消息，格式同 AbstractChannel::messageReceived */
    void messageReceived(const QJsonObject &payload);

    /**
     * @brief 某平台 channel 发生错误（如 WebSocket 断开、认证失败）。
     *
     * 连接方可据此取消该平台的进行中请求并通知用户。
     */
    void channelError(const QString &platform, const QString &error);

private:
    AbstractChannel *createChannel(const QString &platform);

    QMap<QString, AbstractChannel *> m_channels;      // platform → channel
    QMap<QString, QByteArray>        m_configHashes;  // platform → 上次应用时的紧凑 JSON 哈希
};

} // namespace chatbot
} // namespace uos_ai
