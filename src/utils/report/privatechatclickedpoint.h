#ifndef PRIVATE_CHAT_CLICKED_POINT_H
#define PRIVATE_CHAT_CLICKED_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class PrivateChatClickedPoint : public BasicPoint
{
public:
    explicit PrivateChatClickedPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::PRIVATE_CHAT_CLICKED;
        this->m_event = "private_chat_clicked";
    }
    ~PrivateChatClickedPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // PRIVATE_CHAT_CLICKED_POINT_H
