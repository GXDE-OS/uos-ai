#ifndef WRITER_POINT_H
#define WRITER_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class WriterPoint : public BasicPoint
{
public:
    explicit WriterPoint() : BasicPoint()
    {
        this->m_eventId.second = EventID::WRITER;
        this->m_event = "writer";
    }
    ~WriterPoint() {}
};

}

}

#endif // WRITER_POINT_H
