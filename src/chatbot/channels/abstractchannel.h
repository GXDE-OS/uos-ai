#pragma once

#include <QFile>
#include <QObject>
#include <QJsonObject>

namespace uos_ai {
namespace chatbot {

/**
 * @brief AbstractChannel - 平台 IM 频道抽象接口
 *
 * 统一的消息收发抽象，各平台（QQ/飞书/钉钉）各自实现。
 * messageReceived 信号使用与原 Python chatbot 兼容的统一 JSON 格式：
 * {
 *   "platform":     "qq|feishu|dingtalk",
 *   "message_id":   "...",
 *   "sender":       { "id": "...", "name": "...", "type": "user" },
 *   "conversation": { "id": "...", "type": "user|group|guild" },
 *   "content":      { "type": "text", "text": "..." },
 *   "timestamp":    1234567890
 * }
 */
class AbstractChannel : public QObject
{
    Q_OBJECT

public:
    explicit AbstractChannel(QObject *parent = nullptr) : QObject(parent) {}
    ~AbstractChannel() override = default;

    virtual void    start(const QJsonObject &config) = 0;
    virtual void    stop() = 0;
    virtual void    sendMessage(const QString &to, const QString &content,
                                const QString &conversationType) = 0;
    virtual bool    isRunning() const = 0;
    virtual QString platformName() const = 0;

    /**
     * @brief 开始流式回复（可选实现）。
     *
     * 实现类应在此发送初始占位消息（如"生成中..."），并返回一个不透明的 handle
     * 供后续 updateStreamingReply / finalizeStreamingReply 使用。
     * 默认实现返回空字符串，表示该平台不支持流式回复，
     * 调用方应回退到 sendMessage。
     *
     * @param to               接收方 ID（open_id / chat_id）
     * @param conversationType "user" 或 "group"
     * @return 非空 handle 表示流式已启动；空字符串表示不支持
     */
    virtual QString beginStreamingReply(const QString &to,
                                        const QString &conversationType)
    {
        Q_UNUSED(to); Q_UNUSED(conversationType);
        return {};
    }

    /**
     * @brief 以新内容更新流式回复（追加/替换）。
     * @param streamHandle  beginStreamingReply 返回的 handle
     * @param content       当前全量文本（非增量）
     */
    virtual void updateStreamingReply(const QString &streamHandle,
                                      const QString &content)
    {
        Q_UNUSED(streamHandle); Q_UNUSED(content);
    }

    /**
     * @brief 结束流式回复，关闭"生成中"状态。
     * @param streamHandle  beginStreamingReply 返回的 handle
     */
    virtual void finalizeStreamingReply(const QString &streamHandle)
    {
        Q_UNUSED(streamHandle);
    }

Q_SIGNALS:
    void messageReceived(const QJsonObject &payload);
    void errorOccurred(const QString &error);
};

} // namespace chatbot
} // namespace uos_ai
