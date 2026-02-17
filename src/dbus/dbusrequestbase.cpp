#include "dbusrequestbase.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

DbusRequestBase::DbusRequestBase(const QString &service, const QString &path, const QString &interface, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, interface.toStdString().c_str(), connection, parent)
{
    //关联后端dbus触发信号
    if (!QDBusConnection::sessionBus().connect(this->service(), this->path(), this->interface(), "", this, SLOT(slotDbusCall(QDBusMessage)))) {
        qCWarning(logDBus) << "Failed to connect DBus signal - path:" << this->path() << "interface:" << this->interface();
    }
    
    //关联后端dbus触发信号
    if (!QDBusConnection::sessionBus().connect(this->service(), this->path(), "org.freedesktop.DBus.Properties", "", this, SLOT(slotDbusCall(QDBusMessage)))) {
        qCWarning(logDBus) << "Failed to connect DBus Properties signal - path:" << this->path() << "interface:" << this->interface();
    }
}

void DbusRequestBase::setCallbackFunc(CallbackFunc func)
{
    qCDebug(logDBus) << "Setting callback function";
    m_callbackFunc = func;
}

/**
 * @brief DbusRequestBase::asyncCall
 * 异步访问dbus接口
 * @param method    dbus方法名
 * @param args  参数
 */
void DbusRequestBase::asyncCall(const QString &method, const QList<QVariant> &args)
{
    qCDebug(logDBus) << "Making async DBus call - method:" << method;
    
    QDBusPendingCall async = QDBusAbstractInterface::asyncCall(method, args);
    CDBusPendingCallWatcher *watcher = new CDBusPendingCallWatcher(async, method, this);
    //将回调函数放进CallWatcher中，随CallWatcher调用结果返回
    watcher->setCallbackFunc(m_callbackFunc);
    //清楚回调函数，防止多方法调用时混淆
    setCallbackFunc(nullptr);
    connect(watcher, &CDBusPendingCallWatcher::signalCallFinished, this, &DbusRequestBase::slotCallFinished);
}

/**
 * @brief DbusRequestBase::asyncCall
 * 异步访问dbus接口
 * @param method    dbus方法名
 * @param args  参数
 */
void DbusRequestBase::asyncCall(const QString &method,
                                const QVariant &arg1, const QVariant &arg2, const QVariant &arg3, const QVariant &arg4,
                                const QVariant &arg5, const QVariant &arg6, const QVariant &arg7, const QVariant &arg8)
{
    qCDebug(logDBus) << "Making async DBus call with multiple arguments - method:" << method;
    asyncCall(method, method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

/**
 * @brief DbusRequestBase::asyncCall
 * 异步访问dbus接口
 * @param method    dbus方法名
 * @param args  参数
 */
void DbusRequestBase::asyncCall(const QString &method, const QString &callName,
                                const QVariant &arg1, const QVariant &arg2, const QVariant &arg3, const QVariant &arg4,
                                const QVariant &arg5, const QVariant &arg6, const QVariant &arg7, const QVariant &arg8)
{
    qCDebug(logDBus) << "Making async DBus call with named method - method:" << method << "callName:" << callName;
    
    QList<QVariant> argList;
    int count = 0 + arg1.isValid() + arg2.isValid() + arg3.isValid() + arg4.isValid() +
                arg5.isValid() + arg6.isValid() + arg7.isValid() + arg8.isValid();

    switch (count) {
    case 8:
        argList.prepend(arg8);
    case 7:
        argList.prepend(arg7);
    case 6:
        argList.prepend(arg6);
    case 5:
        argList.prepend(arg5);
    case 4:
        argList.prepend(arg4);
    case 3:
        argList.prepend(arg3);
    case 2:
        argList.prepend(arg2);
    case 1:
        argList.prepend(arg1);
    }
    QDBusPendingCall async = QDBusAbstractInterface::asyncCallWithArgumentList(method, argList);
    CDBusPendingCallWatcher *watcher = new CDBusPendingCallWatcher(async, callName, this);
    //将回调函数放进CallWatcher中，随CallWatcher调用结果返回
    watcher->setCallbackFunc(m_callbackFunc);
    //清楚回调函数，防止多方法调用时混淆
    setCallbackFunc(nullptr);
    connect(watcher, &CDBusPendingCallWatcher::signalCallFinished, this, &DbusRequestBase::slotCallFinished);
}

/**
 * @brief slotDbusCall
 * dbus服务端调用
 * @param msg 调用消息
 */
void DbusRequestBase::slotDbusCall(const QDBusMessage &)
{
}
