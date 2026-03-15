#include "commanderagent.h"
#include "oaifunctionparser.h"
#include "wrapper/llmservicevendor.h"
#include "networkdefs.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

CommanderAgent::CommanderAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "CommanderAgent";
}

CommanderAgent::~CommanderAgent()
{
}

bool CommanderAgent::initialize()
{
    m_tools.append(subagentTool());
    return true;
}

QString CommanderAgent::systemPrompt() const
{
    return m_systemPrompt + subagentPrompt();
}

void CommanderAgent::setModel(QSharedPointer<LLM> llm)
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

QJsonArray CommanderAgent::initChatMessages(const QJsonObject &question, const QJsonArray &messages) const
{
    QJsonArray initialMessages;
    
    // 添加系统消息
    {
        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt();
        initialMessages.append(systemMessage);
    }
    
    // 添加历史消息，过滤掉system角色的消息
    for (const QJsonValue &msg : messages) {
        QJsonObject msgObj = msg.toObject();
        if (msgObj["role"].toString() != "system") {
            initialMessages.append(msg);
        }
    }
    
    // 添加当前用户问题
    {
        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = question["content"].toString();
        initialMessages.append(userMessage);
    }
    
    return initialMessages;
}

QPair<int, QString> CommanderAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName != "transfer_to_agent")
        return qMakePair(-1, QString("Unknown tool: %1").arg(toolName));

    QString agentName = params["agent_name"].toString();
    QString content = params["content"].toString();

    if (agentName.isEmpty()) {
        return qMakePair(-1, QString("Error: Sub-agent name not specified"));
    }

    if (content.isEmpty()) {
        return qMakePair(-1, QString("Error: Request content not specified"));
    }

    auto subAgent = m_subAgents.value(agentName);
    if (!subAgent) {
        return qMakePair(1, QString("Error: Sub-agent '%1' not found").arg(agentName));
    }

    // 构建请求对象
    QJsonObject request;
    request["content"] = content;

    // 调用子智能体
    QJsonObject result = subAgent->processRequest(request, {}, {});

    if (subAgent->lastError() != AIServer::NoError)
        return qMakePair(-1, subAgent->lastErrorString());

    if (result.contains("error")) {
        return qMakePair(-1, result["error"].toString());
    }

    QString response = result.value("content").toString();
    if (response.isEmpty())
        return qMakePair(-1, QString("Error: No content received from sub-agent"));

    return qMakePair(0, response);

}

QJsonObject CommanderAgent::subagentTool() const
{
    if (m_subAgents.isEmpty()) {
        return QJsonObject();
    }
    
    // 构建agent_name的enum数组
    QJsonArray enumArray;
    for (auto it = m_subAgents.begin(); it != m_subAgents.end(); ++it) {
        enumArray.append(it.key());
    }
    
    // 构建完整的JSON对象
    QJsonObject toolObj;
    toolObj["name"] = "transfer_to_agent";
    toolObj["description"] = "Transfer the current task to a specified sub-agent for execution";
    
    QJsonObject parameters;
    parameters["type"] = "object";
    
    QJsonArray required;
    required.append("agent_name");
    required.append("content");
    parameters["required"] = required;
    
    QJsonObject properties;
    
    QJsonObject agentNameProp;
    agentNameProp["type"] = "string";
    agentNameProp["description"] = "The name of the sub-agent who received the transferred task.";
    //agentNameProp["enum"] = enumArray;
    properties["agent_name"] = agentNameProp;
    
    QJsonObject contentProp;
    contentProp["type"] = "string";
    contentProp["description"] = "Detailed task content to be assigned to the sub-agent.";
    properties["content"] = contentProp;
    
    QJsonObject explProp;
    explProp["type"] = "string";
    explProp["description"] = "Explain to user what you are doing. But NEVER refer to tool and agent names.";
    properties["explanation"] = explProp;

    parameters["properties"] = properties;
    toolObj["parameters"] = parameters;
    
    return toolObj;
}

QString CommanderAgent::subagentPrompt() const
{
    QString agentsXml;

    for (auto it = m_subAgents.begin(); it != m_subAgents.end(); ++it) {
        QString agentName = it.key();
        QString agentDescription = it.value()->description();

        agentsXml += QString("  - agent_name\n    %1\n")
                        .arg(agentName);
        agentsXml += QString("  - description\n    %1\n")
                        .arg(agentDescription);
    }

    QString tmpl = R"(
You have the following sub-agents available to transfer task. Please transfer tasks precisely based on the descriptions of the sub-agents.
- sub_agents
%0
)";

    return tmpl.arg(agentsXml);
}
} // namespace uos_ai 
