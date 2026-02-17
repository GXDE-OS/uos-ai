#ifndef MCP_CHAT_POINT_H
#define MCP_CHAT_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class MCPChatPoint : public BasicPoint
{
public:
    explicit MCPChatPoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::MCP_CHAT;
        this->m_event = "mcp_chat";
        QVariantMap map;
        map.insert("MCP_type", type);
        this->setAdditionalData(map);
    }
    ~MCPChatPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // MCP_CHAT_POINT_H
