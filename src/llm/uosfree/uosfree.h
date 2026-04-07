// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSFREE_H
#define UOSFREE_H

#include "llm.h"
#include "uosai_global.h"

namespace uos_ai {

class UosFree : public LLM
{
    Q_OBJECT
public:
    inline QString baseUrl() {
        return QString("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
    }

    inline QString searchUrl() {
        return QString("https://ark.cn-beijing.volces.com/api/v3/bots/chat/completions");
    }

    inline QString modelId() {
        return QString("glm-4-7-251222");
    }

    inline QString searchBotId() {
        return QString("bot-20250321110601-77w5l");
    }

    explicit UosFree(const LLMServerProxy &serverproxy);

    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    QPair<int, QString> verify() override;
    QJsonObject onlineSearch(const QString &content);

protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
};

}
#endif // UOSFREE_H
