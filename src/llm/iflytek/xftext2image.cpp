#include "xftext2image.h"
#include <QUuid>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

XFText2Image::XFText2Image(const AccountProxy &account)
    : XFNetWork(account)
{

}

QPair<int, QString> XFText2Image::create(int model, XFConversation &conversation, int number)
{
    qCDebug(logLLM) << "XFText2Image Creating text-to-image request with model:" << model << "and image count:" << number;

    QJsonObject data;
    data["header"] = header();
    data["parameter"] = parameter(LLMChatModel::SPARKDESK, 0.5);

    QJsonObject payloadObj;
    payloadObj["message"] = payloadMessage(conversation);
    data["payload"] = payloadObj;

    QString path = "https://spark-api.cn-huabei-1.xf-yun.com/v2.1/tti";
    qCDebug(logLLM) << "XFText2Image Sending request to:" << path;
    const QPair<int, QByteArray> &resultPairs = httpRequest(data, path);

    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "Text-to-image request failed with error code:" << resultPairs.first;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    // 星火这很特殊，聊天包含违规的语言，返回错误，但是内容正常，这里先看返回结果，不看错误码
    QPair<int, QString> codepair = conversation.update(resultPairs.second);
    if (codepair.first != 0)
        qCWarning(logLLM) << "XFText2Image Failed to update conversation with image response:" << codepair.second;

    return codepair;
}
