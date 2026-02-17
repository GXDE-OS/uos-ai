#include "deepincontrolcenter.h"
#include "oscallcontext.h"

#include <QDBusPendingCall>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

UOSAI_USE_NAMESPACE

DeepinControlCenter::DeepinControlCenter(QObject *parent) : QObject{parent}
{
    m_controlCenterProxy.reset(
        new QDBusInterface(
            osCallDbusCtrCenterService,
            osCallDbusCtrCentertPath,
            osCallDbusCtrCenterInterface,
            QDBusConnection::sessionBus(), this));
}

int DeepinControlCenter::ShowModule(const QString &module)
{
    qCDebug(logOsControl) << "Showing control center module:" << module;
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
            qCWarning(logOsControl) << "ShowModule failed - module:" << module
                                  << "error:" << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        qCWarning(logOsControl) << "Control center proxy is null";
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}

int DeepinControlCenter::ShowPage(const QString &module, const QString &page)
{
    qCDebug(logOsControl) << "Showing control center page - module:" << module << "page:" << page;
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
            qCWarning(logOsControl) << "ShowPage failed - module:" << module
                                  << "page:" << page
                                  << "error:" << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        qCWarning(logOsControl) << "Control center proxy is null";
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}

int DeepinControlCenter::ShowPage(const QString &url)
{
    qCDebug(logOsControl) << "Showing control center page with URL:" << url;
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
            qCWarning(logOsControl) << "ShowPage failed - URL:" << url
                                  << "error:" << reply.error().errorString(reply.error().type());

            if (QDBusError::UnknownMethod == reply.error().type()
                    || QDBusError::InvalidArgs == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        qCWarning(logOsControl) << "Control center proxy is null";
        errorCode = OSCallContext::NonService;
    }
    return errorCode;
}
