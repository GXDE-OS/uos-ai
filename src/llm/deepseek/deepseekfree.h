// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEEPSEEKFREE_H
#define DEEPSEEKFREE_H

#include "deepseekai.h"
#include "uosai_global.h"

namespace uos_ai {

class DeepSeekFree : public DeepSeekAI
{
public:
    inline QString baseUrl() override {
        return QString("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
    }

    inline QString searchUrl() {
        return QString("https://ark.cn-beijing.volces.com/api/v3/bots/chat/completions");
    }

    inline QString modelId() override {
        return QString("deepseek-v3-2-251201");
    }

    inline QString searchBotId() {
        return QString("bot-20250321110601-77w5l");
    }

    explicit DeepSeekFree(const LLMServerProxy &serverproxy);
    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    QJsonObject onlineSearch(const QString &content);
};

}
#endif // DEEPSEEKFREE_H
