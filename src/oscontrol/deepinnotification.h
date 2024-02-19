#ifndef DEEPINNOTIFICATION_H
#define DEEPINNOTIFICATION_H
#include "notificationability.h"

#include <QObject>

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>

class DeepinNotification : public QObject,  public INotification
{
    Q_OBJECT
public:
    explicit DeepinNotification(bool isLinglong, QObject *parent = nullptr);

    int SetSystemInfo(int param, QVariant data) override;
signals:
protected:
    QScopedPointer<QDBusInterface> m_uosNotificationProxy;

    bool m_fIsLinglong;
};

#endif // DEEPINNOTIFICATION_H
