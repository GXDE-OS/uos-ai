#include "serverwrapper.h"
#include "llmutils.h"
#include "dbusinterface.h"
#include "networkdefs.h"
#include "session.h"
#include "dbwrapper.h"
#include "networkmonitor.h"

#include <QtDBus>

UOSAI_USE_NAMESPACE

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

    m_copilotDbusObject.reset(new DBusInterface(this));

    //Register the dbus object used for registration
    if (!connection.registerObject(DBUS_SERVER_PATH, m_copilotDbusObject.data(), QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        QDBusError error = connection.lastError();
        qCritical() << "Failed to register DBus path:" << error.message();
        return false;
    }

    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchMgmt, this, &ServerWrapper::sigToLaunchMgmt, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchChat, this, &ServerWrapper::sigToLaunchChat, Qt::QueuedConnection);


    // 先屏蔽助手的兼容接口
#if 0
    m_aiassistant  = new AiassistantSubstitute(this);
    if (m_aiassistant->registerInterface())
        qInfo() << "Aiassistant register successfully.";
    else
        qWarning() << "Fail to register aiassistant.";
#endif
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

void ServerWrapper::updateVisibleState(bool visible)
{
    m_copilotDbusObject->updateVisibleState(visible);
}

void ServerWrapper::updateActiveState(bool active)
{
    m_copilotDbusObject->updateActiveState(active);
}

QPair<int, QString> ServerWrapper::verify(const LLMServerProxy &serverProxy)
{
    QSharedPointer<LLM> copilot = LLMUtils::getCopilot(serverProxy);
    if (copilot.isNull()) {
        return qMakePair(AIServer::ContentAccessDenied, QString("Invalid LLM Account."));
    }

    const QPair<int, QString> &result = copilot->verify();

    return result;
}
