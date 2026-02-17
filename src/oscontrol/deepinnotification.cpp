#include "deepinnotification.h"
#include "oscallcontext.h"

#include <QDBusPendingCall>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

UOSAI_USE_NAMESPACE

DeepinNotification::DeepinNotification(QObject *parent) : QObject{parent}
{
    /*V23
     * qdbus --literal
     *      org.deepin.dde.Notification1
     *      /org/deepin/dde/Notification1
     *      org.deepin.dde.Notification1.SetSystemInfo 0 true
     *
     * */
    m_uosNotificationProxy.reset(
        new QDBusInterface(
            osCallDbusNotifyService,
            osCallDbusNotifyPath,
            osCallDbusNotifyInterface,
            QDBusConnection::sessionBus(), this));
}

int DeepinNotification::SetSystemInfo(int param, QVariant data)
{
    qCDebug(logOsControl) << "Setting system info - param:" << param << "data:" << data;
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
        reply.waitForFinished();

        if (reply.isError()) {
            qCWarning(logOsControl) << "SetSystemInfo failed - param:" << param
                                  << "error:" << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        qCWarning(logOsControl) << "Notification proxy interface is invalid";
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}
