#ifndef DEEPINCONTROLCENTER_H
#define DEEPINCONTROLCENTER_H

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>

namespace uos_ai {

class DeepinControlCenter : public QObject
{
    Q_OBJECT
public:
    explicit DeepinControlCenter(QObject *parent = nullptr);

signals:

    // IControlCenter interface
public:
    int ShowModule(const QString &module);
    int ShowPage(const QString &module, const QString &page);
    int ShowPage(const QString &url);

protected:
    QScopedPointer<QDBusInterface> m_controlCenterProxy;
};

} // namespace uos_ai
#endif // DEEPINCONTROLCENTER_H
