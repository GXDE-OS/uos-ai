#include "wxqfchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

WXQFChatCompletion::WXQFChatCompletion(const AccountProxy &account)
    : WXQFNetWork(account)
{

}

QPair<int, QString> WXQFChatCompletion::create(int model, WXQFConversation &conversation, qreal temperature)
{
    QJsonObject dataObject;
    QJsonArray conversions = conversation.getConversions();
    if (!conversation.getFunctions().isEmpty()) {
        dataObject.insert("functions", conversation.getFunctions());
    }

    dataObject.insert("messages", conversions);
    dataObject.insert("temperature", qBound(0.0, temperature, 1.0));
    dataObject.insert("stream", true);

    QString path;
    if (model == WXQF_ERNIE_Bot_turbo) {
        path = "/chat/eb-instant";
    }  else if (model == WXQF_ERNIE_Bot_4) {
        path = "/chat/completions_pro";
    } else {
        path = "/chat/ernie_bot_8k";
    }

    const QString rootPath = "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop";
    const QPair<int, QByteArray> &resultPairs = request(dataObject, rootPath + path);

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
