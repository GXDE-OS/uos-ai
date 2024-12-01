#include "wxqf.h"
#include "wxqfconversation.h"
#include "wxqfchatcompletion.h"
#include "wxqftext2image.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

WXQFAI::WXQFAI(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{
    m_deltaConversion.reset(new WXQFConversation);
}

QJsonObject WXQFAI::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    WXQFConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    WXQFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &chatCompletion, &WXQFChatCompletion::requestAborted);
    connect(&chatCompletion, &WXQFChatCompletion::readyReadDeltaContent, this, &WXQFAI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, temperature);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> WXQFAI::text2Image(const QString &prompt, int number)
{
    WXQFText2Image textToImage(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &textToImage, &WXQFText2Image::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return imageData;
}

QPair<int, QString> WXQFAI::verify()
{
    WXQFConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    WXQFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &chatCompletion, &WXQFChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

void WXQFAI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;

    QJsonObject deltacontent = m_deltaConversion->parseContentString(content);

    if (deltacontent.contains("content"))
        emit readyReadChatDeltaContent(deltacontent.value("content").toString());
}
