#include "zhipu.h"
#include "zhipuconversation.h"
#include "zhipuchatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ZhiPuAI::ZhiPuAI(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject ZhiPuAI::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    Q_UNUSED(systemRole)
    ZhiPuConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    ZhiPuChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ZhiPuAI::aborted, &chatCompletion, &ZhiPuChatCompletion::requestAborted);
    connect(&chatCompletion, &ZhiPuChatCompletion::readyReadDeltaContent, this, &ZhiPuAI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, conversion, temperature);
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

QPair<int, QString> ZhiPuAI::verify()
{
    ZhiPuConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    ZhiPuChatCompletion chatCompletion(m_accountProxy.account);
    connect(this, &ZhiPuAI::aborted, &chatCompletion, &ZhiPuChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(m_accountProxy.model, conversion);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

void ZhiPuAI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;

    const QPair<int, QString> &deltacontent = ZhiPuConversation::parseContentString(content);
    if (deltacontent.first == 0 && !deltacontent.second.isEmpty())
        emit readyReadChatDeltaContent(deltacontent.second);
}
