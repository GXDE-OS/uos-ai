#include "wxqf.h"
#include "wxqfconversation.h"
#include "wxqfchatcompletion.h"
#include "wxqftext2image.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

WXQFAI::WXQFAI(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{
    m_deltaConversion.reset(new WXQFConversation);
}

QJsonObject WXQFAI::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "WXQFAI Starting prediction with content length:" << content.length()
                         << "and" << functions.size() << "functions";
    
    WXQFConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    WXQFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &chatCompletion, &WXQFChatCompletion::requestAborted);
    connect(&chatCompletion, &WXQFChatCompletion::readyReadDeltaContent, this, &WXQFAI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0)
        qCWarning(logLLM) << "WXQFAI Prediction failed with error:" << errorpair.first << errorpair.second;

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "WXQFAI Prediction returned tools response";
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> WXQFAI::text2Image(const QString &prompt, int number)
{
    qCInfo(logLLM) << "WXQFAI Starting text2Image with prompt length:" << prompt.length() << "and number:" << number;
    
    WXQFText2Image textToImage(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &textToImage, &WXQFText2Image::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "WXQFAI text2Image failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "WXQFAI text2Image completed successfully, generated" << imageData.size() << "images";
    }

    return imageData;
}

QPair<int, QString> WXQFAI::verify()
{
    qCDebug(logLLM) << "WXQFAI Starting account verification";
    
    WXQFConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    WXQFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &WXQFAI::aborted, &chatCompletion, &WXQFChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "WXQFAI Account verification failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "WXQFAI Account verification completed successfully";
    }

    return errorpair;
}

void WXQFAI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    QJsonObject deltacontent = m_deltaConversion->parseContentString(content);
    if (deltacontent.contains("content")) {
        textChainContent(deltacontent.value("content").toString());
    }
}
