#include "commanderagent.h"
#include "conversation/messagenode.h"
#include "global_key_define.h"
#include "model/modeltool.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QLoggingCategory>
#include <QStringList>

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
    m_tools += subagentTool();
    return true;
}

QString CommanderAgent::systemPrompt() const
{
    return m_systemPrompt + subagentPrompt();
}

void CommanderAgent::setModel(QSharedPointer<AbstractChatModel> llm)
{
    LlmAgent::setModel(llm);

    for (QSharedPointer<LlmAgent> agent : m_subAgents.values()) {
        agent->setModel(llm);
    }
}

void CommanderAgent::cancel()
{
    LlmAgent::cancel();

    for (QSharedPointer<LlmAgent> agent : m_subAgents.values())
        agent->cancel();
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

    ModelMessage request;
    request.role = "user";
    request.content = {{ContentType::CntText, content}};

    QVariantHash result = subAgent->processRequest(request, {}, {});

    if (!subAgent->lastError().isEmpty())
        return qMakePair(-1, subAgent->lastError().value(STR_KEY_MESSAGE).toString());

    if (result.contains("error")) {
        return qMakePair(-1, result["error"].toString());
    }

    QString response = result.value("content").toString();
    if (response.isEmpty())
        return qMakePair(-1, QString("Error: No content received from sub-agent"));

    return qMakePair(0, response);

}

ModelToolList CommanderAgent::subagentTool() const
{
    if (m_subAgents.isEmpty()) {
        return ModelToolList();
    }

    ModelToolList toolList;
    ModelTool tool;

    tool.name = "transfer_to_agent";
    tool.description = "Transfer the current task to a specified sub-agent for execution";

    ModelToolProperty agentNameProp;
    agentNameProp.name = "agent_name";
    agentNameProp.type = "string";
    agentNameProp.description = "The name of the sub-agent who received the transferred task.";
    tool.properties.append(agentNameProp);

    ModelToolProperty contentProp;
    contentProp.name = "content";
    contentProp.type = "string";
    contentProp.description = "Detailed task content to be assigned to the sub-agent.";
    tool.properties.append(contentProp);

    ModelToolProperty explProp;
    explProp.name = "explanation";
    explProp.type = "string";
    explProp.description = "Explain to user what you are doing. But NEVER refer to tool and agent names.";
    tool.properties.append(explProp);

    tool.required = QStringList{"agent_name", "content"};

    toolList.append(tool);
    return toolList;
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
