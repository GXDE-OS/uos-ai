#include "360.h"
#include "360conversation.h"
#include "360chatcompletion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

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

QJsonObject Gpt360::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "360 Starting prediction with content length:" << content.length()
                   << "and" << functions.size() << "functions";

    Conversation360 conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "360 Setting system role.";
        conversion.setSystemData(systemRole);
    }

    ChatCompletion360 chatCompletion(m_accountProxy.account);
    connect(this, &Gpt360::aborted, &chatCompletion, &ChatCompletion360::requestAborted);
    connect(&chatCompletion, &ChatCompletion360::readyReadDeltaContent, this, &Gpt360::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "360 Prediction failed with error:" << errorpair.first 
                         << "-" << errorpair.second;
    }

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "360 Prediction returned tools response";
        response["tools"] = tools;
    }
    return response;
}

QPair<int, QString> Gpt360::verify()
{
    qCDebug(logLLM) << "360 Starting account verification";
    Conversation360 conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    ChatCompletion360 chatCompletion(m_accountProxy.account);
    connect(this, &Gpt360::aborted, &chatCompletion, &ChatCompletion360::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0)
        qCWarning(logLLM) << "360 Account verification failed with error:" << errorpair.first
                         << "-" << errorpair.second;

    return errorpair;
}

void Gpt360::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    QString deltacontent = Conversation360::parseContentString(content);
    if (!deltacontent.isEmpty())
        textChainContent(deltacontent);
}
