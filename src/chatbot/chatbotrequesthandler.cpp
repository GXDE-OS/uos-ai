#include "chatbotrequesthandler.h"
#include "chatbotrequestprocessor.h"
#include "chatbotcommandhandler.h"
#include "chatbotpayload.h"
#include "channelmanager.h"
#include "session.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logHandler, "uos-ai.chatbot.handler")

using namespace uos_ai::chatbot;

ChatBotRequestHandler::ChatBotRequestHandler(QObject *parent)
    : QObject(parent)
    , m_memory(this)
{
    m_processor = new ChatBotRequestProcessor(&m_memory, this);
    connect(m_processor, &ChatBotRequestProcessor::requestFinished,
            this, &ChatBotRequestHandler::onRequestFinished);

    // ChannelManager 将在 setChannelManager 时补充；构造时传 nullptr 安全，
    // 因为 onMessageReceived 在 setChannelManager 之后才连接。
    m_commandHandler = new ChatBotCommandHandler(nullptr, &m_memory, m_processor, this);

    m_commandHandler->setQueueClearCallback([this](const QString &targetKey) {
        m_queue.remove(targetKey);
    });
    m_commandHandler->setClearAllSchedulingCallback([this]() {
        m_queue.clear();
        m_activeTargets.clear();
    });
}

void ChatBotRequestHandler::setSession(Session *session)
{
    m_processor->setSession(session);
}

void ChatBotRequestHandler::setChannelManager(ChannelManager *manager)
{
    m_channelManager = manager;
    m_processor->setChannelManager(manager);
    m_commandHandler->setChannelManager(manager);

    connect(manager, &ChannelManager::messageReceived,
            this, &ChatBotRequestHandler::onMessageReceived);
}

// ============================================================
// slots
// ============================================================

void ChatBotRequestHandler::onMessageReceived(const QJsonObject &payload)
{
    if (!m_channelManager)
        return;

    // 先尝试指令处理
    if (m_commandHandler->tryHandle(payload))
        return;

    const ParsedPayload p   = ParsedPayload::from(payload);
    const QString targetKey = p.platform + ":" + p.replyTo;

    if (p.content.isEmpty())
        return;

    if (m_activeTargets.contains(targetKey)) {
        qCDebug(logHandler) << "Target busy, queuing message for" << targetKey;
        m_queue[targetKey].enqueue(payload);
        return;
    }

    m_activeTargets.insert(targetKey);
    m_processor->process(payload);
}

void ChatBotRequestHandler::onRequestFinished(const QString &targetKey)
{
    m_activeTargets.remove(targetKey);

    if (m_queue.contains(targetKey) && !m_queue[targetKey].isEmpty()) {
        QJsonObject next = m_queue[targetKey].dequeue();
        if (m_queue[targetKey].isEmpty())
            m_queue.remove(targetKey);

        m_activeTargets.insert(targetKey);
        m_processor->process(next);
    }
}
