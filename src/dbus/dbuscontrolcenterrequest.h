#ifndef DBUSCONTROLCENTERREQUEST_H
#define DBUSCONTROLCENTERREQUEST_H

#include "dbusrequestbase.h"

class DbusControlCenterRequest : public DbusRequestBase
{
    Q_OBJECT

public:
    explicit DbusControlCenterRequest(QObject *parent = nullptr);

    void showPage(const QString &module, const QString &page);

public slots:
    void slotCallFinished(CDBusPendingCallWatcher *) override;

};

#endif // DBUSCONTROLCENTERREQUEST_H
