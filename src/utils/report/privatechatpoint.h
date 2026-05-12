#ifndef PRIVATE_CHAT_POINT_H
#define PRIVATE_CHAT_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class PrivateChatPoint : public BasicPoint
{
public:
    explicit PrivateChatPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::PRIVATE_CHAT;
        this->m_event = "private_chat";
    }
    ~PrivateChatPoint() {}
};

}

}

#endif // PRIVATE_CHAT_POINT_H
