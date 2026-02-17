// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GEMINI_1_5_H
#define GEMINI_1_5_H

#include "uosai_global.h"
#include "llm.h"

namespace uos_ai {

class Gemini_1_5 : public LLM
{
    Q_OBJECT
public:
    explicit Gemini_1_5(const LLMServerProxy &serverproxy);
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    QPair<int, QString> verify() override;
protected:
    QString modelId() const;
    QString apiUrl() const;
private slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
};

}
#endif // GEMINI_1_5_H
