// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GEMINICHATCOMPLETION_H
#define GEMINICHATCOMPLETION_H

#include "uosai_global.h"
#include "ainetwork.h"

#include "geminiconversation.h"

namespace uos_ai {

class GeminiChatCompletion : public BaseNetWork
{
public:
    GeminiChatCompletion(const QString &url, const AccountProxy &account);
    QPair<int, QString> create(const QString &model, GeminiConversation &conversation, const QVariantHash &params);
private:
    QString rootUrl;
};

}
#endif // GEMINICHATCOMPLETION_H
