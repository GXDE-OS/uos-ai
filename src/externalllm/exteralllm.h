// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERALLLM_H
#define EXTERALLLM_H

#include "llm.h"

namespace uos_ai {
class LLMModel;
class ExteralLLM : public LLM
{
public:
    explicit ExteralLLM(const LLMServerProxy &serverproxy, LLMModel *m);
    ~ExteralLLM();
    QJsonObject predict(const QString &content, const QJsonArray &functions, const QString &systemRole = "", qreal temperature = 1.0) override;
    QPair<int, QString> verify() override;
protected:
    static bool streamData(const QString &deltaData, void *user);
protected:
    LLMModel *model = nullptr;
    bool abortFlag = false;
};

}
#endif // EXTERALLLM_H
