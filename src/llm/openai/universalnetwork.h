// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UNIVERSALNETWORK_H
#define UNIVERSALNETWORK_H

#include "uosai_global.h"
#include "ainetwork.h"
#include "aiconversation.h"

namespace uos_ai {

class UniversalNetWork : public AINetWork
{
public:
    UniversalNetWork(const QString &url, const AccountProxy &account);
    QString rootUrlPath() const override;
    QPair<int, QString> create(AIConversation &conversation, const QVariantHash &params, const QJsonObject &jparams);
private:
    QString rootUrl;
};

}

#endif // UNIVERSALNETWORK_H
