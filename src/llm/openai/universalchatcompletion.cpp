// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalchatcompletion.h"
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(logLLM)
UniversalChatCompletion::UniversalChatCompletion(const QString &url, const AccountProxy &account)
    : AINetWork(account)
    , rootUrl(url)
{
    setTimeOut(5 * 60 * 1000);
}

QString UniversalChatCompletion::rootUrlPath() const
{
    return rootUrl;
}

QPair<int, QString> UniversalChatCompletion::create(const QString &model, AIConversation &conversation, const QVariantHash &params)
{
    QJsonObject dataObject;
    dataObject.insert("model", model);
    dataObject.insert("messages", conversation.getConversions());
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.1, params.value("temperature").toDouble(), 0.9)); // zhihu模型的取值为(0, 1)的开区间
    dataObject.insert("stream", true);
    if (params.contains("max_tokens"))
        dataObject.insert("max_tokens", params.value("max_tokens").toInt());

    qCDebug(logLLM) << "Universal Creating chat completion for model:" << model 
                   << "with" << conversation.getConversions().size() << "messages"
                   << rootUrlPath();

    if (!conversation.getFunctions().isEmpty()) {
        QJsonArray tools;
        for (const QJsonValue &func : conversation.getFunctions()) {
            QJsonObject funcObj;
            funcObj.insert("type", "function");
            funcObj.insert("function", func);
            tools.append(funcObj);
        }
        dataObject.insert("tools", tools);
        qCInfo(logLLM) << "Universal Added" << tools.size() << "tools/functions to request";
    }

    const QString suffix = "/chat/completions";
    const QPair<int, QByteArray> &resultPairs = request(dataObject, rootUrlPath().contains(suffix) ? "" : suffix);

    if (resultPairs.first != 0) {
        qCWarning(logLLM) << "Universal Chat completion request failed with error:" << resultPairs.first
                         << "Message:" << resultPairs.second;
        return qMakePair(resultPairs.first, resultPairs.second);
    }

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
