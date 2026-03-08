#include "defaultmcpserver.h"
#include "dbwrapper.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)
using namespace uos_ai;

DefaultMcpServer::DefaultMcpServer(const QString &agentName, QObject *parent) : MCPServer(agentName, parent)
{

}

void DefaultMcpServer::scanServers()
{
    MCPServer::scanServers();

    // add fixed mcp server name.
    QVariantHash info;
    QVariantHash des;
    des.insert("generic", QObject::tr("System AI Intelligent ButlerFunction\nIntroduction: Intelligently schedules system tools to perform complex system operations, supporting system control, file management, application management, and networking protocols.\nPrompt Example: Name all files in the newly created folder on the desktop as project materials and arrange them in numerical order."));
    info.insert("descriptions", des);
    m_servers.insert("uos-mcp", info);
    return;
}

bool DefaultMcpServer::isBuiltin(const QString &name) const
{
    if (name.compare("uos-mcp", Qt::CaseInsensitive) == 0)
        return true;

    QString path = m_servers.value(name).value("uosai-file-path").toString();
    if (path.isEmpty())
        return false;

    return path.endsWith("uosai-builtin-mcp.json", Qt::CaseInsensitive);
}
