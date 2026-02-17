#include "dbusinterface.h"
#include "appdbusobject.h"
#include "llmutils.h"
#include "browswenativedbusobject.h"
#include "dbwrapper.h"
#include "threadtaskmana.h"

#include <QtDBus>
#include <QDebug>
#include <QtConcurrent>
#include <QThreadPool>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent)
    , QDBusContext()
{
    qRegisterMetaType<DBusInterface::TaskType>("TaskType");
    m_appDbusObjects = DbWrapper::localDbWrapper().queryAppList();

    connect(this, &DBusInterface::sigTask, this, &DBusInterface::onProcessTask, Qt::QueuedConnection);
}

DBusInterface::~DBusInterface()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(DBUS_SERVER);
}

void DBusInterface::onProcessTask(DBusInterface::TaskType type)
{
    qCDebug(logDBus) << "Processing task type:" << type;
    if (type == TASK_UPDATE_LLM_ACCOUNT) {
        updateLLMAccount();
    }
}

void DBusInterface::onRequestTaskFinished()
{
    QFutureWatcher<QString> *watcher = dynamic_cast<QFutureWatcher<QString> *>(sender());
    if (watcher) {
        LLMThreadTaskMana::instance()->requestTaskFinished(watcher->result());
    }
    sender()->deleteLater();
}

void DBusInterface::updateLLMAccount()
{
    qCDebug(logDBus) << "Updating LLM accounts for all apps";
    for (auto iter = m_appDbusObjects.begin(); iter != m_appDbusObjects.end(); iter++) {
        QSharedPointer<AppDbusObject> appDbusObj = iter.value().object;
        if (appDbusObj.isNull()) {
            qCWarning(logDBus) << "Null app DBus object found for app:" << iter.key();
            continue;
        }

        appDbusObj->updateLLMAccount();
    }
}

void DBusInterface::asyncUpdateLLMAccount()
{
    qCDebug(logDBus) << "Scheduling async LLM account update";
    emit sigTask(TASK_UPDATE_LLM_ACCOUNT);
}

void DBusInterface::updateUserExpState(int state)
{
    qCDebug(logDBus) << "Updating user experience state:" << state;
    emit userExpStateChanged(state > 0);
}

void DBusInterface::updateVisibleState(bool visible)
{
    qCDebug(logDBus) << "Updating window visibility:" << visible;
    emit windowVisibleChanged(visible);
}

void DBusInterface::updateActiveState(bool active)
{
    qCDebug(logDBus) << "Updating window active state:" << active;
    emit windowActiveChanged(active);
}

void DBusInterface::addAppFunction(const QString &appId, const QJsonObject &funciton)
{
    qCDebug(logDBus) << "Adding function for app:" << appId;
    m_appFunctions[appId] << funciton;
}

QJsonArray DBusInterface::appFunctions()
{
    const QString &appId = qApp->applicationName();
    const QJsonArray &functions = m_appFunctions.value(appId);
    m_appFunctions.remove(appId);
    qCDebug(logDBus) << "Retrieved functions for app:" << appId << "count:" << functions.size();
    return functions;
}

QString DBusInterface::version()
{
    return "1.1";
}

bool DBusInterface::queryUserExpState()
{
    bool state = DbWrapper::localDbWrapper().getUserExpState() > 0;
    qCDebug(logDBus) << "Querying user experience state:" << state;
    return state;
}

QString DBusInterface::cachedFunctions()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    QString appId = LLMUtils::queryAppId(pid);
    const QJsonArray &functions = m_appFunctions.value(appId);
    if (functions.isEmpty()) {
        qCDebug(logDBus) << "No cached functions found for app:" << appId;
        return QString();
    }

    QJsonObject rootObject;
    rootObject["functions"] = functions;
    QJsonDocument jsonDocument(rootObject);
    m_appFunctions.remove(appId);
    qCDebug(logDBus) << "Retrieved cached functions for app:" << appId << "count:" << functions.size();
    return jsonDocument.toJson(QJsonDocument::Compact);
}

void DBusInterface::launchChatPage(int index)
{
    qCDebug(logDBus) << "Launching chat page with index:" << index;
    emit sigToLaunchChat(index);
}

