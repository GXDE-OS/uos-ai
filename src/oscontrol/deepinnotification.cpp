#include "deepinnotification.h"
#include "oscallcontext.h"

#include <QDebug>
#include <QDBusPendingCall>

DeepinNotification::DeepinNotification(bool isLinglong, QObject *parent)
    : QObject{parent}
    , m_fIsLinglong(isLinglong)
{
    QString deepinNotifyService;
    QString deepinNotifyPath;
    QString deepinNotifyInterface;

    /*V23
     * qdbus --literal
     *      org.deepin.dde.Notification1
     *      /org/deepin/dde/Notification1
     *      org.deepin.dde.Notification1.SetSystemInfo 0 true
     *
     * */
    if (isLinglong) {
        deepinNotifyService = QString("org.deepin.dde.Notification1");
        deepinNotifyPath = QString("/org/deepin/dde/Notification1");
        deepinNotifyInterface = deepinNotifyService;
    } else {
        deepinNotifyService = QString("com.deepin.dde.Notification");
        deepinNotifyPath = QString("/com/deepin/dde/Notification");
        deepinNotifyInterface = deepinNotifyService;
    }

    m_uosNotificationProxy.reset(
        new QDBusInterface(
            deepinNotifyService,
            deepinNotifyPath,
            deepinNotifyInterface,
            QDBusConnection::sessionBus(), this));
}

int DeepinNotification::SetSystemInfo(int param, QVariant data)
{
    int errorCode = OSCallContext::NonError;

    if (m_uosNotificationProxy->isValid()) {
        //TODO:
        // Now we only open the disturb switch.
        //May need set other options.
        //
        //Modify: 2023/11/29
        //    Make the code compatible for 1030 system.
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<uint>(param)
                     << QVariant::fromValue<QDBusVariant>(QDBusVariant(data));

        auto reply = m_uosNotificationProxy->asyncCallWithArgumentList(
                         QStringLiteral("SetSystemInfo"), argumentList);

        if (reply.isError()) {
            qInfo() << "SetSystemInfo call failed:"
                    << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}
