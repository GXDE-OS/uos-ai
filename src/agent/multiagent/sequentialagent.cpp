#include "sequentialagent.h"
#include "global_key_define.h"

#include "conversation/messagenode.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

SequentialAgent::SequentialAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "SequentialAgent";
}

SequentialAgent::~SequentialAgent()
{
}

void SequentialAgent::setModel(QSharedPointer<AbstractChatModel> llm)
{
    LlmAgent::setModel(llm);

    for (QSharedPointer<LlmAgent> agent : m_subAgents.values()) {
        agent->setModel(llm);
    }
}

QVariantHash SequentialAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &messages, const QVariantHash &params)
{
    QVariantHash response;
    QList<ModelMessage> currentMessages = messages;
    QList<ModelMessage> globalMessages = messages;
    ModelMessage currentQuestion = question;

    for (const QString &agentName : m_agentOrder) {
        if (canceled)
            break;

        auto subAgent = m_subAgents.value(agentName);
        if (!subAgent) {
            qCWarning(logAgent) << "Sub-agent not found:" << agentName;
            break;
        }

        if (!beforeSubAgentCall(agentName, currentQuestion, currentMessages, globalMessages))
            break;

        globalMessages = initChatMessages(currentQuestion, globalMessages);

        QVariantHash result = subAgent->processRequest(currentQuestion, currentMessages, params);

        if (subAgent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
            response[STR_KEY_CONTEXT] = QVariant::fromValue(QList<ModelMessage>());
            return response;
        }

        ModelMessage resultMsg = result.value(STR_KEY_CONTENT).value<ModelMessage>();
        QString content;
        if (!resultMsg.content.isEmpty()) {
            content = resultMsg.content.first().data.toString();
        }
        qCInfo(logAgent) << subAgent->name() << "output" << content;

        if (!content.isEmpty()) {
            currentQuestion.role = "user";
            currentQuestion.content = {{ContentType::CntText, content}};

            ModelMessage assistantMessage;
            assistantMessage.role = "assistant";
            assistantMessage.content = {{ContentType::CntText, content}};
            assistantMessage.source = agentName;
            currentMessages.append(assistantMessage);
            globalMessages.append(assistantMessage);

            response[STR_KEY_CONTENT] = QVariant::fromValue(resultMsg);
        } else {
            break;
        }
    }

    return response;
}

void SequentialAgent::cancel()
{
    LlmAgent::cancel();

    for (QSharedPointer<LlmAgent> agent : m_subAgents.values())
        agent->cancel();
}

bool SequentialAgent::beforeSubAgentCall(const QString &agentName, ModelMessage &currentQuestion, QList<ModelMessage> &localMessages, const QList<ModelMessage> &globalMessages)
{
    if (m_agentOrder.indexOf(agentName) != 0)
        localMessages = QList<ModelMessage>();

    return true;
}

} // namespace uos_ai 
