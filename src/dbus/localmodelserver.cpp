#include "localmodelserver.h"
#include "app/application.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

#define NM_SERVICE      "com.home.appstore.client"
#define NM_PATH         "/com/home/appstore/client"
#define NM_INTERFACE    "com.home.appstore.client"

LocalModelServer::LocalModelServer(QObject *parent) : QObject(parent)
{
    appStoreInterface = new QDBusInterface(NM_SERVICE,
                                   NM_PATH,
                                   NM_INTERFACE,
                                   QDBusConnection::sessionBus());
}

LocalModelServer::~LocalModelServer()
{
    delete appStoreInterface;
}

LocalModelServer &LocalModelServer::getInstance()
{
    static LocalModelServer instance;
    return instance;
}

void LocalModelServer::openInstallWidget(const QString &appname)
{
    QDBusMessage reply = appStoreInterface->call("openBusinessUri", QVariant::fromValue(QString("app_detail_info/%1").arg(appname)));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to open install widget:" << reply.errorMessage();
    }
}

void LocalModelServer::openManagerWidget()
{
    QDBusMessage reply = appStoreInterface->call("openBusinessUri", QVariant::fromValue(QString("tab/manager")));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to open manager widget:" << reply.errorMessage();
    }
}

bool LocalModelServer::checkInstallStatus(const QString &appName)
{
    QDir dir("/var/lib/dpkg/info");
    bool b = !dir.entryList(QStringList() << appName + "*.list").isEmpty();
    qCDebug(logDBus) << "Checking install status for app:" << appName << b;
    return b;

#if 0
    QProcess m_pProcess;
    m_pProcess.start("dpkg-query", QStringList() << "-W" << QString("-f='${db:Status-Status}\n'") << appName);
    m_pProcess.waitForFinished();
    QByteArray reply = m_pProcess.readAllStandardOutput();
    bool InstallStatus = (reply == "'installed\n'" ? true : false);
    return InstallStatus;
#else
    return QFileInfo::exists(QString("/usr/bin/%0").arg(appName));
#endif
}

void LocalModelServer::openInstallWidgetOnTimer(const QString &appname)
{
    qCDebug(logDBus) << "Opening install widget with timer for app:" << appname;

    aiApp->initMgmtWindow();

    openInstallWidget(appname);
    QTimer::singleShot(2000, this, [=](){
        emit beginCheck(appname, 720);//启动定时器检查安装状态
    });
}
