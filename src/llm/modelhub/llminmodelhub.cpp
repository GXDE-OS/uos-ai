// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "llminmodelhub.h"

#include "aiconversation.h"
#include "universalchatcompletion.h"
#include "universalimage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logLLM)

LLMinModelHub::LLMinModelHub(QSharedPointer<ModelhubWrapper> ins, const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
    , wrapper(ins)
{

}

QPair<int, QString> LLMinModelHub::verify()
{
    if (wrapper.isNull()) {
        qCWarning(logLLM) << "Modelhub wrapper is null - account invalid";
        QPair<int, QString> errorpair(AIServer::AccountInvalid, "");
        setLastError(errorpair.first);
        setLastErrorString(errorpair.second);
        return errorpair;
    }

    qCInfo(logLLM) << "LLMinModelHub Starting account verification";
    wrapper->ensureRunning();

    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");
    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != AIServer::NoError) {
        qCWarning(logLLM) << "Account verification failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "Account verification successful";
    }

    return errorpair;
}

QJsonObject LLMinModelHub::predict(const QString &content, const QJsonArray &functions)
{
    if (wrapper.isNull()) {
        qCWarning(logLLM) << "Modelhub wrapper is null - account invalid";
        setLastError(AIServer::AccountInvalid);
        return QJsonObject();
    }

    qCDebug(logLLM) << "Modelhub Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions";
    wrapper->ensureRunning();

    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);
    conversion.filterThink();

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty())
        conversion.setSystemData(systemRole);

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &LLMinModelHub::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);
    connect(&chatCompletion, &UniversalChatCompletion::readyReadDeltaContent, this, &LLMinModelHub::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != AIServer::NoError) {
        qCWarning(logLLM) << "Modelhub Prediction failed with error:" << errorpair.first << errorpair.second;
    }

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "Modelhub Prediction completed with tools response";
        response["tools"] = tools;
    }
    return response;
}

void LLMinModelHub::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = AIConversation::parseContentString(content);

    readyThinkChainContent(deltacontent);
}

QString LLMinModelHub::modelId() const
{
    return m_accountProxy.llmName(m_accountProxy.model);
}

QString LLMinModelHub::apiUrl() const
{
    return wrapper->urlPath("");
}
