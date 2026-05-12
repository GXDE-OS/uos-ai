#ifndef CHATWINDOW_START_POINT_H
#define CHATWINDOW_START_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class ChatwindowStartPoint : public BasicPoint
{
public:
    explicit ChatwindowStartPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::CHATWINDOW_START;
        this->m_event = "chatwindow_start";
    }
    ~ChatwindowStartPoint() {}
};

}

}

#endif // CHATWINDOW_START_POINT_H
