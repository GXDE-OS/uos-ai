// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LLMMODEL_H
#define LLMMODEL_H

#include <QString>
#include <QIcon>
#include <QVariant>

namespace uos_ai {

#define GENERATE_PARAM_TEMPERATURE "temperature"
#define GENERATE_PARAM_FUNCTION "function"
#define GENERATE_PARAM_ROLE "role"
#define GENERATE_RESPONSE_CODE "code"
#define GENERATE_RESPONSE_ERRORMSG "errorMsg"
#define GENERATE_RESPONSE_CONTENT "content"

class LLMModel
{
public:
    typedef bool (*streamFuncion)(const QString &deltaData, void *user);
public:
    explicit LLMModel() {}
    virtual ~LLMModel() {}
    virtual QString model() const = 0;
    virtual QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) = 0;

};

}

#endif // LLMMODEL_H
