// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationactionhandler.h"
#include "systemchannel.h"

#include <QLoggingCategory>
#include <QDBusConnection>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

NotificationActionHandler::NotificationActionHandler(QObject *parent)
    : QObject(parent)
{
    auto *systemChannel = qobject_cast<SystemChannel *>(parent);
    if (!systemChannel) {
        qCWarning(logAIGUI) << "NotificationActionHandler: parent is not SystemChannel";
        return;
    }

    if (!QDBusConnection::sessionBus().registerService(NotificationDBus::SERVICE_NAME)) {
        qCWarning(logAIGUI) << "Failed to register DBus service:" << NotificationDBus::SERVICE_NAME
                             << QDBusConnection::sessionBus().lastError().message();
        return;
    }

    if (!QDBusConnection::sessionBus().registerObject(NotificationDBus::OBJECT_PATH, this,
            QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        qCWarning(logAIGUI) << "Failed to register DBus object:" << NotificationDBus::OBJECT_PATH
                             << QDBusConnection::sessionBus().lastError().message();
        return;
    }

    qCDebug(logAIGUI) << "NotificationActionHandler registered:" << NotificationDBus::SERVICE_NAME << NotificationDBus::OBJECT_PATH;
}

void NotificationActionHandler::HandleAction(const QString &actionId)
{
    if (actionId.isEmpty()) {
        qCWarning(logAIGUI) << "HandleAction called with empty actionId";
        return;
    }

    qCDebug(logAIGUI) << "Notification action from panel:" << actionId;

    auto *systemChannel = qobject_cast<SystemChannel *>(parent());
    if (!systemChannel) {
        qCWarning(logAIGUI) << "HandleAction: cannot find SystemChannel";
        return;
    }

    // 直接调用 SystemChannel 的 emitNotificationAction 槽函数
    QMetaObject::invokeMethod(systemChannel, "emitNotificationAction", Qt::QueuedConnection,
        Q_ARG(QString, actionId));
}
