#include "zhipu.h"
#include "zhipuconversation.h"
#include "zhipuchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

ZhiPuAI::ZhiPuAI(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject ZhiPuAI::predict(const QString &content, const QJsonArray &functions)
{
    qCDebug(logLLM) << "ZhiPuAI Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions";

    ZhiPuConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    ZhiPuChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ZhiPuAI::aborted, &chatCompletion, &ZhiPuChatCompletion::requestAborted);
    connect(&chatCompletion, &ZhiPuChatCompletion::readyReadDeltaContent, this, &ZhiPuAI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        response["tools"] = tools;
        qCDebug(logLLM) << "ZhiPuAI Tools data included in response";
    }
    return response;
}

QPair<int, QString> ZhiPuAI::verify()
{
    qCDebug(logLLM) << "ZhiPuAI Starting account verification";

    ZhiPuConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    ZhiPuChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ZhiPuAI::aborted, &chatCompletion, &ZhiPuChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "ZhiPuAI Account verification failed with error:" << errorpair.first 
                         << "-" << errorpair.second;
    } else {
        qCDebug(logLLM) << "ZhiPuAI Account verification successful";
    }

    return errorpair;
}

void ZhiPuAI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QPair<int, QString> &deltacontent = ZhiPuConversation::parseContentString(content);
    if (deltacontent.first == 0 && !deltacontent.second.isEmpty())
        textChainContent(deltacontent.second);
}
