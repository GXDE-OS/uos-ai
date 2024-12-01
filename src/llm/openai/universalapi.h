// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UNIVERSALAPI_H
#define UNIVERSALAPI_H

#include "llm.h"
#include "uosai_global.h"

namespace uos_ai {

class UniversalAPI : public LLM
{
    Q_OBJECT
public:
    UniversalAPI(const LLMServerProxy &serverproxy);
    QPair<int, QString> verify() override;
    QJsonObject predict(const QString &content, const QJsonArray &functions, const QString &systemRole = "", qreal temperature = 1.0) override;
    virtual QList<QByteArray> text2Image(const QString &prompt, int number) override;
protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
private:
    QString modelId() const;
    QString apiUrl() const;
};

}

#endif // UNIVERSALAPI_H
