// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "openrouterapi.h"

#include "aiconversation.h"
#include "universalchatcompletion.h"
#include "universalimage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logLLM)

OpenRouterAPI::OpenRouterAPI(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}

QPair<int, QString> OpenRouterAPI::verify()
{
    qCDebug(logLLM) << "OpenRouterAPI Starting account verification";
    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &OpenRouterAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "OpenRouterAPI Account verification failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "OpenRouterAPI Account verification successful";
    }

    return errorpair;
}

QJsonObject OpenRouterAPI::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "OpenRouterAPI Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions";
    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "OpenRouterAPI Setting system role for prediction";
        conversion.setSystemData(systemRole);
    }

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &OpenRouterAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);
    connect(&chatCompletion, &UniversalChatCompletion::readyReadDeltaContent, this, &OpenRouterAPI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "OpenRouterAPI Prediction completed with tools response";
        response["tools"] = tools;
    }
    return response;
}

QList<QByteArray> OpenRouterAPI::text2Image(const QString &prompt, int number)
{
    qCDebug(logLLM) << "Starting text-to-image conversion with prompt length:" << prompt.length()
                   << "and image count:" << number;
    UniversalImage textToImage(apiUrl(), m_accountProxy.account);
    connect(this, &OpenRouterAPI::aborted, &textToImage, &UniversalImage::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "Text-to-image conversion failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "Text-to-image conversion successful, generated" << imageData.size() << "images";
    }

    return imageData;
}

void OpenRouterAPI::onReadyReadChatDeltaContent(const QByteArray &content)
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

QString OpenRouterAPI::modelId() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_MODEL).toString();
}

QString OpenRouterAPI::apiUrl() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_URL).toString();
}
