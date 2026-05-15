#ifndef MODEL_POINT_H
#define MODEL_POINT_H

#include "basicpoint.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace uos_ai {

namespace report {

class ModelPoint : public BasicPoint
{
public:
    explicit ModelPoint(const QString &jsonData) : BasicPoint()
    {
        QString species = "";
        QString type = "";

        // Parse JSON to extract model_species and model_type
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            species = obj.value("model_species").toString();
            type = obj.value("model_type").toString();
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

}

#endif // MODEL_POINT_H
