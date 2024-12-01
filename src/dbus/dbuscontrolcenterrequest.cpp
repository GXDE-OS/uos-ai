#include "dbuscontrolcenterrequest.h"
#include <QDebug>

#ifdef COMPILE_ON_V23
#define CONTROL_CENTER_SERVICE      "org.deepin.dde.ControlCenter1"
#define CONTROL_CENTER_PATH         "/org/deepin/dde/ControlCenter1"
#define CONTROL_CENTER_INTERFACE    "org.deepin.dde.ControlCenter1"
#else
#define CONTROL_CENTER_SERVICE      "com.deepin.dde.ControlCenter"
#define CONTROL_CENTER_PATH         "/com/deepin/dde/ControlCenter"
#define CONTROL_CENTER_INTERFACE    "com.deepin.dde.ControlCenter"
#endif

DbusControlCenterRequest::DbusControlCenterRequest(QObject *parent)
    : DbusRequestBase(CONTROL_CENTER_SERVICE, CONTROL_CENTER_PATH, CONTROL_CENTER_INTERFACE, QDBusConnection::sessionBus(), parent)
{

}

void DbusControlCenterRequest::showPage(const QString &module, const QString &page)
{
    asyncCall("ShowPage", QVariant(module), QVariant(page));
}

void DbusControlCenterRequest::showPage(const QString &page)
{
    asyncCall("ShowPage", QVariant(page));
}

void DbusControlCenterRequest::slotCallFinished(CDBusPendingCallWatcher *call)
{
    if (call->isError()) {
        qWarning() << call->reply().member() << call->error().message();
    }

    call->deleteLater();
}
