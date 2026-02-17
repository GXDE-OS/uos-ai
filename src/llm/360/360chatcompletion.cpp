#include "360chatcompletion.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"

#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

ChatCompletion360::ChatCompletion360(const AccountProxy &account)
    : NetWork360(account)
{

}

QPair<int, QString> ChatCompletion360::create(const QString &model, Conversation360 &conversation, const QVariantHash &params)
{
    qCDebug(logLLM) << "360 Creating chat completion with model:" << model
                   << "and temperature:" << params;

    QJsonObject dataObject;
    dataObject.insert("model", model);
    dataObject.insert("messages", conversation.getConversions());
    if (params.contains("temperature"))
         dataObject.insert("temperature", qBound(0.0, params.value("temperature").toDouble(), 1.0));
    dataObject.insert("stream", true);

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/chat/completions");

    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "360 Chat completion request failed with error:" << resultPairs.first 
                         << "-" << resultPairs.second;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
