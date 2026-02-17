#ifndef FOLLOWALONG_POINT_H
#define FOLLOWALONG_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class FollowalongPoint : public BasicPoint
{
public:
    explicit FollowalongPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::FOLLOWALONG;
        this->m_event = "followalong";
    }
    ~FollowalongPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // FOLLOWALONG_POINT_H
