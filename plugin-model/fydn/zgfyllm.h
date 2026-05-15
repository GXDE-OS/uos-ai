// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZGFYLLM_H
#define ZGFYLLM_H

#include <llmmodel.h>

namespace uos_ai {
namespace fydn {

class ZgfyLLM : public QObject, public LLMModel
{
    Q_OBJECT
public:
    static inline QString modelID() {
        return QString("flfg-llm");
    }

    static inline QString qaModelID() {
        return QString("qa-wanfallm");
    }

    static inline QString roleQa() {
        return QString("Q&A Service");
    }

    static inline QString roleFlfg() {
        return QString("Regulatory inquiry");
    }
public:
    explicit ZgfyLLM(const QString &id);
    QString model() const override;

    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;

    void setAbort() override;

signals:
    void sigAbort();
private:
    QString modelId;
};

}}

#endif // ZGFYLLM_H
