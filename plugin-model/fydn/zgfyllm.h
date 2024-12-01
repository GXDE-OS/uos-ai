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
        return QString("中国法研LLM");
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
};

}}

#endif // ZGFYLLM_H
