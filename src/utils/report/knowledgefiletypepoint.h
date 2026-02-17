#ifndef KNOWLEDGE_FILE_TYPE_POINT_H
#define KNOWLEDGE_FILE_TYPE_POINT_H

#include "basicpoint.h"

UOSAI_BEGIN_NAMESPACE

namespace report {

class KnowledgeFileTypePoint : public BasicPoint
{
public:
    explicit KnowledgeFileTypePoint(const QString &type) : BasicPoint()
    {
        this->m_eventId.second = EventID::KNOWLEDGE_FILE_TYPE;
        this->m_event = "knowledge_file_type";
        QVariantMap map;
        map.insert("knowledge_file_type", type);
        this->setAdditionalData(map);
    }
    ~KnowledgeFileTypePoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // KNOWLEDGE_FILE_TYPE_POINT_H
