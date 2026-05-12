#include "defaultagent.h"
#include "defaultmcpserver.h"
#include "global_define.h"
#include "conversation/messagenode.h"

#include <QJsonDocument>
#include <QDir>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

DefaultAgent::DefaultAgent(QObject *parent)
    : MCPAgent(parent)
{
    m_name = kDefaultAgentName;
    m_description = "A Default Agent for UOS AI with MCP server support";

    m_systemPrompt =
R"(You are UOS Claw, an intelligent AI assistant integrated into the Deepin Desktop Environment (DDE) on UOS (an operating system based on Debian Linux).

## Tool Calling
You have tools at your disposal to solve user's task. Follow these rules regarding tool calls:
1. ALWAYS follow the tool call schema exactly as specified and make sure to provide all necessary parameters.
2. The conversation may reference tools that are no longer available. NEVER call tools that are not explicitly provided.
3. Only calls tools when they are necessary. If the USER's task is general or you already know the answer, just respond without calling tools.
4. Before calling each tool, first explain to the USER why you are calling it. But NEVER refer to tool names.

## Uesr Info
The absolute path of the user's home dir is %1. Today is %2.
)";
}

QSharedPointer<MCPServer> DefaultAgent::mcpServer() const
{
    return QSharedPointer<MCPServer>(new DefaultMcpServer(m_name));
}

QString DefaultAgent::systemPrompt() const
{
    m_toolList.clear();

    for (const ModelTool &tool: m_tools) {
        m_toolList.insert(tool.name);
    }

    qCInfo(logAgent)  << "Available tools:" << m_toolList;
    return m_systemPrompt
           .arg(QDir::homePath())
           .arg(QDate::currentDate().toString("yyyy-MM-dd hh:mm ddd"));
}

QPair<int, QString> DefaultAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    QString strParams = QString::fromUtf8(QJsonDocument(params).toJson());
    QString id = GlobalUtil::generateMsId();
    int status = NormalStatus::NsRunning;
    emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status)});

    auto ret = MCPAgent::callTool(toolName, params);

    status = ret.first < 0 ? NormalStatus::NsFailed : NormalStatus::NsCompleted;
    emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status, ret.second)});

    return ret;
}
