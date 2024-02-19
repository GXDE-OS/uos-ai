#ifndef DBUSREQUESTBASE_H
#define DBUSREQUESTBASE_H

#include "cdbuspendingcallwatcher.h"

#include <QDBusAbstractInterface>
#include <QDBusPendingReply>

class DbusRequestBase: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    explicit DbusRequestBase(const QString &service, const QString &path, const QString &interface,
                             const QDBusConnection &connection = QDBusConnection::sessionBus(),
                             QObject *parent = nullptr);

    //设置回调函数
    void setCallbackFunc(CallbackFunc func);

public slots:
    //dbus服务端调用
    virtual void slotDbusCall(const QDBusMessage &msg);
    //dbus调用完成事件
    virtual void slotCallFinished(CDBusPendingCallWatcher *) = 0;

protected:
    //异步调用，包装异步调用事件
    void asyncCall(const QString &method, const QList<QVariant> &args);
    void asyncCall(const QString &method,
                   const QVariant &arg1 = QVariant(),
                   const QVariant &arg2 = QVariant(),
                   const QVariant &arg3 = QVariant(),
                   const QVariant &arg4 = QVariant(),
                   const QVariant &arg5 = QVariant(),
                   const QVariant &arg6 = QVariant(),
                   const QVariant &arg7 = QVariant(),
                   const QVariant &arg8 = QVariant());
    void asyncCall(const QString &method, const QString &callName,
                   const QVariant &arg1 = QVariant(),
                   const QVariant &arg2 = QVariant(),
                   const QVariant &arg3 = QVariant(),
                   const QVariant &arg4 = QVariant(),
                   const QVariant &arg5 = QVariant(),
                   const QVariant &arg6 = QVariant(),
                   const QVariant &arg7 = QVariant(),
                   const QVariant &arg8 = QVariant());

private:
    CallbackFunc m_callbackFunc = nullptr; //回调函数
};

#endif // DBUSREQUESTBASE_H
