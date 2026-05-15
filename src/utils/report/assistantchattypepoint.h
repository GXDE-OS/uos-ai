#ifndef ASSISTANT_CHAT_TYPE_POINT_H
#define ASSISTANT_CHAT_TYPE_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class AssistantChatTypePoint : public BasicPoint
{
public:
    explicit AssistantChatTypePoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::ASSISTANT_CHAT_TYPE;
        this->m_event = "assistant_chat_type";
        QVariantMap map;
        map.insert("message_type", type);
        this->setAdditionalData(map);
    }
    ~AssistantChatTypePoint() {}
};

}

}

#endif // ASSISTANT_CHAT_TYPE_POINT_H
