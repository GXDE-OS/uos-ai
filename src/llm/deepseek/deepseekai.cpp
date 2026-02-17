#include "deepseekai.h"
#include "deepseekconversation.h"
#include "deepseekcompletion.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UOSAI_USE_NAMESPACE

DeepSeekAI::DeepSeekAI(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject DeepSeekAI::predict(const QString &content, const QJsonArray &functions)
{
    Q_UNUSED(functions)
    qCInfo(logLLM) << "DeepSeekAI Starting prediction with content length:" << content.length();

    DeepSeekConversation conversation;
    conversation.addUserData(content);
    conversation.filterThink();

    DeepSeekCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &DeepSeekAI::aborted, &chatCompletion, &DeepSeekCompletion::requestAborted);
    connect(&chatCompletion, &DeepSeekCompletion::readyReadDeltaContent, this, &DeepSeekAI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversation, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0)
        qCWarning(logLLM) << "DeepSeekAI Prediction failed with error:" << errorpair.first << "-" << errorpair.second;

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    return response;
}

QPair<int, QString> DeepSeekAI::verify()
{
    qCDebug(logLLM) << "DeepSeekAI Starting account verification";
    DeepSeekConversation conversation;
    conversation.addUserData("Account verification only, no need for any response.");

    DeepSeekCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &DeepSeekAI::aborted, &chatCompletion, &DeepSeekCompletion::requestAborted);

    bool ok = false;
    connect(&chatCompletion, &DeepSeekCompletion::readyReadDeltaContent, this, [this, &ok](const QByteArray &content) {
        const QJsonObject &deltacontent = DeepSeekConversation::parseContentString(QString::fromUtf8(content));
        if (deltacontent.contains("reasoningContent") || deltacontent.contains("content")) {
            qCInfo(logLLM) << "DeepSeekAI Account verification received content.";
            ok = true;
            QMetaObject::invokeMethod(this, "aborted", Qt::QueuedConnection);
        }
    });

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversation, m_params);

    if (ok)
        return QPair<int, QString>(0, "");

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "Account verification failed:" << errorpair.second;
    } else {
        qCDebug(logLLM) << "Account verification successful";
    }

    return errorpair;
}

void DeepSeekAI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = DeepSeekConversation::parseContentString(QString::fromUtf8(content));
    readyThinkChainContent(deltacontent);
}
