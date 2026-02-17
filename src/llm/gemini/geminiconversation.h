// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GEMINICONVERSATION_H
#define GEMINICONVERSATION_H

#include "uosai_global.h"

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace uos_ai {

class GeminiConversation
{
public:
    explicit GeminiConversation();
    bool setChatContent(const QString &content);
    bool setSystemData(const QString &sys);
    bool setFunctions(const QJsonArray &functions);
    QJsonArray getChatContent() const;
    QJsonObject getSystemData() const;
    QString getLastResponse() const;
    void update(const QByteArray &response);
    static QJsonObject parseContentString(const QString &content);
protected:
    QJsonObject m_system;
    QJsonArray m_contents;
    QJsonArray m_functions;
};

}
#endif // GEMINICONVERSATION_H
