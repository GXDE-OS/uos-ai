#include "serverwrapper.h"
#include "llmutils.h"
#include "dbusinterface.h"
#include "networkdefs.h"
#include "session.h"
#include "dbwrapper.h"
#include "networkmonitor.h"
#include "chatdbusinterface.h"
#include "chatbotservice.h"

#include <QtDBus>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logWrapper)

UOSAI_USE_NAMESPACE
using namespace uos_ai::chatbot;

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

    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchMgmt, this, &ServerWrapper::sigToLaunchMgmt, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchChat, this, &ServerWrapper::sigToLaunchChat, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchWordWizard, this, &ServerWrapper::sigToLaunchWordWizard, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToTranslate, this, &ServerWrapper::sigToTranslate, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToStartScreenshot, this, &ServerWrapper::sigToStartScreenshot, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToLaunchAiQuickOCR, this, &ServerWrapper::sigToLaunchAiQuickOCR, Qt::QueuedConnection);
    connect(m_copilotDbusObject.data(), &DBusInterface::sigToUploadImage, this, &ServerWrapper::sigToUploadImage, Qt::QueuedConnection);

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

#ifdef ENABLE_CHATBOT
    // 启动 IM 机器人服务（读取 chatbot.json，按配置决定是否启动）
    m_chatBotService = new ChatBotService(this);
    QSharedPointer<Session> chatbotSession;
    chatbotSession.reset(new Session(qApp->applicationName(), this));
    m_chatBotService->initialize(chatbotSession);
#endif

    return true;
}

QSharedPointer<Session> ServerWrapper::createChatSession()
{
    if (m_copilotSeesion.isNull()) {
        qCDebug(logWrapper) << "Creating new chat session for application:" << qApp->applicationName();
        m_copilotSeesion.reset(new Session(qApp->applicationName(), this));
    }

    return m_copilotSeesion;
}

void ServerWrapper::updateLLMAccount()
{
    qCDebug(logWrapper) << "Updating LLM account";
    m_copilotDbusObject->asyncUpdateLLMAccount();

    if (!m_copilotSeesion.isNull()) {
        m_copilotSeesion->updateLLMAccount();
    }
}

void ServerWrapper::updateUserExpState(int state)
{
    qCDebug(logWrapper) << "Updating user experience state:" << state;
    m_copilotDbusObject->updateUserExpState(state);
}

void ServerWrapper::addAppFunction(const QString &appId, const QJsonObject &funciton)
{
    qCDebug(logWrapper) << "Adding function for app:" << appId;
    m_copilotDbusObject->addAppFunction(appId, funciton);
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

QPair<int, QString> ServerWrapper::verify(const LLMServerProxy &serverProxy)
{
    qCDebug(logWrapper) << "Verifying server proxy:" << serverProxy.id;
    QSharedPointer<LLM> copilot = LLMUtils::getCopilot(serverProxy);
    if (copilot.isNull()) {
        qCWarning(logWrapper) << "Invalid LLM account:" << serverProxy.id;
        return qMakePair(AIServer::ContentAccessDenied, QString("Invalid LLM Account."));
    }

    const QPair<int, QString> &result = copilot->verify();

    return result;
}
