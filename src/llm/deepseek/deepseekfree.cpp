// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deepseekfree.h"
#include "deepseekconversation.h"
#include "deepseekcompletion.h"
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logLLM)

DeepSeekFree::DeepSeekFree(const LLMServerProxy &serverproxy)
    : DeepSeekAI(serverproxy)
{

}

QJsonObject DeepSeekFree::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "DeepSeekFree Starting prediction with content length:" << content.length();

    DeepSeekConversation conversation;
    conversation.addUserData(content);
    conversation.filterThink();

    bool hasThink = m_params.value(PREDICT_PARAM_THINKCHAIN).toBool();
    bool enableOnlineSearch = m_params.value(PREDICT_PARAM_ONLINESEARCH).toBool();

    if (!functions.isEmpty()) {
        // 只有选择指令后才会带入Functions
        qCInfo(logLLM) << "DeepSeekFree Functions provided, functions size:" << functions.size();
        conversation.setFunctions(functions);
    } else if (enableOnlineSearch) {
        return onlineSearch(content);
    }

    // 统一使用 deepseek-v3-2 模型，通过 thinking 参数控制思考模式
    // Functions 场景下禁用思考模式
    bool thinkEnabled = functions.isEmpty() && hasThink;
    QVariantHash params = m_params;
    params[PREDICT_PARAM_THINKINGMODE] = thinkEnabled ? QString("enabled") : QString("disabled");

    qCDebug(logLLM) << "DeepSeekFree Creating completion with model:" << modelId() << "thinking:" << (thinkEnabled ? "enabled" : "disabled");
    DeepSeekCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &DeepSeekAI::aborted, &chatCompletion, &DeepSeekCompletion::requestAborted);
    connect(&chatCompletion, &DeepSeekCompletion::readyReadDeltaContent, this, &DeepSeekFree::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversation, params);
    if (errorpair.first != 0)
        qCWarning(logLLM) << "DeepSeekFree Prediction failed with error:" << errorpair.first << errorpair.second;

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    QJsonObject tools = conversation.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "DeepSeekFree Tools response received";
        response["tools"] = tools;
    }

    return response;
}

QJsonObject DeepSeekFree::onlineSearch(const QString &content)
{
    qCInfo(logLLM) << "DeepSeekFree Starting online search with content length:" << content.length();

    DeepSeekConversation conversation;
    conversation.addUserData(content);
    conversation.filterThink();

    DeepSeekCompletion chatCompletion(searchUrl(), m_accountProxy.account);
    connect(this, &DeepSeekAI::aborted, &chatCompletion, &DeepSeekCompletion::requestAborted);
    connect(&chatCompletion, &DeepSeekCompletion::readyReadDeltaContent, this, &DeepSeekFree::onReadyReadChatDeltaContent);

    bool hasThink = m_params.value(PREDICT_PARAM_THINKCHAIN).toBool();
    const QString botId = searchBotId();
    qCDebug(logLLM) << "DeepSeekFree Online search using bot ID:" << botId;

    QVariantHash params = m_params;
    params[PREDICT_PARAM_THINKINGMODE] = hasThink ? QString("enabled") : QString("disabled");

    QPair<int, QString> errorpair = chatCompletion.create(botId, conversation, params);
    if (errorpair.first != 0)
        qCWarning(logLLM) << "DeepSeekFree Online search failed with error:" << errorpair.first << errorpair.second;

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    return response;
}
