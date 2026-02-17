#ifndef ASSISTANT_CHAT_POINT_H
#define ASSISTANT_CHAT_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class AssistantChatPoint : public BasicPoint
{
public:
    explicit AssistantChatPoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::ASSISTANT_CHAT;
        this->m_event = "assistant_chat";

        QVariantMap map;
        map.insert("assistant_type", type);
        this->setAdditionalData(map);
    }
    ~AssistantChatPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // ASSISTANT_CHAT_POINT_H
