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
    QString model = modelId();

    if (!functions.isEmpty()) {
        // 只有选择指令后才会带入Functions
        // 选择指令功能后强制调用V3模型
        qCInfo(logLLM) << "DeepSeekFree Functions provided, switching to V3 model, functions size:" << functions.size();
        model = v3Id();
        conversation.setFunctions(functions);
    } else if (enableOnlineSearch) {
        return onlineSearch(content);
    } else if (!hasThink) {
        qCInfo(logLLM) << "DeepSeekFree No think chain, switching to V3 model";
        model = v3Id();
    }

    qCDebug(logLLM) << "DeepSeekFree Creating completion with model:" << model;
    DeepSeekCompletion chatCompletion(baseUrl(), m_accountProxy.account);
    connect(this, &DeepSeekAI::aborted, &chatCompletion, &DeepSeekCompletion::requestAborted);
    connect(&chatCompletion, &DeepSeekCompletion::readyReadDeltaContent, this, &DeepSeekFree::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(model, conversation, m_params);
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
    const QString botId = hasThink ? searchR1Id() : searchV3Id();
    qCDebug(logLLM) << "DeepSeekFree Online search using bot ID:" << botId;

    QPair<int, QString> errorpair = chatCompletion.create(botId, conversation, {});
    if (errorpair.first != 0)
        qCWarning(logLLM) << "DeepSeekFree Online search failed with error:" << errorpair.first << errorpair.second;
  
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversation.getLastResponse();

    return response;
}
