#ifndef FUNCTIONCALL_POINT_H
#define FUNCTIONCALL_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class FunctioncallPoint : public BasicPoint
{
public:
    explicit FunctioncallPoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::FUNCTIONCALL;
        this->m_event = "functioncall";

        QVariantMap map;
        map.insert("is_functioncall", true);
        map.insert("functioncall_type", type);
        this->setAdditionalData(map);
    }
    ~FunctioncallPoint() {}
};

}

}

#endif // FUNCTIONCALL_POINT_H
