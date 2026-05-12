#include "dbusinterface.h"
#include "appdbusobject.h"
#include "util.h"
#include "browswenativedbusobject.h"
#include "database/appdatabase.h"
#include "app/application.h"

#include <QtDBus>
#include <QDebug>
#include <QtConcurrent>
#include <QThreadPool>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)
using namespace uos_ai;

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent)
    , QDBusContext()
{

}

DBusInterface::~DBusInterface()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(DBUS_SERVER);
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

QString DBusInterface::adjustDbusPath(QString appId)
{
    return DBUS_SERVER_PATH + QString("/") + "a_" + appId.replace(QRegularExpression("[^A-Za-z]"), "_");
}

QString DBusInterface::version()
{
    return "3.0";
}

bool DBusInterface::queryUserExpState()
{
    qCWarning(logDBus) <<"queryUserExpState is deprecated";
    return false;
}

QString DBusInterface::cachedFunctions()
{
    qCWarning(logDBus) <<"cachedFunctions is deprecated";
    return "";
}

void DBusInterface::launchChatPage(int index)
{
    qCDebug(logDBus) << "Launching chat page with index:" << index;
    QMetaObject::invokeMethod(aiApp, "launchChatWindow", Qt::QueuedConnection, Q_ARG(int, index));
}

void DBusInterface::launchWordWizard()
{
    qCDebug(logDBus) << "Launching word wizard";
    QMetaObject::invokeMethod(aiApp, "launchWordWizard", Qt::QueuedConnection);
}

void DBusInterface::textTranslation()
{
    qCDebug(logDBus) << "Initiating text translation";
    QMetaObject::invokeMethod(aiApp, "showTranslate", Qt::QueuedConnection);
}

void DBusInterface::startScreenshot()
{
    qCDebug(logDBus) << "Initiating screenshot for AI analysis";
    QMetaObject::invokeMethod(aiApp, "startScreenshot", Qt::QueuedConnection);
}

bool DBusInterface::isCopilotEnabled()
{
    bool enabled = AppDatabase::instance()->getConfigBool(CONFIG_APP_AGREEMENT);
    qCDebug(logDBus) << "Checking copilot status:" << enabled;
    return enabled;
}

void DBusInterface::launchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath)
{
    qCDebug(logDBus) << "upload screenshot to aiquickdialog";
    QMetaObject::invokeMethod(aiApp, "launchAiQuick", Qt::QueuedConnection, 
            Q_ARG(int, type), Q_ARG(QString, query), Q_ARG(QPoint, pos), Q_ARG(bool, isCustom), Q_ARG(QString, imagePath));
}

void DBusInterface::launchChatUploadImage(const QString &imagePath)
{
    qCDebug(logDBus) << "upload screenshot to chatwindow";
    QMetaObject::invokeMethod(aiApp, "uploadImage", Qt::QueuedConnection, Q_ARG(QString, imagePath));
}

void DBusInterface::launchLLMUiPage(bool openAddAccountDialog)
{
    qCDebug(logDBus) << "Launching LLM UI page, openAddAccountDialog:" << openAddAccountDialog;
    QMetaObject::invokeMethod(aiApp, "showConfig", Qt::QueuedConnection, Q_ARG(int, MgmtWindow::Page::ModelList));
}

QStringList DBusInterface::registerAppCmdPrompts(const QVariantMap &/*cmdPrompts*/)
{
    // 保留参数，此版本先不用
    QVariantMap cmdPrompts;

    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    const QString &appId = Util::queryProcessName(pid);

    if (appId.isEmpty()) {
        qCWarning(logDBus) << "Empty appId received during registration";
        return QStringList();
    }

    QDBusConnection connection = QDBusConnection::sessionBus();
    const QString &path = adjustDbusPath(appId);

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

    // 浏览器专属VIP接口
    if (appId == "browser_native") {
        object.object.reset(new BrowsweNativeDbusObject(appId));
    } else {
        object.object.reset(new AppDbusObject(appId));
    }

    m_appDbusObjects[appId] = object;

    connection.unregisterObject(object.path);
    if (!connection.registerObject(object.path, object.object.data(), QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        QDBusError error = connection.lastError();
        qCCritical(logDBus) << "Failed to register DBus path:" << object.path << "error:" << error.message();
    } else {
        qCDebug(logDBus) << "Successfully registered DBus path:" << object.path;
    }

    //DbWrapper::localDbWrapper().appendApp(object);

    QStringList reply;
    reply << path;

    return reply;
}

QString DBusInterface::registerApp()
{
    auto list = registerAppCmdPrompts(QVariantMap());
    return list.isEmpty() ? "" : list.first();
}

void DBusInterface::unregisterApp()
{
    qCDebug(logDBus) << "Unregistering app";
    unregisterAppCmdPrompts();
}

void DBusInterface::unregisterAppCmdPrompts()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    const QString &appId = Util::queryProcessName(pid);

    QDBusConnection connection = QDBusConnection::sessionBus();
    const QString &path = adjustDbusPath(appId);
    connection.unregisterObject(path);

    m_appDbusObjects.remove(appId);
    qCDebug(logDBus) << "Unregistered app:" << appId << "path:" << path;
}
