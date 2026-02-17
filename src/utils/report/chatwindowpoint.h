#ifndef CHATWINDOW_POINT_H
#define CHATWINDOW_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

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

UOSAI_END_NAMESPACE

#endif // CHATWINDOW_POINT_H
