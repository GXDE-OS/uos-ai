// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERNALLLM_H
#define EXTERNALLLM_H

#include "llm.h"

namespace uos_ai {
class LLMModel;
class ExternalLLM : public LLM
{
public:
    explicit ExternalLLM(const LLMServerProxy &serverproxy, LLMModel *m);
    ~ExternalLLM();
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    QPair<int, QString> verify() override;
protected:
    static bool streamData(const QString &deltaData, void *user);
protected:
    LLMModel *model = nullptr;
    bool abortFlag = false;
};

}
#endif // EXTERNALLLM_H
