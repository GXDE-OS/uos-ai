#ifndef CHATWINDOW_START_POINT_H
#define CHATWINDOW_START_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

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

UOSAI_END_NAMESPACE

#endif // CHATWINDOW_START_POINT_H
