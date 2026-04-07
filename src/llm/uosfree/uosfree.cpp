// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosfree.h"
#include "uosfreeconversation.h"
#include "uosfreecompletion.h"
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UosFree::UosFree(const LLMServerProxy &serverproxy)
    : LLM(serverproxy)
{

}

QJsonObject UosFree::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "Starting prediction with content length:" << content.length();

    UosFreeConversation conversation;
    conversation.addUserData(content);
    conversation.filterThink();

    bool hasThink = m_params.value(PREDICT_PARAM_THINKCHAIN).toBool();
    bool enableOnlineSearch = m_params.value(PREDICT_PARAM_ONLINESEARCH).toBool();

    if (!functions.isEmpty()) {
        // 只有选择指令后才会带入Functions
        qCInfo(logLLM) << "Functions provided, functions size:" << functions.size();
        conversation.setFunctions(functions);
    } else if (enableOnlineSearch) {
        return onlineSearch(content);
    }

    // 通过 thinking 参数控制思考模式
    // Functions 场景下禁用思考模式
    bool thinkEnabled = functions.isEmpty() && hasThink;
    QVariantHash params = m_params;
    params[PREDICT_PARAM_THINKINGMODE] = thinkEnabled ? QString("enabled") : QString("disabled");

    qCDebug(logLLM) << "Creating completion with model:" << modelId() << "thinking:" << (thinkEnabled ? "enabled" : "disabled");
    UosFreeCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &UosFree::aborted, &chatCompletion, &UosFreeCompletion::requestAborted);
    connect(&chatCompletion, &UosFreeCompletion::readyReadDeltaContent, this, &UosFree::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversation, params);
    if (errorpair.first != 0)
        qCWarning(logLLM) << "Prediction failed with error:" << errorpair.first << errorpair.second;

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    QJsonObject tools = conversation.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "Tools response received";
        response["tools"] = tools;
    }

    return response;
}

QPair<int, QString> UosFree::verify()
{
    qCDebug(logLLM) << "UosFree Starting account verification";
    UosFreeConversation conversation;
    conversation.addUserData("Account verification only, no need for any response.");

    UosFreeCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &UosFree::aborted, &chatCompletion, &UosFreeCompletion::requestAborted);

    bool ok = false;
    connect(&chatCompletion, &UosFreeCompletion::readyReadDeltaContent, this, [this, &ok](const QByteArray &content) {
        const QJsonObject &deltacontent = UosFreeConversation::parseContentString(QString::fromUtf8(content));
        if (deltacontent.contains("reasoningContent") || deltacontent.contains("content")) {
            qCInfo(logLLM) << "UosFree Account verification received content.";
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
        qCWarning(logLLM) << "UosFree Account verification failed:" << errorpair.second;
    } else {
        qCDebug(logLLM) << "UosFree Account verification successful";
    }

    return errorpair;
}

QJsonObject UosFree::onlineSearch(const QString &content)
{
    qCInfo(logLLM) << "Starting online search with content length:" << content.length();

    UosFreeConversation conversation;
    conversation.addUserData(content);
    conversation.filterThink();

    UosFreeCompletion chatCompletion(searchUrl(), m_accountProxy.account);
    connect(this, &UosFree::aborted, &chatCompletion, &UosFreeCompletion::requestAborted);
    connect(&chatCompletion, &UosFreeCompletion::readyReadDeltaContent, this, &UosFree::onReadyReadChatDeltaContent);

    bool hasThink = m_params.value(PREDICT_PARAM_THINKCHAIN).toBool();
    const QString botId = searchBotId();
    qCDebug(logLLM) << "Online search using bot ID:" << botId;

    QVariantHash params = m_params;
    params[PREDICT_PARAM_THINKINGMODE] = hasThink ? QString("enabled") : QString("disabled");

    QPair<int, QString> errorpair = chatCompletion.create(botId, conversation, params);
    if (errorpair.first != 0)
        qCWarning(logLLM) << "Online search failed with error:" << errorpair.first << errorpair.second;

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    return response;
}

void UosFree::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = UosFreeConversation::parseContentString(QString::fromUtf8(content));
    readyThinkChainContent(deltacontent);
}
