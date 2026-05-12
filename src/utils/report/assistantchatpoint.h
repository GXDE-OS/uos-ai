#ifndef ASSISTANT_CHAT_POINT_H
#define ASSISTANT_CHAT_POINT_H

#include "basicpoint.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace uos_ai {

namespace report {

class AssistantChatPoint : public BasicPoint
{
public:
    explicit AssistantChatPoint(const QString &jsonData) : BasicPoint()
    {
        this->m_eventId.second = EventID::ASSISTANT_CHAT;
        this->m_event = "assistant_chat";

        QString assistantType = "";

        // Parse JSON to extract assistant_type
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            assistantType = obj.value("assistant_type").toString();
        }

        QVariantMap map;
        map.insert("assistant_type", assistantType);
        this->setAdditionalData(map);
    }
    ~AssistantChatPoint() {}
};

}

}

#endif // ASSISTANT_CHAT_POINT_H
