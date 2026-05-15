#ifndef FOLLOWALONG_POINT_H
#define FOLLOWALONG_POINT_H

#include "basicpoint.h"

namespace uos_ai {

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

}

#endif // FOLLOWALONG_POINT_H
