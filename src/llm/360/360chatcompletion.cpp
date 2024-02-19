#include "360chatcompletion.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ChatCompletion360::ChatCompletion360(const AccountProxy &account)
    : NetWork360(account)
{

}

QPair<int, QString> ChatCompletion360::create(const QString &model, Conversation360 &conversation, qreal temperature)
{
    QJsonObject dataObject;
    dataObject.insert("model", model);
    dataObject.insert("messages", conversation.getConversions());
    dataObject.insert("temperature", qBound(0.0, temperature, 1.0));
    dataObject.insert("stream", true);

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/chat/completions");

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
