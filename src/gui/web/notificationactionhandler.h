// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NOTIFICATIONACTIONHANDLER_H
#define NOTIFICATIONACTIONHANDLER_H

#include <QObject>

namespace uos_ai {

class SystemChannel;

// DBus 服务常量定义
namespace NotificationDBus {
    inline constexpr char SERVICE_NAME[] = "org.deepin.copilot.Notification";
    inline constexpr char OBJECT_PATH[] = "/org/deepincopilot/Notification/Callback";
    inline constexpr char INTERFACE_NAME[] = "org.deepin.copilot.Notification.Callback";
    inline constexpr char METHOD_NAME[] = "HandleAction";
}

class NotificationActionHandler : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.copilot.Notification.Callback")
public:
    explicit NotificationActionHandler(QObject *parent = nullptr);

public slots:
    Q_SCRIPTABLE void HandleAction(const QString &actionId);
};

} // namespace uos_ai

#endif // NOTIFICATIONACTIONHANDLER_H
