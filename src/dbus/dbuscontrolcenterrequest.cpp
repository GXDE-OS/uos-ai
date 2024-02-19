#include "dbuscontrolcenterrequest.h"
#include <QDebug>

DbusControlCenterRequest::DbusControlCenterRequest(QObject *parent)
    : DbusRequestBase("com.deepin.dde.ControlCenter", "/com/deepin/dde/ControlCenter", "com.deepin.dde.ControlCenter", QDBusConnection::sessionBus(), parent)
{

}

void DbusControlCenterRequest::showPage(const QString &module, const QString &page)
{
    asyncCall("ShowPage", QVariant(module), QVariant(page));
}

void DbusControlCenterRequest::slotCallFinished(CDBusPendingCallWatcher *call)
{
    if (call->isError()) {
        qWarning() << call->reply().member() << call->error().message();
    }

    call->deleteLater();
}
