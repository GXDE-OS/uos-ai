#include "aichatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

AIChatCompletion::AIChatCompletion(const AccountProxy &account)
    : AINetWork(account)
{

}

QPair<int, QString> AIChatCompletion::create(const QString &model, AIConversation &conversation, const QVariantHash &params)
{
    QJsonObject dataObject;
    dataObject.insert("model", model);
    dataObject.insert("messages", conversation.getConversions());
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.0, params.value("temperature").toDouble(), 2.0));
    dataObject.insert("stream", true);

    if (!conversation.getFunctions().isEmpty()) {
        dataObject.insert("functions", conversation.getFunctions());
    }

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/chat/completions");

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
