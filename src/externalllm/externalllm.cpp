// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externalllm.h"

#include "llmmodel.h"
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)
using namespace uos_ai;

ExternalLLM::ExternalLLM(const LLMServerProxy &serverproxy, LLMModel *m)
    : LLM(serverproxy)
    , model(m)
{
    Q_ASSERT(m);
    qCDebug(logExternalLLM) << "Creating ExternalLLM instance";
    connect(this, &ExternalLLM::aborted, this, [this](){
        abortFlag = true;
        model->setAbort();
    });
}

ExternalLLM::~ExternalLLM()
{
    delete model;
    model = nullptr;
}

QJsonObject ExternalLLM::predict(const QString &content, const QJsonArray &functions)
{
    qCDebug(logExternalLLM) << "Starting prediction with content length:" << content.length();
    QVariantHash genParams;
    if (m_params.contains(PREDICT_PARAM_TEMPERATURE))
        genParams.insert(GENERATE_PARAM_TEMPERATURE, m_params.value(PREDICT_PARAM_TEMPERATURE, 1.0).toReal());
    genParams.insert(GENERATE_PARAM_FUNCTION, functions);
    genParams.insert(GENERATE_PARAM_ROLE, m_params.value(PREDICT_PARAM_SYSTEMROLE).toString());

    QJsonObject obj = model->generate(content, genParams, &ExternalLLM::streamData, this);

    if (obj[GENERATE_RESPONSE_CODE].toInt() != 0) {
        qCWarning(logExternalLLM) << "Prediction failed with error code:" 
                                 << obj[GENERATE_RESPONSE_CODE].toInt()
                                 << "message:" << obj[GENERATE_RESPONSE_ERRORMSG].toString();
        setLastError(obj[GENERATE_RESPONSE_CODE].toInt());
        setLastErrorString(obj[GENERATE_RESPONSE_ERRORMSG].toString());
    }
    obj.remove(GENERATE_RESPONSE_CODE);
    obj.remove(GENERATE_RESPONSE_ERRORMSG);

    return obj;
}

QPair<int, QString> ExternalLLM::verify()
{
    return qMakePair(0, QString());
}

bool ExternalLLM::streamData(const QString &deltaData, void *user)
{
    ExternalLLM *self = static_cast<ExternalLLM *>(user);
    if (!deltaData.isEmpty() && !self->abortFlag) {
        // 尝试解析为 JSON
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(deltaData.toUtf8(), &parseError);
        
        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            // 是有效的 JSON 对象，调用 readyThinkChainContent
            QJsonObject jsonObj = jsonDoc.object();
            self->readyThinkChainContent(jsonObj);
        } else {
            // 不是有效的 JSON，当作普通字符串处理
            self->textChainContent(deltaData);
        }
    }

    return !self->abortFlag;
}
