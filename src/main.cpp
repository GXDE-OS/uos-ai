#include "oscontrol/osinfo.h"
#include "utils/esystemcontext.h"

#include <QtDBus>

#include <DLog>
#include <DApplicationSettings>
#include <DGuiApplicationHelper>
#include "serverwrapper.h"
#include "application.h"

int main(int argc, char *argv[])
{
    UosInfo();

#ifdef QT_DEBUG
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "10777");
#endif
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--enable-logging --log-level=2 --no-sandbox");

    if (!QDBusConnection::sessionBus().isConnected()) {
        qCritical() << "Cannot connect to the D-Bus session bus." << QDBusConnection::sessionBus().lastError().message();
        return -1;
    }

    //if it already exists then just exit
    if (!ServerWrapper::registerService()) {
        return Application::handleExistingArgument(argc, argv);
    }

    if (ESystemContext::isWayland()) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");

        QProcess process;
        process.start("sh", QStringList() << "-c" << "dmidecode | grep -i \"String 4\"");
        process.waitForFinished();
        QString outputString = QString::fromLocal8Bit(process.readAllStandardOutput());
        //以下机型释放webengien时会在mali驱动中闪退，华为海思修复改问题会带来系统整体性能问题，所以我们通过不使用客户端缓存来针对性解决闪退问题
        if (outputString.contains("PWC30")) {
            qputenv("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION", "null");
        }

        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");
    }

    Application a(argc, argv);
    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    DApplicationSettings setting;

    //OpenGL config
    ESystemContext::configOpenGL();

    a.initialization();

    return a.exec();
}
