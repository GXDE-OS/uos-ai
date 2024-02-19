#include "ddedockobject.h"
#include "osinfo.h"

#include <QtDBus>

struct DockRect {
    int x;
    int y;
    int w;
    int h;

    QRect rect() const
    {
        return QRect(x, y, w, h);
    };
};

Q_DECLARE_METATYPE(DockRect)

QDBusArgument &operator<<(QDBusArgument &argument, const DockRect &rect)
{
    argument.beginStructure();
    argument << rect.x << rect.y << rect.w << rect.h;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DockRect &rect)
{
    argument.beginStructure();
    argument >> rect.x >> rect.y >> rect.w >> rect.h;
    argument.endStructure();
    return argument;
}

DDeDockObject::DDeDockObject(QObject *parent)
    : QObject(parent)
{
    QDBusConnection session = QDBusConnection::sessionBus();
    bool isLingLong = UosInfo()->isLingLong();
    if (isLingLong) {
        m_dbus.reset(new QDBusInterface("org.deepin.dde.daemon.Dock1", "/org/deepin/dde/daemon/Dock1", "org.deepin.dde.daemon.Dock1", session));
    } else {
        m_dbus.reset(new QDBusInterface("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", "com.deepin.dde.daemon.Dock", session));
    }

    session.connect(m_dbus->service(), m_dbus->path(),
                    "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    this, SLOT(propertiesChanged(QString, QVariantMap, QStringList)));
}

void DDeDockObject::propertiesChanged(QString interface, QVariantMap changedProperties, QStringList)
{
    if (interface != m_dbus->interface())
        return;

    for (auto iter = changedProperties.begin(); iter != changedProperties.end(); iter++) {
        if (iter.key() == "FrontendWindowRect") {
            DockRect rect = qdbus_cast<DockRect>(iter.value());
            emit FrontendWindowRectChanged(rect.rect());
        }
    }
}

int DDeDockObject::position()
{
    QVariant reply = m_dbus->property("Position");
    if (reply.isValid()) {
        return reply.toInt();
    } else {
        qWarning() << "Failed to get Position:" << reply;
    }

    return 0;
}

int DDeDockObject::displayMode()
{
    QVariant reply = m_dbus->property("DisplayMode");
    if (reply.isValid()) {
        return reply.toInt();
    } else {
        qWarning() << "Failed to get Position:" << reply;
    }

    return 0;
}

QRect DDeDockObject::frontendWindowRect()
{
    QDBusInterface dbusInterface(m_dbus->service(), m_dbus->path(), "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());
    QDBusReply<QDBusVariant> reply = dbusInterface.call("Get", m_dbus->interface(), "FrontendWindowRect");
    if (reply.isValid()) {
        return qdbus_cast<DockRect>(reply.value().variant()).rect();
    } else {
        qWarning() << "Failed to get FrontendWindowRect:" << reply.error().message();
    }

    return QRect();
}
