#include "sparkdesk.h"
#include "xfconversation.h"
#include "xfchatcompletion.h"
#include "xftext2image.h"

SparkDesk::SparkDesk(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject SparkDesk::predict(const QString &conversation, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    Q_UNUSED(systemRole);

    XFConversation conversion;
    conversion.addUserData(conversation);
    conversion.setFunctions(functions);

    XFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &chatCompletion, &XFChatCompletion::requestAborted);
    connect(&chatCompletion, &XFChatCompletion::readyReadDeltaContent, this, &SparkDesk::onReadyReadChatDeltaContent);

    QPair<int, QString> errorPair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, temperature);
    setLastError(errorPair.first);
    setLastErrorString(errorPair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> SparkDesk::text2Image(const QString &prompt, int number)
{
    XFText2Image textToImage(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &textToImage, &XFText2Image::requestAborted);

    XFConversation conversion;
    conversion.addUserData(prompt);

    QPair<int, QString> errorpair = textToImage.create(m_accountProxy.model, conversion, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QList<QByteArray> imageData;
    const QByteArray &data = conversion.getLastByteResponse();
    if (!data.isEmpty()) {
        imageData << QByteArray::fromBase64(data);
    }

    return imageData;
}

QPair<int, QString> SparkDesk::verify()
{
    XFConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    XFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &chatCompletion, &XFChatCompletion::requestAborted);

    QPair<int, QString> errorPair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion);
    setLastError(errorPair.first);
    setLastErrorString(errorPair.second);

    return errorPair;
}

void SparkDesk::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;

    QPair<int, QJsonObject> contentPair = XFConversation::parseContentString(content);

    if (contentPair.first == 0 && !contentPair.second.value("content").toString().isEmpty())
        emit readyReadChatDeltaContent(contentPair.second.value("content").toString());
}
