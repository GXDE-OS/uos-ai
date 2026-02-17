#ifndef SCREENSHOT_CLICKED_POINT_H
#define SCREENSHOT_CLICKED_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class ScreenShotClickedPoint : public BasicPoint
{
public:
    explicit ScreenShotClickedPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::SCREENSHOT_CLICKED;
        this->m_event = "screenshot_clicked";
    }
    ~ScreenShotClickedPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // SCREENSHOT_CLICKED_POINT_H
