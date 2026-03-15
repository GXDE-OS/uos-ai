#include "sequentialagent.h"
#include "wrapper/llmservicevendor.h"
#include "networkdefs.h"

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

void SequentialAgent::setModel(QSharedPointer<LLM> llm)
{
    LlmAgent::setModel(llm);

    auto account = llm->account();
    for (QSharedPointer<LlmAgent> agent : m_subAgents.values()) {
        auto newllm = LLMVendor()->getCopilot(account);
        newllm->switchStream(false);
        connect(llm.data(), &LLM::aborted, newllm.data(), &LLM::aborted, Qt::DirectConnection);
        agent->setModel(newllm);
    }
}

QJsonObject SequentialAgent::processRequest(const QJsonObject &question, const QJsonArray &messages, const QVariantHash &params)
{
    QJsonObject response;
    QJsonArray currentMessages = messages;
    QJsonArray globalMessages = messages;
    QJsonObject currentQuestion = question;

    // 按顺序执行子智能体
    for (const QString &agentName : m_agentOrder) {
        if (canceled)
            break;

        auto subAgent = m_subAgents.value(agentName);
        if (!subAgent) {
            qCWarning(logAgent) << "Sub-agent not found:" << agentName;
            break;
        }

        // 调用子智能体前的处理
        if (!beforeSubAgentCall(agentName, currentQuestion, currentMessages, globalMessages))
            break;

        globalMessages = initChatMessages(currentQuestion, globalMessages);

        // 调用子智能体
        QJsonObject result = subAgent->processRequest(currentQuestion, currentMessages, params);

        if (subAgent->lastError() != AIServer::NoError) {
            m_llm->setLastError(subAgent->lastError());
            m_llm->setLastErrorString(subAgent->lastErrorString());
            response["context"] = globalMessages;
            return response;
        }

        // 更新当前问题和消息记录
        QString content = result.value("content").toString();
        qCInfo(logAgent) << subAgent->name() << "output" << content;

        if (!content.isEmpty()) {
            // 将子智能体的输出作为下一个智能体的输入
            currentQuestion = QJsonObject{{"content", content}};
            
            // 更新消息记录，添加子智能体的回复
            QJsonObject assistantMessage;
            assistantMessage["role"] = "assistant";
            assistantMessage["content"] = content;
            currentMessages.append(assistantMessage);

            assistantMessage["agent"] = agentName;
            globalMessages.append(assistantMessage);

            response["content"] = content;
        } else {
            break;
        }
    }

    response["context"] = globalMessages;

    return response;
}

bool SequentialAgent::beforeSubAgentCall(const QString &agentName, QJsonObject &currentQuestion, QJsonArray &localMessages, const QJsonArray &globalMessages)
{
    if (m_agentOrder.indexOf(agentName) != 0)
        localMessages = QJsonArray();

    return true;
}

} // namespace uos_ai 
