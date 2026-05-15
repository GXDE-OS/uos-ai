#include "serverwrapper.h"
#include "dbusinterface.h"
#include "networkmonitor.h"
#include "chatdbusinterface.h"

#include <QtDBus>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logWrapper)

using namespace uos_ai;

ServerWrapper::ServerWrapper()
{

}

ServerWrapper *ServerWrapper::instance()
{
    static ServerWrapper wrapper;
    return &wrapper;
}

bool ServerWrapper::registerService()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    bool isServiceRegistered = connection.interface()->isServiceRegistered(DBUS_SERVER);
    if (isServiceRegistered) {
        qCDebug(logWrapper) << "Service already registered:" << DBUS_SERVER;
        return false;
    }

    if (!connection.registerService(DBUS_SERVER)) {
        QDBusError error = connection.lastError();
        qCCritical(logWrapper) << "Failed to register DBus service:" << DBUS_SERVER << "error:" << error.message();
        return false;
    }

    return true;
}

bool ServerWrapper::initialization()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    m_copilotDbusObject.reset(new DBusInterface(this));

    //Register the dbus object used for registration
    if (!connection.registerObject(DBUS_SERVER_PATH, m_copilotDbusObject.data(), QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        QDBusError error = connection.lastError();
        qCCritical(logWrapper) << "Failed to register DBus path:" << DBUS_SERVER_PATH << "error:" << error.message();
        return false;
    }

    m_chatDbusObject.reset(new ChatDBusInterface(this));
    //Register the dbus object used for registration
    if (!connection.registerObject(QString("/org/deepin/copilot/chat"), m_chatDbusObject.data(),
                                   QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        QDBusError error = connection.lastError();
        qCCritical(logWrapper) << "Failed to register chat DBus path error:" << error.message();
    }

#if 1
    QString deepinAIAssistantFilePath = "/usr/bin/DeepinAIAssistant";
    QFile deepinAIAssistantFile(deepinAIAssistantFilePath);
    if (!deepinAIAssistantFile.exists()) {
        m_audioAiassistant  = new AudioAiassistant(this);
        if (m_audioAiassistant->registerInterface()) {
            qCInfo(logWrapper) << "AudioAiassistant registered successfully";
        } else {
            qCWarning(logWrapper) << "Failed to register AudioAiassistant";
        }
    }
#endif

    qCInfo(logWrapper) << "Server initialization completed";
    return true;
}

void ServerWrapper::updateVisibleState(bool visible)
{
    qCDebug(logWrapper) << "Updating visible state:" << visible;
    m_copilotDbusObject->updateVisibleState(visible);
}

void ServerWrapper::updateActiveState(bool active)
{
    qCDebug(logWrapper) << "Updating active state:" << active;
    m_copilotDbusObject->updateActiveState(active);
}
