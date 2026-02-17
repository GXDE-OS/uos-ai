#include "mcpagent.h"
#include "mcpclient.h"
#include "mcpserver.h"
#include "networkdefs.h"

#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonArray>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

MCPAgent::MCPAgent(QObject *parent)
    : LlmAgent(parent)
    , m_mcpClient(nullptr)
{
}

MCPAgent::~MCPAgent()
{
    if (m_mcpClient) {
        delete m_mcpClient;
        m_mcpClient = nullptr;
    }
}

QSharedPointer<MCPServer> MCPAgent::mcpServer() const
{
    return QSharedPointer<MCPServer>(new MCPServer(m_name));
}

bool MCPAgent::initialize()
{
    if (!m_mcpClient) {
        m_mcpClient = new McpClient(this);
    }
    
    if (!m_mcpClient->init()) {
        qCWarning(logAgent) << "Failed to initialize MCP client for agent:" << name();
        return false;
    }
    
    return true;
}

QStringList MCPAgent::listServers() const
{
    if (!m_mcpClient) {
        qCWarning(logAgent) << "MCP client not initialized for agent:" << name();
        return QStringList();
    }
    
    return syncCall<QStringList>([this]() {
        qCDebug(logAgent) << "list servers in" << QThread::currentThreadId();
        return m_mcpClient->listServers(m_name);
    });
}

QPair<int, QJsonValue> MCPAgent::listTools() const
{   
    return syncCall<QPair<int, QJsonValue>>([this]() {
        qCDebug(logAgent) << "list tools in" << QThread::currentThreadId();
        return m_mcpClient->getTools(m_name);
    });
}

QPair<int, QString> MCPAgent::fetchTools(const QStringList &servers)
{
    m_tools = QJsonArray();
    qCDebug(logAgent) << "Fetching tools from servers:" << servers;

    QPair<int, QJsonValue> srv = syncCall<QPair<int, QJsonValue>>([this, servers]() {
        qCDebug(logAgent) << "query servers in" << QThread::currentThreadId();
        return m_mcpClient->queryServers(m_name, servers);
    });

    if (srv.first != 0)
        return qMakePair(srv.first, QString());

    QJsonArray toolsArray;
    QString error;
    QStringList invaildSrv = servers;
    for (const QJsonValue &serverValue : srv.second.toArray()) {
        if (!serverValue.isObject()) {
            continue;
        }

        QJsonObject serverObj = serverValue.toObject();
        QString name = serverObj.value("name").toString();

        if (name.isEmpty() || (!servers.isEmpty() && !servers.contains(name)))
            continue;

        invaildSrv.removeOne(name);
        if (serverObj.contains("error")) {
            QString er = serverObj.value("error").toString();
            error.append(QString("%0: %1\n\n").arg(name).arg(er));
            continue;
        }

        if (serverObj.contains("tools") && serverObj["tools"].isArray()) {
            QJsonArray serverTools = serverObj["tools"].toArray();
            for (const QJsonValue &toolValue : serverTools) {
                toolsArray.append(toolValue);
            }
        }
    }

    for (const QString &name : invaildSrv)
         error.append(QString("%0: %1\n\n").arg(name).arg("No such server."));

    if (!error.isEmpty()) {
        error = tr("MCP server is not available") + QString("\n\n") + error;
        qCWarning(logAgent) << error;
        // 继续执行。
        //return qMakePair(AIServer::ErrorType::MCPSeverUnavailable, error);
    }

    m_tools = std::move(toolsArray);
    return qMakePair(AIServer::ErrorType::NoError, QString());
}

bool MCPAgent::syncServers() const
{
    auto ret = syncCall<QPair<int, QJsonObject>>([this]() {
        qCDebug(logAgent) << "sync servers in" << QThread::currentThreadId();
        return m_mcpClient->syncServers(m_name);
    });

    if (ret.first == 0) {
        auto details = ret.second.value("details").toObject();
        if (!details.isEmpty())
            qCInfo(logAgent) << "Synced servers" << details;
    } else {
        qCWarning(logAgent) << "Failed to sync severs:" << ret.second;
    }

    return ret.first == 0;
}

QJsonObject MCPAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    QJsonObject response;
    // 先刷新一次服务
    syncServers();

    // 取消
    if (canceled) {
        qCDebug(logAgent) << "agent canceled before fetch tools.";
        return response;
    }

    {
        auto toolRet = fetchTools(params.value(PREDICT_PARAM_MCPSERVERS).toStringList());
        if (toolRet.first != 0) {
            qCWarning(logAgent) << "Failed to fetch tools:" << toolRet.second;
            if (m_llm) {
                m_llm->setLastError(toolRet.first);
                m_llm->setLastErrorString(toolRet.second);
            }
            return response;
        }
    }

    // 取消
    if (canceled){
        qCDebug(logAgent) << "agent canceled before init messages.";
        return response;
    }

    return LlmAgent::processRequest(question, history, params);
}

QPair<int, QString> MCPAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    auto result = syncCall<QPair<int, QString>>([this, toolName, params]() {
        qCDebug(logAgent) << "call tool in" << QThread::currentThreadId();
        return m_mcpClient->callTool(m_name, toolName, params);
    });

    if ((result.first != AIServer::ErrorType::NoError)
            && (result.first != AIServer::ErrorType::MCPToolError))
        result.first = -1;

    return result;
}

} // namespace uos_ai 
