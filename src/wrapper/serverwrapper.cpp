#include "serverwrapper.h"
#include "llmutils.h"
#include "dbusinterface.h"
#include "networkdefs.h"
#include "session.h"
#include "dbwrapper.h"
#include "networkmonitor.h"

#include <QtDBus>

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
    if (isServiceRegistered)
        return false;

    if (!connection.registerService(DBUS_SERVER)) {
        QDBusError error = connection.lastError();
        qCritical() << "Failed to register DBus service:" << error.message();
        return false;
    }

    return true;
}

bool ServerWrapper::initialization()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    DbWrapper::localDbWrapper().initialization(DbWrapper::getDatabaseDir());

    m_copilotDbusObject.reset(new DBusInterface());

    //Register the dbus object used for registration
    if (!connection.registerObject(DBUS_SERVER_PATH, m_copilotDbusObject.data(), QDBusConnection::ExportScriptableSlots)) {
        QDBusError error = connection.lastError();
        qCritical() << "Failed to register DBus path:" << error.message();
        return false;
    }

    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchMgmt, this, &ServerWrapper::sigToLaunchMgmt, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchChat, this, &ServerWrapper::sigToLaunchChat, Qt::QueuedConnection);

    return true;
}

QSharedPointer<Session> ServerWrapper::createChatSession()
{
    if (m_copilotSeesion.isNull()) {
        m_copilotSeesion.reset(new Session(qApp->applicationName(), this));
    }

    return m_copilotSeesion;
}

void ServerWrapper::updateLLMAccount()
{
    m_copilotDbusObject->asyncUpdateLLMAccount();

    if (!m_copilotSeesion.isNull()) {
        m_copilotSeesion->updateLLMAccount();
    }
}

void ServerWrapper::updateUserExpState(int state)
{
    m_copilotDbusObject->updateUserExpState(state);
}

void ServerWrapper::addAppFunction(const QString &appId, const QJsonObject &funciton)
{
    m_copilotDbusObject->addAppFunction(appId, funciton);
}

QPair<int, QString> ServerWrapper::verify(const LLMServerProxy &serverProxy)
{
    QSharedPointer<LLM> copilot = LLMUtils::getCopilot(serverProxy);
    if (copilot.isNull()) {
        return qMakePair(AIServer::ContentAccessDenied, QString("Invalid LLM Account."));
    }

    const QPair<int, QString> &result = copilot->verify();
    if (result.first != AIServer::NoError && !NetworkMonitor::getInstance().isOnline()) {
        return qMakePair(AIServer::NetworkError, QCoreApplication::translate("ServerWrapper", "Connection failed, please check the network."));
    }

    return result;
}
