// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COZECHATCOMPLETION_H
#define COZECHATCOMPLETION_H

#include "basenetwork.h"

namespace uos_ai {

class CozeChatCompletion : public BaseNetWork
{
public:
    explicit CozeChatCompletion(const LLMServerProxy &account);
    QPair<int, QString> ensureToken(QString &token);
    QPair<int, QString> create(const QJsonArray &msg);
protected:
    QString getUniqueIdentifier();
    QString generateJWTToken();
    virtual QUrl apiUrl() const;
    virtual QString botId() const;
private:
    static QPair<QString, qint64> oauthToken;
    static QString userID;
    LLMServerProxy severPorxy;
};

}
#endif // COZECHATCOMPLETION_H
