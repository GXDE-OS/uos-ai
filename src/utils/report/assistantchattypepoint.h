#ifndef ASSISTANT_CHAT_TYPE_POINT_H
#define ASSISTANT_CHAT_TYPE_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

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

UOSAI_END_NAMESPACE

#endif // ASSISTANT_CHAT_TYPE_POINT_H
