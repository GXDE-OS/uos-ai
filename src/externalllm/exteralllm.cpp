// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exteralllm.h"

#include "llmmodel.h"

using namespace uos_ai;

ExteralLLM::ExteralLLM(const LLMServerProxy &serverproxy, LLMModel *m)
    : LLM(serverproxy)
    , model(m)
{
    Q_ASSERT(m);
    connect(this, &ExteralLLM::aborted, this, [this](){
        abortFlag = true;
    });
}

ExteralLLM::~ExteralLLM()
{
    delete model;
    model = nullptr;
}

QJsonObject ExteralLLM::predict(const QString &content, const QJsonArray &functions, const QString &systemRole, qreal temperature)
{
    QVariantHash params;
    params.insert(GENERATE_PARAM_TEMPERATURE, temperature);
    params.insert(GENERATE_PARAM_FUNCTION, functions);
    params.insert(GENERATE_PARAM_ROLE, systemRole);

    QJsonObject obj = model->generate(content, params, &ExteralLLM::streamData, this);

    if (obj[GENERATE_RESPONSE_CODE].toInt() != 0) {
        setLastError(obj[GENERATE_RESPONSE_CODE].toInt());
        setLastErrorString(obj[GENERATE_RESPONSE_ERRORMSG].toString());
    }
    obj.remove(GENERATE_RESPONSE_CODE);
    obj.remove(GENERATE_RESPONSE_ERRORMSG);

    return obj;
}

QPair<int, QString> ExteralLLM::verify()
{
    return qMakePair(0, QString());
}

bool ExteralLLM::streamData(const QString &deltaData, void *user)
{
    ExteralLLM *self = static_cast<ExteralLLM *>(user);
    if (!deltaData.isEmpty() && !self->abortFlag)
        emit self->readyReadChatDeltaContent(deltaData);

    return !self->abortFlag;
}
