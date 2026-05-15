#include "chatagent.h"

#include <QDir>
#include <QDate>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

ChatAgent::ChatAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "ChatAgent";
    m_description = "A Chat Agent.";
}

QList<ModelMessage> ChatAgent::initChatMessages(const ModelMessage &question, const QList<ModelMessage> &history) const
{
    QList<ModelMessage> initialMessages = history;
    // 添加当前用户问题
    initialMessages.append(question);

    return initialMessages;
}
