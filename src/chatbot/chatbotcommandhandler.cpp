#include "chatbotcommandhandler.h"
#include "chatbotpayload.h"
#include "channelmanager.h"
#include "chatmemory.h"
#include "chatbotrequestprocessor.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logCommand, "uos-ai.chatbot.command")

using namespace uos_ai::chatbot;

ChatBotCommandHandler::ChatBotCommandHandler(ChannelManager *channels,
                                             ChatMemory *memory,
                                             ChatBotRequestProcessor *processor,
                                             QObject *parent)
    : QObject(parent)
    , m_channels(channels)
    , m_memory(memory)
    , m_processor(processor)
{
}

bool ChatBotCommandHandler::tryHandle(const QJsonObject &payload)
{
    // Quick check before full parse — most messages are not commands
    const QString content = payload.value("content").toObject()
                                   .value("text").toString().trimmed();
    if (!content.startsWith('/'))
        return false;

    const ParsedPayload p = ParsedPayload::from(payload);

    if (p.content == QLatin1String("/new")) {
        handleNew(p);
        return true;
    }
    if (p.content == QLatin1String("/stop")) {
        handleStop(p);
        return true;
    }
    if (p.content == QLatin1String("/clear")) {
        handleClear(p);
        return true;
    }
    if (p.content == QLatin1String("/help")) {
        handleHelp(p);
        return true;
    }

    // 未知指令
    m_channels->sendMessage(p.platform, p.replyTo,
        tr("Unknown command: %1\nSend /help to see available commands.").arg(p.content),
        p.convType);
    return true;
}

void ChatBotCommandHandler::setChannelManager(ChannelManager *channels)
{
    m_channels = channels;
}

void ChatBotCommandHandler::setQueueClearCallback(std::function<void(const QString &)> fn)
{
    m_clearQueueFn = std::move(fn);
}

void ChatBotCommandHandler::setClearAllSchedulingCallback(std::function<void()> fn)
{
    m_clearAllSchedulingFn = std::move(fn);
}

// ============================================================
// private
// ============================================================

void ChatBotCommandHandler::handleNew(const ParsedPayload &p)
{
    m_processor->newSession(p.memKey());
    m_channels->sendMessage(p.platform, p.replyTo, tr("New conversation started."), p.convType);
}

void ChatBotCommandHandler::handleStop(const ParsedPayload &p)
{
    const QString targetKey = p.platform + ":" + p.replyTo;

    // 先清空该 target 的等待队列，避免 requestFinished 后触发下一条
    if (m_clearQueueFn)
        m_clearQueueFn(targetKey);

    // 取消进行中请求，内部发送"已取消"消息并 emit requestFinished
    m_processor->cancelForTarget(p.platform, p.replyTo, p.convType);
}

void ChatBotCommandHandler::handleClear(const ParsedPayload &p)
{
    // 取消所有进行中请求，清空 Processor 内部状态
    m_processor->cancelAll();

    // 清空 Handler 的调度状态（queue + activeTargets）
    if (m_clearAllSchedulingFn)
        m_clearAllSchedulingFn();

    // PROFILE.md / SOUL.md 属于 Bot 身份配置，不受 /clear 影响
    m_memory->clearAll();

    qCInfo(logCommand) << "All conversation history cleared by /clear command";
    m_channels->sendMessage(p.platform, p.replyTo, tr("Conversation history has been cleared."), p.convType);
}

void ChatBotCommandHandler::handleHelp(const ParsedPayload &p)
{
    const QString msg = tr(
        "Available commands:\n"
        "  /help   — Show this help message\n"
        "  /new    — Start a new conversation (clears current context)\n"
        "  /stop   — Cancel the current in-progress request\n"
        "  /clear  — Clear all conversation history"
    );
    m_channels->sendMessage(p.platform, p.replyTo, msg, p.convType);
}
