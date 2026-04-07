// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSFREECONVERSATION_H
#define UOSFREECONVERSATION_H

#include "uosai_global.h"
#include "conversation.h"

namespace uos_ai {

class UosFreeConversation : public Conversation
{
public:
    UosFreeConversation();

public:
    void update(const QByteArray &response);

    static QJsonObject parseContentString(const QString &content);
};

}
#endif // UOSFREECONVERSATION_H
