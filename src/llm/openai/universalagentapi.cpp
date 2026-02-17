// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalagentapi.h"
#include "aiconversation.h"
#include "universalnetwork.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UniversalAgentAPI::UniversalAgentAPI(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}


QPair<int, QString> UniversalAgentAPI::verify()
{
    return qMakePair(0, QString());
}

QJsonObject UniversalAgentAPI::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "UniversalAgentAPI Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions"
                   << "url" << apiUrl();

    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "UniversalAgentAPI Setting system role for conversation";
        conversion.setSystemData(systemRole);
    }

    UniversalNetWork chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAgentAPI::aborted, &chatCompletion, &UniversalNetWork::requestAborted);
    connect(&chatCompletion, &UniversalNetWork::readyReadDeltaContent, this, &UniversalAgentAPI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(conversion, m_params, params());
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "UniversalAgentAPI Tools data included in response";
        response["tools"] = tools;
    }
    return response;
}

void UniversalAgentAPI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = AIConversation::parseContentString(content);
    if (deltacontent.contains("content"))
        textChainContent(deltacontent.value("content").toString());
}

QString UniversalAgentAPI::apiUrl() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_URL).toString();
}

QJsonObject UniversalAgentAPI::params()
{
    return QJsonObject::fromVariantHash(m_accountProxy.
                                        ext.value(LLM_EXTKEY_VENDOR_PARAMS).toHash());
}
