#ifndef DIGITAL_CHAT_POINT_H
#define DIGITAL_CHAT_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class DigitalChatPoint : public BasicPoint
{
public:
    explicit DigitalChatPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::DIGITAL_CHAT;
        this->m_event = "digital_chat";
    }
    ~DigitalChatPoint() {}
};

}

}

#endif // DIGITAL_CHAT_POINT_H
