#ifndef DIGITAL_CHAT_POINT_H
#define DIGITAL_CHAT_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

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

UOSAI_END_NAMESPACE

#endif // DIGITAL_CHAT_POINT_H
