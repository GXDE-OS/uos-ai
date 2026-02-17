#ifndef DEFAULTMCPSERVER_H
#define DEFAULTMCPSERVER_H

#include "mcpserver.h"

namespace uos_ai {

class DefaultMcpServer : public MCPServer
{
public:
    explicit DefaultMcpServer(const QString &agentName, QObject *parent = nullptr);

    void scanServers() override;

    bool isBuiltin(const QString &name) const override;
};

}


#endif // DEFAULTMCPSERVER_H
