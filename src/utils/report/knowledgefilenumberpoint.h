#ifndef KNOWLEDGE_FILE_NUMBER_POINT_H
#define KNOWLEDGE_FILE_NUMBER_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class KnowledgeFileNumberPoint : public BasicPoint
{
public:
    explicit KnowledgeFileNumberPoint(int number) : BasicPoint()
    {
        this->m_eventId.second = EventID::KNOWLEDGE_FILE_NUMBER;
        this->m_event = "knowledge_file_number";
        QVariantMap map;
        map.insert("knowledge_file_number", number);
        this->setAdditionalData(map);
    }
    ~KnowledgeFileNumberPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // KNOWLEDGE_FILE_NUMBER_POINT_H
