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
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UOSAI_USE_NAMESPACE

UniversalAPI::UniversalAPI(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}

QPair<int, QString> UniversalAPI::verify()
{
    qCDebug(logLLM) << "UniversalAPI Starting account verification";
    AIConversation conversion;
    conversion.addUserData("Account verification only, no need for any response.");

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);
    bool ok = false;
    connect(&chatCompletion, &UniversalChatCompletion::readyReadDeltaContent, this, [this, &ok](const QByteArray &content) {
        const QJsonObject &deltacontent = AIConversation::parseContentString(QString::fromUtf8(content));
        if (deltacontent.contains("reasoningContent") || deltacontent.contains("content")) {
            qCInfo(logLLM) << "UniversalAPI Account verification received content.";
            ok = true;
            QMetaObject::invokeMethod(this, "aborted", Qt::QueuedConnection);
        }
    });

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);

    if (ok)
        return QPair<int, QString>(0, "");

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "UniversalAPI Account verification failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "UniversalAPI Account verification successful";
    }

    return errorpair;
}

QJsonObject UniversalAPI::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "UniversalAPI Starting prediction with content length:" << content.length() 
                   << "and" << functions.size() << "functions";
    AIConversation conversion;
    conversion.addUserData(content);
    conversion.setFunctions(functions);
    conversion.filterThink();

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "UniversalAPI Setting system role for prediction";
        conversion.setSystemData(systemRole);
    }

    UniversalChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &chatCompletion, &UniversalChatCompletion::requestAborted);
    connect(&chatCompletion, &UniversalChatCompletion::readyReadDeltaContent, this, &UniversalAPI::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    QJsonObject tools = conversion.getLastTools();
    if (!tools.isEmpty()) {
        qCDebug(logLLM) << "UniversalAPI Prediction returned with tools data";
        response["tools"] = tools;
    }
    
    return response;
}

QList<QByteArray> UniversalAPI::text2Image(const QString &prompt, int number)
{
    qCDebug(logLLM) << "UniversalAPI Starting text-to-image conversion for prompt, size:" << prompt.size();
    UniversalImage textToImage(apiUrl(), m_accountProxy.account);
    connect(this, &UniversalAPI::aborted, &textToImage, &UniversalImage::requestAborted);

    QList<QByteArray> imageData;
    QPair<int, QString> errorpair = textToImage.create(prompt, imageData, number);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "UniversalAPI Text-to-image conversion failed with error:" << errorpair.first << errorpair.second;
    } else {
        qCDebug(logLLM) << "UniversalAPI Text-to-image conversion successful, generated" << imageData.size() << "images";
    }

    return imageData;
}

void UniversalAPI::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = AIConversation::parseContentString(content);
    readyThinkChainContent(deltacontent);
}

QString UniversalAPI::modelId() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_MODEL).toString();
}

QString UniversalAPI::apiUrl() const
{
    return m_accountProxy.ext.value(LLM_EXTKEY_VENDOR_URL).toString();
}
