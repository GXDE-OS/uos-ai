#include "ddedockobject.h"
#include "osinfo.h"
#include "oscallcontext.h"

#include <QLoggingCategory>
#include <QtDBus>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

UOSAI_USE_NAMESPACE

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
    m_dbus.reset(new QDBusInterface(osCallDbusDockService, osCallDbusDockPath, osCallDbusDockInterface, session));
    
#ifdef COMPILE_ON_V23
    qRegisterMetaType<DockRect>("DockRect");
    qDBusRegisterMetaType<DockRect>();
    session.connect(m_dbus->service(), m_dbus->path(),
                    m_dbus->interface(), "FrontendWindowRectChanged",
                    this, SLOT(onFrontendWindowRectChanged(DockRect)));
#else
    session.connect(m_dbus->service(), m_dbus->path(),
                    "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    this, SLOT(propertiesChanged(QString, QVariantMap, QStringList)));
#endif
}

void DDeDockObject::onFrontendWindowRectChanged(DockRect rect) {
    qCDebug(logDBus) << "Frontend window rect changed:" << rect.rect();
    emit FrontendWindowRectChanged(rect.rect());
}

void DDeDockObject::propertiesChanged(QString interface, QVariantMap changedProperties, QStringList)
{
    if (interface != m_dbus->interface()) {
        return;
    }

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
        qCWarning(logDBus) << "Failed to get dock position";
    }

    return 0;
}

int DDeDockObject::displayMode()
{
    QVariant reply = m_dbus->property("DisplayMode");
    if (reply.isValid()) {
        return reply.toInt();
    } else {
        qCWarning(logDBus) << "Failed to get dock display mode";
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
        qCWarning(logDBus) << "Failed to get frontend window rect";
    }

    return QRect();
}
