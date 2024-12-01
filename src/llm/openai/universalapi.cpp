// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalapi.h"

#include "aiconversation.h"
#include "universalchatcompletion.h"
#include "universalimage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

UOSAI_USE_NAMESPACE

UniversalAPI::UniversalAPI(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}

QPair<int, QString> UniversalAPI::verify()
{
    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return errorpair;
}

QJsonObject UniversalAPI::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    if (!systemRole.isEmpty())
        conversion.setSystemData(systemRole);

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);
    connect(&chatCompletion, &UniversalChatCompletion::readyReadDeltaContent, this, &UniversalAPI::onReadyReadChatDeltaContent);

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

QList<QByteArray> UniversalAPI::text2Image(const QString &prompt, int number)
{
    UniversalImage textToImage(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &textToImage, &UniversalImage::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    return imageData;
}

void UniversalAPI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty() || !stream())
        return;
    const QJsonObject &deltacontent = AIConversation::parseContentString(content);
    if (deltacontent.contains("content"))
        emit readyReadChatDeltaContent(deltacontent.value("content").toString());
}

QString UniversalAPI::modelId() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_MODEL).toString();
}

QString UniversalAPI::apiUrl() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_URL).toString();
}
