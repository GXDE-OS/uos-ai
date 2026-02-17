// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gemini_1_5.h"
#include "geminiconversation.h"
#include "geminichatcompletion.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UOSAI_USE_NAMESPACE

Gemini_1_5::Gemini_1_5(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}

QString Gemini_1_5::modelId() const
{
    const QMap<LLMChatModel, QString> modelIds = {
        {LLMChatModel::GEMINI_1_5_FLASH, "gemini-1.5-flash-latest"},
        {LLMChatModel::GEMINI_1_5_PRO, "gemini-1.5-pro-latest"},
    };

    return modelIds.value(m_accountProxy.model);
}

QString Gemini_1_5::apiUrl() const
{
   return QString("https://generativelanguage.googleapis.com/v1beta/models/%0:streamGenerateContent?alt=sse&key=%1")
                  .arg(modelId()).arg(m_accountProxy.account.apiKey);
}

QJsonObject Gemini_1_5::predict(const QString &content, const QJsonArray &functions)
{
    qCInfo(logLLM) << "Gemini_1_5 Starting prediction with content length:" << content.length();
    GeminiConversation conversion;
    conversion.setChatContent(content);

    QString systemRole = m_params.value(PREDICT_PARAM_SYSTEMROLE).toString();

    if (!systemRole.isEmpty()) {
        qCDebug(logLLM) << "Gemini_1_5 Setting system role for prediction";
        conversion.setSystemData(systemRole);
    }

    GeminiChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &Gemini_1_5::aborted, &chatCompletion, &GeminiChatCompletion::requestAborted);
    connect(&chatCompletion, &GeminiChatCompletion::readyReadDeltaContent, this, &Gemini_1_5::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) 
        qCWarning(logLLM) << "Gemini_1_5 Prediction failed with error:" << errorpair.first << "-" << errorpair.second;

    QJsonObject response;
    response["content"] = conversion.getLastResponse();

    return response;
}

QPair<int, QString> Gemini_1_5::verify()
{
    qCDebug(logLLM) << "Gemini_1_5 Starting account verification";
    GeminiConversation conversion;
    conversion.setChatContent("Account verification only, no need for any response.");

    GeminiChatCompletion chatCompletion(apiUrl(), m_accountProxy.account);
    connect(this, &Gemini_1_5::aborted, &chatCompletion, &GeminiChatCompletion::requestAborted);

    QPair<int, QString> errorpair = chatCompletion.create(modelId(), conversion, m_params);
    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0) {
        qCWarning(logLLM) << "Gemini_1_5 Verification failed with error:" << errorpair.first << "-" << errorpair.second;
    } else {
        qCDebug(logLLM) << "Gemini_1_5 Verification completed successfully";
    }

    return errorpair;
}

void Gemini_1_5::onReadyReadChatDeltaContent(const QByteArray &content)
{
    if (content.isEmpty())
        return;

    m_replied = true;

    if (!stream())
        return;

    const QJsonObject &deltacontent = GeminiConversation::parseContentString(QString::fromUtf8(content));
    if (deltacontent.contains("content"))
        textChainContent(deltacontent.value("content").toString());
}
