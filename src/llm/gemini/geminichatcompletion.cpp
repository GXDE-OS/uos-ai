// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "geminichatcompletion.h"

#include "servercodetranslation.h"

#include <QJsonDocument>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

UOSAI_USE_NAMESPACE

GeminiChatCompletion::GeminiChatCompletion(const QString &url, const AccountProxy &account)
    : BaseNetWork(account)
    , rootUrl(url)
{
    m_accountProxy.apiKey.clear();
    setTimeOut(3 * 60 * 1000);
}

QPair<int, QString> GeminiChatCompletion::create(const QString &model, GeminiConversation &conversation, const QVariantHash &params)
{
    qCDebug(logLLM) << "Creating Gemini chat completion for model:" << model;

    QJsonObject dataObject;
    if (!conversation.getSystemData().isEmpty()) {
        qCDebug(logLLM) << "Gemini Adding system instruction to request";
        dataObject.insert("system_instruction", conversation.getSystemData());
    }

    dataObject.insert("contents", conversation.getChatContent());
    auto baseresult = request(QUrl(rootUrl), dataObject, nullptr);

    if (baseresult.error != AIServer::NoError && baseresult.data.isEmpty()) {
        qCWarning(logLLM) << "Gemini Request failed with error:" << baseresult.error << "-" << baseresult.errorString;
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(baseresult.data);
        auto root = doc.object();
        if (root.contains("error")) {
            QJsonObject erObj = root["error"].toObject();
            if (erObj.contains("message")) {
                qCWarning(logLLM) << "Gemini API returned error:" << erObj.value("message").toString();
                baseresult.data = erObj.value("message").toString().toUtf8();
            }
        }
    }

    if (baseresult.error != 0) {
        qCCritical(logLLM) << "Gemini Chat completion failed with error:" << baseresult.error;
        return qMakePair(baseresult.error, baseresult.data);
    }

    conversation.update(baseresult.data);
    return qMakePair(0, QString());
}
