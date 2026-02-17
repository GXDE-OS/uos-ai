#include "basicpoint.h"
#include <QLoggingCategory>
#include <QDateTime>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

UOSAI_USE_NAMESPACE
using namespace report;

BasicPoint::BasicPoint()
    : m_eventId(QPair<QString, EventID>("tid", EventID::Default))
{
}

BasicPoint::~BasicPoint()
{

}

QVariantMap BasicPoint::assemblingData() const
{
    qCDebug(logAIBar) << "Assembling report data for event:" << m_eventId.second;
    QVariantMap map;
    map.insert(m_eventId.first, m_eventId.second);
    map.insert("event", m_event);
    map.insert("app", "uos-ai");
    map.insert("app_version", APP_VERSION);
    QDateTime curTime = QDateTime::currentDateTime();
    map.insert("time", curTime.toString("yyyy-MM-dd HH:mm:ss.zzz"));
    map.insert("timestamp", QString::number(curTime.toMSecsSinceEpoch()));

    if (!m_additionalData.isEmpty()) {
        qCDebug(logAIBar) << "Adding additional data fields:" << m_additionalData.keys();
        QJsonObject msgJson;
        for (auto it = m_additionalData.begin(); it != m_additionalData.end(); ++it) {
            msgJson.insert(it.key(), it.value().toJsonValue());
        }
        map.insert("message", msgJson);
    }

    qCDebug(logAIBar) << "Report data assembled successfully";
    return map;
}

void BasicPoint::setAdditionalData(const QVariantMap &data)
{
    qCDebug(logAIBar) << "Setting additional data with" << data.size() << "items";
    m_additionalData = data;
}
