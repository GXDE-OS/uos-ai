#include "zhipuchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ZhiPuChatCompletion::ZhiPuChatCompletion(const AccountProxy &account)
    : ZhiPuNetWork(account)
{

}

QPair<int, QString> ZhiPuChatCompletion::create(int mode, ZhiPuConversation &conversation, qreal temperature)
{
    QJsonObject dataObject;
    dataObject.insert("prompt", conversation.getConversions());
    dataObject.insert("temperature", qBound(0.0, temperature, 1.0));
    dataObject.insert("incremental", true);

    QString path = "/chatglm_turbo/sse-invoke";
    const QPair<int, QByteArray> &resultPairs = request(dataObject, path);

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    return conversation.update(resultPairs.second);
}
