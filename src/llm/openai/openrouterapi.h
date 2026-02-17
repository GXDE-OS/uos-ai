// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef OPENROUTERAPI_H
#define OPENROUTERAPI_H

#include "llm.h"
#include "uosai_global.h"

namespace uos_ai {

class OpenRouterAPI : public LLM
{
public:
    Q_OBJECT
public:
    OpenRouterAPI(const LLMServerProxy &serverproxy);
    QPair<int, QString> verify() override;
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    virtual QList<QByteArray> text2Image(const QString &prompt, int number) override;
protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
private:
    QString modelId() const;
    QString apiUrl() const;
};

}

#endif // OPENROUTERAPI_H
