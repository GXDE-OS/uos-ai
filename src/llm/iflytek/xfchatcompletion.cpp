#include "xfchatcompletion.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

XFChatCompletion::XFChatCompletion(const AccountProxy &account, QObject *parent)
    : XFNetWork(account, parent)
{

}

QPair<int, QString> XFChatCompletion::create(int model, const QString &url, XFConversation &conversation, qreal temperature)
{
    QJsonObject data;
    data["header"] = header();
    data["parameter"] = parameter(model, temperature, url);

    QJsonObject payloadObj;
    payloadObj["message"] = payloadMessage(conversation);

    // 讯飞服务器目前存在BUG，先屏蔽
//    if (model >= LLMChatModel::SPARKDESK_3 && model < WXQF_ERNIE_Bot && !conversation.getFunctions().isEmpty()) {
//        payloadObj["functions"] = payloadFunctions(conversation.getFunctions());
//    }

    data["payload"] = payloadObj;

    QString path = url;
    if (path.isEmpty())
        path = "wss://spark-api.xf-yun.com/" + version(model) + "/chat";

    qInfo() << "xunfei request url:" << path;
    const QPair<int, QByteArray> &resultPairs = wssRequest(QJsonDocument(data).toJson(QJsonDocument::Compact), path);

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    // 星火这很特殊，聊天包含违规的语言，返回错误，但是内容正常，这里先看返回结果，不看错误码
    QPair<int, QString> codepair = conversation.update(resultPairs.second);
    return codepair;
}
