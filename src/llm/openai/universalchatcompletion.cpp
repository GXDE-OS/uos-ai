// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalchatcompletion.h"

UOSAI_USE_NAMESPACE

UniversalChatCompletion::UniversalChatCompletion(const QString &url, const AccountProxy &account)
    : AINetWork(account)
    , rootUrl(url)
{

}

QString UniversalChatCompletion::rootUrlPath() const
{
    return rootUrl;
}

QPair<int, QString> UniversalChatCompletion::create(const QString &model, AIConversation &conversation, qreal temperature)
{
    QJsonObject dataObject;
    dataObject.insert("model", model);
    dataObject.insert("messages", conversation.getConversions());
    dataObject.insert("temperature", qBound(0.01, temperature, 0.99)); // zhihu模型的取值为(0, 1)的开区间
    dataObject.insert("stream", true);

    // 自定义模型暂不支持function call，存在不同模型返回方式不一样的问题。
#if 0
    if (!conversation.getFunctions().isEmpty()) {
        QJsonArray tools;
        for (const QJsonValue &func : conversation.getFunctions()) {
            QJsonObject funcObj;
            funcObj.insert("type", "function");
            funcObj.insert("function", func);
            tools.append(funcObj);
        }
        dataObject.insert("tools", tools);
    }
#endif

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "/chat/completions");

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
