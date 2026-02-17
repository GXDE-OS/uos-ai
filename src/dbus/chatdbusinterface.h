// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHATDBUSINTERFACE_H
#define CHATDBUSINTERFACE_H

#include "uosai_global.h"

#include <QObject>
#include <QDBusContext>

class ServerWrapper;
UOSAI_BEGIN_NAMESPACE

class ChatDBusInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.copilot.chat")
public:
    explicit ChatDBusInterface(ServerWrapper *parent = nullptr);

signals:

public slots:
    Q_SCRIPTABLE void inputPrompt(const QString &question, const QMap<QString, QString> &params);
    Q_SCRIPTABLE void sendToKnowledgeBase(const QStringList &file);
private:
    ServerWrapper *m_server = nullptr;
};

UOSAI_END_NAMESPACE

#endif // CHATDBUSINTERFACE_H
