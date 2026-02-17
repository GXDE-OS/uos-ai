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
        return QString("万法大模型");
    }

    static inline QString roleQa() {
        return QString("Q&A Service");
    }

    static inline QString roleFlfg() {
        return QString("Regulatory inquiry");
    }
public:
    explicit ZgfyLLM();
    QString model() const override;

    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;

    void setAbort() override;

signals:
    void sigAbort();
};

}}

#endif // ZGFYLLM_H
