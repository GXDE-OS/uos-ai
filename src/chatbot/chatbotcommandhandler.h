#pragma once

#include "chatbotpayload.h"

#include <QObject>
#include <QJsonObject>
#include <functional>

namespace uos_ai {
namespace chatbot {

class ChannelManager;
class ChatMemory;
class ChatBotRequestProcessor;

/**
 * @brief ChatBotCommandHandler - 处理 /new /stop /clear /help 等用户指令
 *
 * 职责：
 * - 识别并路由 / 开头的指令消息
 * - /new：通过 Processor 创建新会话 key，回复确认消息
 * - /stop：清理当前 target 的排队消息，通过 Processor 取消进行中请求
 * - /clear：通过 Processor 取消所有请求并清空 memory，清理调度状态
 * - /help：回复帮助文本
 *
 * /stop 和 /clear 需要操作 ChatBotRequestHandler 的调度状态（queue/activeTargets），
 * 通过注入的回调实现，避免双向依赖。
 */
class ChatBotCommandHandler : public QObject
{
    Q_OBJECT

public:
    explicit ChatBotCommandHandler(ChannelManager *channels,
                                   ChatMemory *memory,
                                   ChatBotRequestProcessor *processor,
                                   QObject *parent = nullptr);

    /**
     * @brief 尝试处理指令，返回 true 表示消息已被识别为指令并处理
     *
     * 调用方在收到消息后先调用此方法；返回 false 时按普通消息处理。
     */
    bool tryHandle(const QJsonObject &payload);

    /**
     * @brief 注入"清理指定 target 队列"回调（/stop 使用）
     *
     * 回调参数为 targetKey（platform:replyTo）。
     */
    void setQueueClearCallback(std::function<void(const QString &)> fn);

    /**
     * @brief 注入"清空所有调度状态"回调（/clear 使用）
     *
     * 回调负责清空 queue 和 activeTargets。
     */
    void setClearAllSchedulingCallback(std::function<void()> fn);

    /** 补充设置 ChannelManager（Handler 在 setChannelManager 时调用） */
    void setChannelManager(ChannelManager *channels);

private:
    void handleNew(const ParsedPayload &p);
    void handleStop(const ParsedPayload &p);
    void handleClear(const ParsedPayload &p);
    void handleHelp(const ParsedPayload &p);

    ChannelManager          *m_channels;
    ChatMemory              *m_memory;
    ChatBotRequestProcessor *m_processor;

    std::function<void(const QString &)> m_clearQueueFn;
    std::function<void()>                m_clearAllSchedulingFn;
};

} // namespace chatbot
} // namespace uos_ai
