#ifndef WRITERFUNCTION_POINT_H
#define WRITERFUNCTION_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class WriterFunctionPoint : public BasicPoint
{
public:
    explicit WriterFunctionPoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::WRITER_FUNCTION;
        this->m_event = "writer_function";

        QVariantMap map;
        map.insert("function", type);
        this->setAdditionalData(map);
    }
    ~WriterFunctionPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // WRITERFUNCTION_POINT_H
