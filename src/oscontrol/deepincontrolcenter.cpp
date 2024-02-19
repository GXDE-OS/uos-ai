#include "deepincontrolcenter.h"
#include "oscallcontext.h"

#include <QDebug>
#include <QDBusPendingCall>

DeepinControlCenter::DeepinControlCenter(bool isLinglong, QObject *parent)
    : QObject{parent}
    , m_fIsLinglong(isLinglong)
{
    QString deepinNotifyService;
    QString deepinNotifyPath;
    QString deepinNotifyInterface;

    /*V23
     * ControlCenter version:6.0.28
     *     only have ShowPage(url) to show modules
     *
     * qdbus --literal
     *      org.deepin.dde.ControlCenter1
     *      /org/deepin/dde/ControlCenter1
     *      org.deepin.dde.ControlCenter1.ShowPage "network"
     *
     * */
    if (isLinglong) {
        deepinNotifyService = QString("org.deepin.dde.ControlCenter1");
        deepinNotifyPath = QString("/org/deepin/dde/ControlCenter1");
        deepinNotifyInterface = deepinNotifyService;
    } else {
        deepinNotifyService = QString("com.deepin.dde.ControlCenter");
        deepinNotifyPath = QString("/com/deepin/dde/ControlCenter");
        deepinNotifyInterface = deepinNotifyService;
    }

    m_controlCenterProxy.reset(
        new QDBusInterface(
            deepinNotifyService,
            deepinNotifyPath,
            deepinNotifyInterface,
            QDBusConnection::sessionBus(), this));
}

int DeepinControlCenter::ShowModule(const QString &module)
{
    int errorCode = OSCallContext::NonError;

    if (!m_controlCenterProxy.isNull()) {
        //TODO:
        // Now we only open the disturb switch.
        //May need set other options.
        //
        //Modify: 2023/11/29
        //    Make the code compatible for 1030 system.
        //
        //ControlCenter:V5.X api
        // void -- com.deepin.dde.ControlCenter.ShowModule(QString module)
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(module);

        auto reply = m_controlCenterProxy->asyncCallWithArgumentList(
                         QStringLiteral("ShowModule"), argumentList);
        reply.waitForFinished();
        if (reply.isError()) {
            qInfo() << QString("ShowModule(QString module=%1) call failed:").arg(module)
                    << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
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

int DeepinControlCenter::ShowPage(const QString &module, const QString &page)
{
    int errorCode = OSCallContext::NonError;

    if (!m_controlCenterProxy.isNull()) {
        //TODO:
        // Now we only open the disturb switch.
        //May need set other options.
        //
        //Modify: 2023/11/29
        //    Make the code compatible for 1030 system.
        //
        //ControlCenter:V5.X api
        //void -- com.deepin.dde.ControlCenter.ShowPage(QString module, QString page)
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(module)
                     << QVariant::fromValue<QString>(page);

        auto reply = m_controlCenterProxy->asyncCallWithArgumentList(
                         QStringLiteral("ShowPage"), argumentList);
        reply.waitForFinished();
        if (reply.isError()) {

            qInfo() << QString("ShowPage(QString module=%1, QString page=%2) call failed:")
                    .arg(module).arg(page)
                    << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
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

int DeepinControlCenter::ShowPage(const QString &url)
{
    int errorCode = OSCallContext::NonError;

    if (!m_controlCenterProxy.isNull()) {
        //TODO:
        // Now we only open the disturb switch.
        //May need set other options.
        //
        //Modify: 2023/11/29
        //    Make the code compatible for 1030 system.
        //
        //ControlCenter:V6.0 api
        //void -- com.deepin.dde.ControlCenter.ShowPage(QString url)
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(url);

        auto reply = m_controlCenterProxy->asyncCallWithArgumentList(
                         QStringLiteral("ShowPage"), argumentList);
        reply.waitForFinished();
        if (reply.isError()) {
            qInfo() << "ShowPage(QString url) call failed:"
                    << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
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
