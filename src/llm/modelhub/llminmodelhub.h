// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LLMINMODELHUB_H
#define LLMINMODELHUB_H

#include "llm.h"
#include "universalapi.h"
#include "externalllm/modelhubwrapper.h"

namespace uos_ai {

class LLMinModelHub : public LLM
{
public:
    explicit LLMinModelHub(QSharedPointer<ModelhubWrapper> ins, const LLMServerProxy &serverproxy);
    QPair<int, QString> verify() override;
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
private:
    QString modelId() const;
    QString apiUrl() const;
private:
    QSharedPointer<ModelhubWrapper> wrapper;
};

}
#endif // LLMINMODELHUB_H
