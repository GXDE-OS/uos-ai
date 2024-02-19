#include "chatgpt.h"
#include "aiconversation.h"
#include "aichatcompletion.h"
#include "aiimages.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

QJsonObject ChatGpt::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    if (!systemRole.isEmpty())
        conversion.setSystemData(systemRole);

    AIChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &chatCompletion, &AIChatCompletion::requestAborted);
    connect(&chatCompletion, &AIChatCompletion::readyReadDeltaContent, this, &ChatGpt::onReadyReadChatDeltaContent);

#ifdef OpenTextAuditService
    connect(m_tasMgr.data(), &TasManager::sigAuditContentResult, this, [&](QSharedPointer<TextAuditResult> result) {
        if (result && result->code == TextAuditEnum::None) {
            emit readyReadChatDeltaContent(result->content);
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

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, temperature);

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
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> ChatGpt::text2Image(const QString &prompt, int number)
{
    AIImages textToImage(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &textToImage, &AIImages::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return imageData;
}

QPair<int, QString> ChatGpt::verify()
{
    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    AIChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ChatGpt::aborted, &chatCompletion, &AIChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

void ChatGpt::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;
    const QJsonObject &deltacontent = AIConversation::parseContentString(content);

#ifdef OpenTextAuditService
    if (!deltacontent.isEmpty()) {
        m_tasMgr->auditText(deltacontent.toLocal8Bit());
    }
#else
    if (deltacontent.contains("content"))
        emit readyReadChatDeltaContent(deltacontent.value("content").toString());
#endif
}
