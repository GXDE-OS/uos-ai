// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RPALLM_H
#define RPALLM_H

#include <llmmodel.h>

#include <QObject>
#include <QJsonArray>

namespace uos_ai {
namespace rpa {

class RpaLLM : public QObject, public LLMModel
{
    Q_OBJECT

public:
    static inline QString modelID() {
        return QString("wxqf");
    }

    static inline QString role() {
        return QString("RPA");
    }

public:
    explicit RpaLLM();
    QString model() const override;

    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;

    void setAbort() override;

private:
    QJsonArray m_funcs;

signals:
    void sigAbort();
};
}}
#endif // RPALLM_H
