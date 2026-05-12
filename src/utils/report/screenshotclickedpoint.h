#ifndef SCREENSHOT_CLICKED_POINT_H
#define SCREENSHOT_CLICKED_POINT_H

#include "basicpoint.h"

namespace uos_ai {

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

}

#endif // SCREENSHOT_CLICKED_POINT_H
