#include "360.h"
#include "360conversation.h"
#include "360chatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Gpt360::Gpt360(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QString Gpt360::modelId()
{
    const QMap<LLMChatModel, QString> modelIds = {
        {LLMChatModel::GPT360_S2_V9, "360GPT_S2_V9"}
    };

    return modelIds.value(m_accountProxy.model);
}

QJsonObject Gpt360::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    Conversation360 conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    if (!systemRole.isEmpty())
        conversion.setSystemData(systemRole);

    ChatCompletion360 chatCompletion(m_accountProxy.account);
    connect(this, &Gpt360::aborted, &chatCompletion, &ChatCompletion360::requestAborted);
    connect(&chatCompletion, &ChatCompletion360::readyReadDeltaContent, this, &Gpt360::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, temperature);
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

QPair<int, QString> Gpt360::verify()
{
    Conversation360 conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    ChatCompletion360 chatCompletion(m_accountProxy.account);
    connect(this, &Gpt360::aborted, &chatCompletion, &ChatCompletion360::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

void Gpt360::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;

    QString deltacontent = Conversation360::parseContentString(content);
    if (!deltacontent.isEmpty())
        emit readyReadChatDeltaContent(deltacontent);
}
