// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UNIVERSALAGENTAPI_H
#define UNIVERSALAGENTAPI_H

#include "llm.h"
#include "uosai_global.h"

namespace uos_ai {
class UniversalAgentAPI : public LLM
{
public:
    UniversalAgentAPI(const LLMServerProxy &serverproxy);
    QPair<int, QString> verify() override;
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
private:
    QString apiUrl() const;
    QJsonObject params();
};

}
#endif // UNIVERSALAGENTAPI_H
