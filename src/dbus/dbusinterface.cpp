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

DBusInterface::DBusInterface(QObject *parent)
    : QThread(parent)
{
    qRegisterMetaType<DBusInterface::TaskType>("TaskType");
    m_appDbusObjects = DbWrapper::localDbWrapper().queryAppList();

    connect(this, &DBusInterface::sigTask, this, &DBusInterface::onProcessTask, Qt::QueuedConnection);

    moveToThread(this);
    start();
}

DBusInterface::~DBusInterface()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(DBUS_SERVER);

    quit();
    wait();
}

void DBusInterface::onProcessTask(DBusInterface::TaskType type)
{
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
    for (auto iter = m_appDbusObjects.begin(); iter != m_appDbusObjects.end(); iter++) {
        QSharedPointer<AppDbusObject> appDbusObj = iter.value().object;
        if (appDbusObj.isNull())
            continue;

        appDbusObj->updateLLMAccount();
    }
}

void DBusInterface::asyncUpdateLLMAccount()
{
    emit sigTask(TASK_UPDATE_LLM_ACCOUNT);
}

void DBusInterface::updateUserExpState(int state)
{
    emit userExpStateChanged(state > 0);
}

void DBusInterface::updateVisibleState(bool visible)
{
    emit windowVisibleChanged(visible);
}

void DBusInterface::updateActiveState(bool active)
{
    emit windowActiveChanged(active);
}

void DBusInterface::addAppFunction(const QString &appId, const QJsonObject &funciton)
{
    m_appFunctions[appId] << funciton;
}

QJsonArray DBusInterface::appFunctions()
{
    const QString &appId = qApp->applicationName();
    const QJsonArray &functions = m_appFunctions.value(appId);
    m_appFunctions.remove(appId);
    return functions;
}

QString DBusInterface::version()
{
    return "1.1";
}

bool DBusInterface::queryUserExpState()
{
    return DbWrapper::localDbWrapper().getUserExpState() > 0;
}

QString DBusInterface::cachedFunctions()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    QString appId = LLMUtils::queryAppId(pid);
    const QJsonArray &functions = m_appFunctions.value(appId);
    if (functions.isEmpty())
        return QString();

    QJsonObject rootObject;
    rootObject["functions"] = functions;
    QJsonDocument jsonDocument(rootObject);
    m_appFunctions.remove(appId);
    return jsonDocument.toJson(QJsonDocument::Compact);
}

void DBusInterface::launchChatPage(int index)
{
    emit sigToLaunchChat(index);
}

void DBusInterface::launchLLMUiPage(bool openAddAccountDialog)
{
    emit sigToLaunchMgmt(openAddAccountDialog);
}

QStringList DBusInterface::registerAppCmdPrompts(const QVariantMap &/*cmdPrompts*/)
{
    // 保留参数，此版本先不用
    QVariantMap cmdPrompts;

    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    const QString &appId = LLMUtils::queryAppId(pid);

    if (appId.isEmpty())
        return QStringList();

    QDBusConnection connection = QDBusConnection::sessionBus();
    const QString &path = LLMUtils::adjustDbusPath(appId);

    if (m_appDbusObjects.contains(appId) && !m_appDbusObjects[appId].object.isNull()) {
        if (m_appDbusObjects[appId].cmdPrompts == cmdPrompts && connection.objectRegisteredAt(path)) {
            qDebug() << path << " registered!";

            QStringList reply;
            reply << path;

            return reply;
        }

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
        qCritical() << "Failed to register DBus path: path, error = " << error.message();
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
}
