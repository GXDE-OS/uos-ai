// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COZEAGENT_H
#define COZEAGENT_H

#include "llm.h"
#include "uosai_global.h"

#include <QByteArray>

namespace uos_ai {
class CozeAgent : public LLM
{
    Q_OBJECT
public:
    explicit CozeAgent(const LLMServerProxy &serverproxy);
    QPair<int, QString> verify() override;
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
protected:
    QJsonArray conversation(const QString &content);
    QString parseContentString(const QByteArray &content);
protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
protected:
    QByteArray m_deltacontent;
    QString anwserContent;
    QPair<int, QString> chatFailed;
};

}
#endif // COZEAGENT_H
