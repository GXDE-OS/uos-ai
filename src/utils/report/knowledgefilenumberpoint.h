#ifndef KNOWLEDGE_FILE_NUMBER_POINT_H
#define KNOWLEDGE_FILE_NUMBER_POINT_H

#include "basicpoint.h"

namespace uos_ai {

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

}

#endif // KNOWLEDGE_FILE_NUMBER_POINT_H
