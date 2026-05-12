#ifndef DEEPINNOTIFICATION_H
#define DEEPINNOTIFICATION_H

#include <QObject>

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>
namespace uos_ai {
class DeepinNotification : public QObject
{
    Q_OBJECT
public:
    explicit DeepinNotification(QObject *parent = nullptr);

    int SetSystemInfo(int param, QVariant data);
signals:
protected:
    QScopedPointer<QDBusInterface> m_uosNotificationProxy;

    bool m_fIsLinglong;
};
} // namespace uos_ai

#endif // DEEPINNOTIFICATION_H
