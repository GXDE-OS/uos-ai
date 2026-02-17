// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalnetwork.h"

using namespace uos_ai;
UniversalNetWork::UniversalNetWork(const QString &url, const AccountProxy &account)
    : AINetWork(account)
    , rootUrl(url)
{
    setTimeOut(60 * 1000);
}

QString UniversalNetWork::rootUrlPath() const
{
    return rootUrl;
}

QPair<int, QString> UniversalNetWork::create(AIConversation &conversation, const QVariantHash &params , const QJsonObject &jparams)
{
    QJsonObject dataObject;
    dataObject.insert("messages", conversation.getConversions());
    if (params.contains("temperature"))
        dataObject.insert("temperature", qBound(0.1, params.value("temperature").toDouble(), 0.9));
    dataObject.insert("stream", true);

    for (const QString &key : jparams.keys())
        dataObject.insert(key, jparams.value(key));

    const QPair<int, QByteArray> &resultPairs = request(dataObject, "");

    if (resultPairs.first != 0)
        return qMakePair(resultPairs.first, resultPairs.second);

    conversation.update(resultPairs.second);
    return qMakePair(0, QString());
}
