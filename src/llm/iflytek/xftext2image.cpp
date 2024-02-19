#include "xftext2image.h"

#include <QUuid>

XFText2Image::XFText2Image(const AccountProxy &account)
    : XFNetWork(account)
{

}

QPair<int, QString> XFText2Image::create(int model, XFConversation &conversation, int number)
{
    QJsonObject data;
    data["header"] = header();
    data["parameter"] = parameter(LLMChatModel::SPARKDESK, 0.5);

    QJsonObject payloadObj;
    payloadObj["message"] = payloadMessage(conversation);
    data["payload"] = payloadObj;

    QString path = "https://spark-api.cn-huabei-1.xf-yun.com/v2.1/tti";
    const QPair<int, QByteArray> &resultPairs = httpRequest(data, path);

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    // 星火这很特殊，聊天包含违规的语言，返回错误，但是内容正常，这里先看返回结果，不看错误码
    QPair<int, QString> codepair = conversation.update(resultPairs.second);
    return codepair;
}
