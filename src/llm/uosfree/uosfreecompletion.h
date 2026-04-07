// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSFREECOMPLETION_H
#define UOSFREECOMPLETION_H

#include "uosai_global.h"
#include "ainetwork.h"
#include "uosfreeconversation.h"

namespace uos_ai {

class UosFreeCompletion : public BaseNetWork
{
public:
    UosFreeCompletion(const QString &url, const AccountProxy &account);

    QPair<int, QString> create(const QString &model, UosFreeConversation &conversation, const QVariantHash &params);

private:
    QString rootUrl;
    QJsonArray transformFunctionList(const QJsonArray &inputArray);
};

}
#endif // UOSFREECOMPLETION_H
