#include "sparkdesk.h"
#include "xfconversation.h"
#include "xfchatcompletion.h"
#include "xftext2image.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)
SparkDesk::SparkDesk(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject SparkDesk::predict(const QString &conversation, const QJsonArray &functions)
{
    qCInfo(logLLM) << "SparkDesk Starting prediction with conversation length:" << conversation.length()
                   << "and" << functions.size() << "functions";

    XFConversation conversion;
    conversion.addUserData(conversation);
    conversion.setFunctions(functions);

    XFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &chatCompletion, &XFChatCompletion::requestAborted);
    connect(&chatCompletion, &XFChatCompletion::readyReadDeltaContent, this, &SparkDesk::onReadyReadChatDeltaContent);

    QPair<int, QString> errorPair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, m_params);
    setLastError(errorPair.first);
    setLastErrorString(errorPair.second);

    if (errorPair.first != 0)
        qCWarning(logLLM) << "SparkDesk Prediction failed with error:" << errorPair.first << "-" << errorPair.second;
    
    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        response["tools"] = tools;
        qCInfo(logLLM) << "SparkDesk Prediction included tool calls";
    }
    return response;
}

QList<QByteArray> SparkDesk::text2Image(const QString &prompt, int number)
{
    qCInfo(logLLM) << "SparkDesk Starting text2Image with prompt length:" << prompt.length() 
                   << "and image count:" << number;

    XFText2Image textToImage(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &textToImage, &XFText2Image::requestAborted);

    XFConversation conversion;
    conversion.addUserData(prompt);

    QPair<int, QString> errorpair = textToImage.create(m_accountProxy.model, conversion, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "SparkDesk Text2Image failed with error:" << errorpair.first << "-" << errorpair.second;
    }

    QList<QByteArray> imageData;
    const QByteArray &data = conversion.getLastByteResponse();
    if (!data.isEmpty()) {
        imageData << data;
        qCDebug(logLLM) << "SparkDesk Text2Image generated" << data.size() << "bytes of image data";
    } else {
        qCDebug(logLLM) << "SparkDesk Text2Image returned empty data";
    }

    return imageData;
}

QPair<int, QString> SparkDesk::verify()
{
    qCInfo(logLLM) << "SparkDesk Starting account verification";

    XFConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    XFChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &SparkDesk::aborted, &chatCompletion, &XFChatCompletion::requestAborted);

    QPair<int, QString> errorPair = chatCompletion.create(m_accountProxy.model, m_accountProxy.url, conversion, m_params);
    setLastError(errorPair.first);
    setLastErrorString(errorPair.second);

    if (errorPair.first != 0) {
        qCWarning(logLLM) << "SparkDesk Account verification failed with error:" << errorPair.first << "-" << errorPair.second;
    } else {
        qCDebug(logLLM) << "SparkDesk Account verification successful";
    }

    return errorPair;
}

void SparkDesk::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    QPair<int, QJsonObject> contentPair = XFConversation::parseContentString(content);

    if (contentPair.first == 0 && !contentPair.second.value("content").toString().isEmpty())
        textChainContent(contentPair.second.value("content").toString());
}
