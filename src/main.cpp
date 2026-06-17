#include "oscontrol/osinfo.h"
#include "utils/esystemcontext.h"
#include "app/serverwrapper.h"
#include "app/application.h"
#include "wordwizard/wordwizard.h"
#include "utils/dconfigmanager.h"
#include "gui/mgmt/private/welcomedialog.h"
#include "utils/report/eventlogutil.h"
#include "utils/globalfilewatcher.h"
#include "dbus/dbusinterface.h"
#include "network/networkproxyhelper.h"

#ifdef QT_DEBUG
#include "network/websocketforwardserver.h"
#endif

// 3.0
#include "gui/window/windowmanager.h"
#include "model/modelvendor.h"
#include "database/appdatabase.h"
#include "datamigration/migrationmanager3.h"
#include "externalllm/externalpluginmanager.h"
#include "conversation/conversationsearch.h"

#include <QtDBus>

#include <DLog>
#include <DGuiApplicationHelper>

// Add logging category declaration
Q_DECLARE_LOGGING_CATEGORY(logMain)

using namespace uos_ai;

static QString getDmiInfo() {
    QString result;
    QProcess process;
    process.start("dmidecode", QStringList());
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    // 过滤
    QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        if (line.contains("String 4", Qt::CaseInsensitive)) {
            result.append(line);
        }
    }

    return result;
}

int main(int argc, char *argv[])
{
    qCInfo(logMain) << "Starting UOS AI application";
    srand(time(nullptr));

    UosInfo();

#ifdef QT_DEBUG
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "10777");
    qCInfo(logMain) << "Enabled remote debugging for WebEngine";
    WebSocketForwardServer wsServer(8081);
    QMetaObject::invokeMethod(&wsServer, "start", Qt::QueuedConnection);
#endif

#if defined(COMPILE_ON_V25) && defined(Q_PROCESSOR_SW_64)
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--enable-logging --log-level=2 --no-sandbox --js-flags=--jitless --disable-gpu");
#else
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--enable-logging --log-level=2 --no-sandbox");
#endif

    Application a(argc, argv);

    ProxySettingsWrapper::enableQtSystemProxyConfiguration();
    qCInfo(logMain) << "Enabled Qt system proxy configuration";

    if (!QDBusConnection::sessionBus().isConnected()) {
        qCCritical(logMain) << "Cannot connect to the D-Bus session bus:"
                          << QDBusConnection::sessionBus().lastError().message();
        return -1;
    }

    if (Application::handleWordWizardArgument(argc, argv)) {
        QDBusConnection connection = QDBusConnection::sessionBus();
        bool isServiceRegistered = connection.interface()->isServiceRegistered(DBUS_SERVER);
        if (!isServiceRegistered) {
            qCCritical(logMain) << "Cannot launch the WordWizard without starting UOS AI";
            return -1;
        }
    }

    if (!ServerWrapper::registerService()) {
        qCInfo(logMain) << "Service already registered, handling existing arguments";
        //无命令行参数执行uos-ai，触发单实例时走--chat逻辑，用于autostart.desktop激活窗口
        if (argc < 2) {
            QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
            QString error = notification.call("launchChatPage").errorMessage();
            if (!error.isEmpty()) {
                qCCritical(logMain) << "Failed to launch chat page via D-Bus:" << error;
                return -1;
            }
            return 0;
        } else {
            return Application::handleExistingArgument(argc, argv);
        }
    }

#ifndef COMPILE_ON_V25
    if (ESystemContext::isWayland()) {
        qCInfo(logMain) << "Running on Wayland, configuring environment";
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");

        QString outputString = getDmiInfo();
        //以下机型释放webengien时会在mali驱动中闪退，华为海思修复改问题会带来系统整体性能问题，所以我们通过不使用客户端缓存来针对性解决闪退问题
        if (outputString.contains("PWC30")) {
            qCWarning(logMain) << "Detected special hardware, disabling client buffer integration";
            qputenv("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION", "null");
        }

        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");
    }
#endif

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();
    qCInfo(logMain) << "Registered log appenders";

    // 初始化文件监视器
    GFWatcher();

    UosInfo()->printInfo();

    //config file
    DConfigManager::instance();

    //OpenGL config
    ESystemContext::configOpenGL();

    // 迁移数据库与聊天记录
    {
        MigrationManager3 migration;
        bool need = migration.checkMigrations();
        // 初始化数据库
        AppDatabase::instance()->init();
        if (need)
            migration.runMigrations();
    }

    // 搜索文件初始化：3.0老用户适配
    ConversationSearch::instance()->initSearchIndexes();

    // 初始化埋点
    ReportIns();

    // 初始化账号服务
    ExternalPluginManager::instance()->init();
    ModelVendor::instance()->refresh();

    // 提前初始化
    WelcomeDialog::instance();

    QPointer<WordWizard> wizard(new WordWizard());

    a.initWordWizard(wizard.data());
    a.initialization();

    qCInfo(logMain) << "UOS AI initialization completed, entering main event loop";
    return a.exec();
}
