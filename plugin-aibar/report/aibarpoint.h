#ifndef AI_BAR_POINT_H
#define AI_BAR_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class AiBarPoint : public BasicPoint
{
public:
    explicit AiBarPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::AI_BAR;
        this->m_event = "ai_bar";
    }
    ~AiBarPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // AI_BAR_POINT_H
