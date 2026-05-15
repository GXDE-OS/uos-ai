#ifndef KNOWLEDGE_FUNCTION_POINT_H
#define KNOWLEDGE_FUNCTION_POINT_H

#include "basicpoint.h"

namespace uos_ai {

namespace report {

class KnowledgeFunctionPoint : public BasicPoint
{
public:
    explicit KnowledgeFunctionPoint(const QString &function) : BasicPoint()
    {
        this->m_eventId.second = EventID::KNOWLEDGE_FUNCTION;
        this->m_event = "knowledge_function";
        QVariantMap map;
        map.insert("knowledge_function", function);
        this->setAdditionalData(map);
    }
    ~KnowledgeFunctionPoint() {}
};

}

}

#endif // KNOWLEDGE_FUNCTION_POINT_H
