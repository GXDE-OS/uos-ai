#include "chatgpt.h"
#include "aiconversation.h"
#include "aichatcompletion.h"
#include "aiimages.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

ChatGpt::ChatGpt(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QString ChatGpt::modelId()
{
    const QMap<LLMChatModel, QString> modelIds = {
        {LLMChatModel::CHATGPT_3_5, "gpt-3.5-turbo"},
        {LLMChatModel::CHATGPT_3_5_16K, "gpt-3.5-turbo-16k"},
        {LLMChatModel::CHATGPT_4, "gpt-4"},
        {LLMChatModel::CHATGPT_4_32K, "gpt-4-32k"}
    };

    return modelIds.value(m_accountProxy.model);
}

QJsonObject ChatGpt::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "ChatGpt Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions";
    
    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "ChatGpt Setting system role for prediction";
        conversion.setSystemData(systemRole);
    }

    AIChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &chatCompletion, &AIChatCompletion::requestAborted);
    connect(&chatCompletion, &AIChatCompletion::readyReadDeltaContent, this, &ChatGpt::onReadyReadChatDeltaContent);

#ifdef OpenTextAuditService
    connect(m_tasMgr.data(), &TasManager::sigAuditContentResult, this, [&](QSharedPointer<TextAuditResult> result) {
        if (result && result->code == TextAuditEnum::None) {
            textChainContent(result->content);
        } else if (result->code == TextAuditEnum::NetError) {
            setLastError(AIServer::ErrorType::NetworkError);
            setLastErrorString(ServerCodeTranslation::serverCodeTranslation(lastError(), ""));
            emit aborted();
        } else {
            setLastError(AIServer::ErrorType::SenSitiveInfoError);
            setLastErrorString(ServerCodeTranslation::serverCodeTranslation(lastError(), ""));
            emit aborted();
        }
    });
    connect(&chatCompletion, &AIChatCompletion::requestFinished, this, [&]() {
        m_tasMgr->endAuditText();
    });
#endif

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);

#ifdef OpenTextAuditService
    do {
        TimerEventLoop oneloop;
        oneloop.setTimeout(1000);
        oneloop.exec();
        m_tasMgr->stopAuditing();
    } while (!m_tasMgr->auditFinished());
    if (lastError() != AIServer::ErrorType::NetworkError && lastError() != AIServer::ErrorType::SenSitiveInfoError) {
        setLastError(errorpair.first);
        setLastErrorString(ServerCodeTranslation::serverCodeTranslation(lastError(), errorpair.second));
    }
#else
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);
#endif

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "ChatGpt Prediction returned tools response";
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> ChatGpt::text2Image(const QString &prompt, int number)
{
    qCDebug(logLLM) << "Starting text2image with prompt length:" << prompt.length()
                   << "and image count:" << number;

    AIImages textToImage(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &textToImage, &AIImages::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    
    if (errorpair.first != 0) {
        qCWarning(logLLM) << "Text2Image failed with error:" << errorpair.first
                         << "-" << errorpair.second;
    } else {
        qCDebug(logLLM) << "Text2Image completed successfully, generated" 
                       << imageData.size() << "images";
    }

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return imageData;
}

QPair<int, QString> ChatGpt::verify()
{
    qCDebug(logLLM) << "Starting account verification";

    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    AIChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &chatCompletion, &AIChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    
    if (errorpair.first != 0) {
        qCWarning(logLLM) << "Account verification failed with error:" << errorpair.first
                         << "-" << errorpair.second;
    } else {
        qCDebug(logLLM) << "Account verification successful";
    }

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

void ChatGpt::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = AIConversation::parseContentString(content);

#ifdef OpenTextAuditService
    if (!deltacontent.isEmpty()) {
        m_tasMgr->auditText(deltacontent.toLocal8Bit());
    }
#else
    if (deltacontent.contains("content"))
        textChainContent(deltacontent.value("content").toString());
#endif
}