void DBusInterface::launchWordWizard()
{
    qCDebug(logDBus) << "Launching word wizard";
    emit sigToLaunchWordWizard();
}

void DBusInterface::textTranslation()
{
    qCDebug(logDBus) << "Initiating text translation";
    emit sigToTranslate();
}

void DBusInterface::startScreenshot()
{
    qCDebug(logDBus) << "Initiating screenshot for AI analysis";
    emit sigToStartScreenshot();
}

bool DBusInterface::isCopilotEnabled()
{
    bool enabled = DbWrapper::localDbWrapper().getAICopilotIsOpen();
    qCDebug(logDBus) << "Checking copilot status:" << enabled;
    return enabled;
}

void DBusInterface::launchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath)
{
    qCDebug(logDBus) << "upload screenshot to aiquickdialog";
    emit sigToLaunchAiQuickOCR(type, query, pos, isCustom, imagePath);
}

void DBusInterface::launchChatUploadImage(const QString &imagePath)
{
    qCDebug(logDBus) << "upload screenshot to chatwindow";
    emit sigToUploadImage(imagePath);
}

void DBusInterface::launchLLMUiPage(bool openAddAccountDialog)
{
    qCDebug(logDBus) << "Launching LLM UI page, openAddAccountDialog:" << openAddAccountDialog;
    emit sigToLaunchMgmt(openAddAccountDialog);
}

QStringList DBusInterface::registerAppCmdPrompts(const QVariantMap &/*cmdPrompts*/)
{
    // 保留参数，此版本先不用
    QVariantMap cmdPrompts;

    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    const QString &appId = LLMUtils::queryAppId(pid);

    if (appId.isEmpty()) {
        qCWarning(logDBus) << "Empty appId received during registration";
        return QStringList();
    }

    QDBusConnection connection = QDBusConnection::sessionBus();
    const QString &path = LLMUtils::adjustDbusPath(appId);

    if (m_appDbusObjects.contains(appId) && !m_appDbusObjects[appId].object.isNull()) {
        if (m_appDbusObjects[appId].cmdPrompts == cmdPrompts && connection.objectRegisteredAt(path)) {
            qCDebug(logDBus) << "App already registered at path:" << path;

            QStringList reply;
            reply << path;

            return reply;
        }

        qCDebug(logDBus) << "Aborting existing execution for app:" << appId;
        m_appDbusObjects[appId].object->executionAborted();
    }

    AppDbusPathObject object = m_appDbusObjects.value(appId);
    object.appId = appId;
    object.path = path;
    object.cmdPrompts = cmdPrompts;
    object.curLLMId = DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appId);

    // 浏览器专属VIP接口
    if (appId == "browser_native") {
        object.object.reset(new BrowsweNativeDbusObject(appId));
    } else {
        object.object.reset(new AppDbusObject(appId));
    }

    connect(object.object.data(), &AppDbusObject::launchUI, this, &DBusInterface::sigToLaunchMgmt, Qt::QueuedConnection);

    m_appDbusObjects[appId] = object;

    connection.unregisterObject(object.path);
    if (!connection.registerObject(object.path, object.object.data(), QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        QDBusError error = connection.lastError();
        qCCritical(logDBus) << "Failed to register DBus path:" << object.path << "error:" << error.message();
    } else {
        qCDebug(logDBus) << "Successfully registered DBus path:" << object.path;
    }

    DbWrapper::localDbWrapper().appendApp(object);

    QStringList reply;
    reply << path;

    return reply;
}

QString DBusInterface::registerApp()
{
    return registerAppCmdPrompts(QVariantMap()).value(0);
}

void DBusInterface::unregisterApp()
{
    qCDebug(logDBus) << "Unregistering app";
    unregisterAppCmdPrompts();
}

void DBusInterface::unregisterAppCmdPrompts()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    const QString &appId = LLMUtils::queryAppId(pid);

    QDBusConnection connection = QDBusConnection::sessionBus();
    const QString &path = LLMUtils::adjustDbusPath(appId);
    connection.unregisterObject(path);

    m_appDbusObjects.remove(appId);
    qCDebug(logDBus) << "Unregistered app:" << appId << "path:" << path;
}
