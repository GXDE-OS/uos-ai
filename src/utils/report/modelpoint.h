#ifndef MODEL_POINT_H
#define MODEL_POINT_H

#include "basicpoint.h"
#include "serverdefs.h"
UOSAI_BEGIN_NAMESPACE

namespace report {

class ModelPoint : public BasicPoint
{
public:
    explicit ModelPoint(const LLMServerProxy &tmpLLMAccount) : BasicPoint()
    {
        QString species = "Online model";
        QString type = tmpLLMAccount.llmName(tmpLLMAccount.model, true);
        switch (tmpLLMAccount.type) {
        case ModelType::USER:
            species = "Private deployment model";
            break;
        case ModelType::LOCAL:
            species = "Local model";
            break;
        case ModelType::FREE_KOL:
            species = "Online model";
            break;
        case ModelType::FREE_NORMAL:
            species = "Online model";
            break;
        default:
            break;
        }
        this->m_eventId.second = EventID::MODEL;
        this->m_event = "model";
        QVariantMap map;
        map.insert("model_species", species);
        map.insert("model_type", type);
        this->setAdditionalData(map);
    }
    ~ModelPoint() {}
};

}

UOSAI_END_NAMESPACE

#endif // MODEL_POINT_H
