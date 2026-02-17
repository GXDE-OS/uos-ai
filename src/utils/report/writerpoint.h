#ifndef WRITER_POINT_H
#define WRITER_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

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

UOSAI_END_NAMESPACE

#endif // WRITER_POINT_H
