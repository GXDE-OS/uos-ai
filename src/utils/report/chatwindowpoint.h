#ifndef CHATWINDOW_POINT_H
#define CHATWINDOW_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class ChatwindowPoint : public BasicPoint
{
public:
    explicit ChatwindowPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::CHATWINDOW;
        this->m_event = "chatwindow";
    }
    ~ChatwindowPoint() {}
};

}

}

#endif // CHATWINDOW_POINT_H
