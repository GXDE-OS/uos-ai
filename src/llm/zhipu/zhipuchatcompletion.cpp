#include "zhipuchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)
ZhiPuChatCompletion::ZhiPuChatCompletion(const AccountProxy &account)
    : ZhiPuNetWork(account)
{

}

QPair<int, QString> ZhiPuChatCompletion::create(int mode, ZhiPuConversation &conversation, const QVariantHash &params)
{
    qCDebug(logLLM) << "Creating ZhiPu chat completion with mode:" << mode;
    
    QJsonObject dataObject;
    dataObject.insert("prompt", conversation.getConversions());
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.0, params.value("temperature").toDouble(), 1.0));
    dataObject.insert("incremental", true);

    QString path = "/chatglm_turbo/sse-invoke";
    const QPair<int, QByteArray> &resultPairs = request(dataObject, path);

    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "ZhiPu Request failed with error:" << resultPairs.first 
                         << "Message:" << resultPairs.second;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    return conversation.update(resultPairs.second);
}
