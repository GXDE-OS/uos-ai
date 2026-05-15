#include "reportchannel.h"
#include "utils/report/eventlogutil.h"

// Report point headers from src/utils/report
#include "utils/report/screenshotclickedpoint.h"
#include "utils/report/knowledgefiletypepoint.h"
#include "utils/report/knowledgefunctionpoint.h"
#include "utils/report/followalongpoint.h"
#include "utils/report/mcpchatpoint.h"
#include "utils/report/writerpoint.h"
#include "utils/report/followfunctionpoint.h"
#include "utils/report/knowledgefilenumberpoint.h"
#include "utils/report/assistantchattypepoint.h"
#include "utils/report/privatechatclickedpoint.h"
#include "utils/report/functioncallpoint.h"
#include "utils/report/chatwindowstartpoint.h"
#include "utils/report/privatechatpoint.h"
#include "utils/report/modelpoint.h"
#include "utils/report/assistantchatpoint.h"
#include "utils/report/chatwindowpoint.h"
#include "utils/report/digitalchatpoint.h"
#include "utils/report/writerfunctionpoint.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QDebug>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

ReportChannel::ReportChannel(QObject *parent)
    : QObject(parent)
{
    qCDebug(logAIGUI) << "ReportChannel initialized";
}

ReportChannel::~ReportChannel()
{
}

void ReportChannel::writeReportEvent(const QString &jsonData)
{
    qCDebug(logAIGUI) << "writeReportEvent called with:" << jsonData;

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "Failed to parse report JSON:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qCWarning(logAIGUI) << "Report JSON must be an object";
        return;
    }

    QJsonObject obj = doc.object();

    // Extract type (now expecting numeric value)
    if (!obj.contains("type")) {
        qCWarning(logAIGUI) << "Report JSON missing required 'type' field";
        return;
    }

    int type = obj["type"].toInt();

    // Extract optional params
    QVariant params;
    if (obj.contains("params")) {
        params = obj["params"].toVariant();
    }

    // Create and write the report point
    QVariantMap reportData = createReportPoint(type, params);

    if (!reportData.isEmpty()) {
        ReportIns()->writeEvent(reportData);
        qCDebug(logAIGUI) << "Report event written successfully for type:" << type;
    }
}

QVariantMap ReportChannel::createReportPoint(int type, const QVariant &params)
{
    if (type == DigitalChatPoint) {
        return report::DigitalChatPoint().assemblingData();
    }

    if (type == PrivateChatClickedPoint) {
        return report::PrivateChatClickedPoint().assemblingData();
    }

    if (type == PrivateChatPoint) {
        return report::PrivateChatPoint().assemblingData();
    }

    // Special types with object parameters
    if (type == ModelPoint) {
        // ModelPoint accepts a JSON object with model_species and model_type
        QString jsonString;
        if (params.canConvert<QVariantMap>()) {
            // If params is already a map/object, convert it to JSON string
            QJsonObject jsonObj = QJsonObject::fromVariantMap(params.toMap());
            QJsonDocument doc(jsonObj);
            jsonString = doc.toJson(QJsonDocument::Compact);
        } else {
            // If params is already a string, use it directly
            jsonString = params.toString();
        }

        return report::ModelPoint(jsonString).assemblingData();
    }

    if (type == AssistantChatPoint) {
        // AssistantChatPoint accepts a JSON object with assistant_type
        QString jsonString;
        if (params.canConvert<QVariantMap>()) {
            // If params is already a map/object, convert it to JSON string
            QJsonObject jsonObj = QJsonObject::fromVariantMap(params.toMap());
            QJsonDocument doc(jsonObj);
            jsonString = doc.toJson(QJsonDocument::Compact);
        } else {
            // If params is already a string, use it directly
            jsonString = params.toString();
        }

        return report::AssistantChatPoint(jsonString).assemblingData();
    }

    // Unknown type
    qCWarning(logAIGUI) << "Unknown report type:" << type;
    return QVariantMap();
}
