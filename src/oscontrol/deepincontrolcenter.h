#ifndef DEEPINCONTROLCENTER_H
#define DEEPINCONTROLCENTER_H
#include "controlcenterability.h"

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>

class DeepinControlCenter : public QObject, public IControlCenter
{
    Q_OBJECT
public:
    explicit DeepinControlCenter(QObject *parent = nullptr);

signals:

    // IControlCenter interface
public:
    int ShowModule(const QString &module) override;
    int ShowPage(const QString &module, const QString &page) override;
    int ShowPage(const QString &url) override;

protected:
    QScopedPointer<QDBusInterface> m_controlCenterProxy;
};

#endif // DEEPINCONTROLCENTER_H
