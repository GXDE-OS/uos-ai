// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenshotservice.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QLoggingCategory>
#include <QVariantMap>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

ScreenshotService *ScreenshotService::instance()
{
    static ScreenshotService ins;
    return &ins;
}

int ScreenshotService::checkAvailability()
{
    // -1: 截图按钮不显示
    // 0: 截图功能可用
    // 1: 需要升级高版本截图
    // 2: 正在录屏
#ifdef COMPILE_ON_V25
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qCWarning(logService) << "Can not connect sessionBus.";
        return 1;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.Screenshot",
                                                     "/com/deepin/Screenshot",
                                                     "org.freedesktop.DBus.Introspectable",
                                                     "Introspect");

    QDBusReply<QString> screenShotReply = connection.call(msg);
    if (screenShotReply.isValid() && screenShotReply.value().contains("CustomScreenshot")) {
        qCInfo(logService) << "The CustomScreenshot interface exists.";
    } else {
        qCWarning(logService) << "The CustomScreenshot interface is not found.";
        return 1;
    }

    QDBusInterface dbusInterface("com.deepin.ScreenRecorder",
                                 "/com/deepin/ScreenRecorder",
                                 "org.freedesktop.DBus.Properties",
                                 QDBusConnection::sessionBus());

    QDBusReply<QDBusVariant> screenRecorderReply = dbusInterface.call("Get", "com.deepin.ScreenRecorder", "IsRecording");

    if (screenRecorderReply.isValid()) {
        return screenRecorderReply.value().variant().toBool() ? 2 : 0;
    }
    return 0;
#endif
    return -1;
}

void ScreenshotService::start()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    // 监听Screenshot信号
    connection.connect("com.deepin.Screenshot", "/com/deepin/Screenshot", "com.deepin.Screenshot", "CustomDone", this, SLOT(onCustomDone(QString)));

    QDBusInterface screenshotInterface(
        "com.deepin.Screenshot",
        "/com/deepin/Screenshot",
        "com.deepin.Screenshot"
        );

    QVariantMap options;
    options["showToolBar"] = true;

    // 发起异步 DBus 调用
    QDBusPendingCall pendingCall = screenshotInterface.asyncCall(
        "CustomScreenshot",
        options
        );

    // 创建 watcher 来监听异步调用结果
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &ScreenshotService::onCallFinished);
}

void ScreenshotService::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<void> reply = *watcher;

    if (reply.isError()) {
        qCWarning(logService) << "Screenshot async call failed:" << reply.error().message();
    } else {
        qCInfo(logService) << "Screenshot async call completed successfully";
    }

    // 清理 watcher
    watcher->deleteLater();
}

void ScreenshotService::onCustomDone(const QString &imagePath)
{
    qCInfo(logService) << "Screenshot ImagePath:" << imagePath;

    if (imagePath.isEmpty()) {
        emit canceled();
    } else {
        emit done(imagePath);
    }

    // 断开对 CustomDone 信号的监听
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.disconnect("com.deepin.Screenshot", "/com/deepin/Screenshot", "com.deepin.Screenshot", "CustomDone", this, SLOT(onCustomDone(QString)));
}

ScreenshotService::ScreenshotService(QObject *parent)
    : QObject(parent)
{
}
