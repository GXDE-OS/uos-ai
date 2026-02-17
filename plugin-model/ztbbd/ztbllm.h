// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZTBLLM_H
#define ZTBLLM_H

#include <llmmodel.h>

#include <QObject>

namespace uos_ai {
namespace ztb {

class ZtbLLM : public QObject, public LLMModel
{
    Q_OBJECT
public:
    static inline QString modelID() {
        return QString("UOS LM");
    }

    static inline QString role() {
        return QString("ztbbd");
    }

public:
    explicit ZtbLLM(QObject *parent = nullptr);
    QString model() const override;
    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;
    void setAbort() override;

signals:
    void sigAbort();

public slots:
};

}}

#endif // ZTBLLM_H
