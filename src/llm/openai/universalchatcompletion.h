// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UNIVERSALCHATCOMPLETION_H
#define UNIVERSALCHATCOMPLETION_H

#include "uosai_global.h"
#include "ainetwork.h"
#include "aiconversation.h"

namespace uos_ai {

class UniversalChatCompletion : public AINetWork
{
public:
    UniversalChatCompletion(const QString &url, const AccountProxy &account);
    QString rootUrlPath() const override;
    QPair<int, QString> create(const QString &model, AIConversation &conversation, qreal temperature = 1.0);
private:
    QString rootUrl;
};

}


#endif // UNIVERSALCHATCOMPLETION_H
