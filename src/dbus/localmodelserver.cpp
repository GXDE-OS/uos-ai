#include "localmodelserver.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#include <QProcess>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

#define NM_SERVICE      "com.home.appstore.client"
#define NM_PATH         "/com/home/appstore/client"
#define NM_INTERFACE    "com.home.appstore.client"

LocalModelServer::LocalModelServer(QObject *parent) : QObject(parent)
{

}

LocalModelServer::~LocalModelServer()
{

}

LocalModelServer &LocalModelServer::getInstance()
{
    static LocalModelServer instance;
    return instance;
}

void LocalModelServer::openInstallWidget(const QString &appname)
{
    auto con = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE, NM_PATH,
                                   NM_INTERFACE, "openBusinessUri");

    QVariantList args;
    args << QVariant::fromValue(QString("app_detail_info/%1").arg(appname));
    msg.setArguments(args);

    con.asyncCall(msg);

    QDBusMessage reply = con.call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to open install widget:" << reply.errorMessage();
    }
}

void LocalModelServer::openManagerWidget()
{
    auto con = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE, NM_PATH,
                                   NM_INTERFACE, "openBusinessUri");

    QVariantList args;
    args << QVariant::fromValue(QString("tab/manager"));
    msg.setArguments(args);

    QDBusMessage reply = con.call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to open manager widget:" << reply.errorMessage();
    }
}

void LocalModelServer::localModelStatusChanged(const QString &app, bool isExist)
{
    qCDebug(logDBus) << "Local model status changed - app:" << app << "exists:" << isExist;
    
    if ("uos-ai-llm" == app)
        emit localLLMStatusChanged(isExist);
    if (PLUGINSNAME == app)
        emit modelPluginsStatusChanged(isExist);
}

bool LocalModelServer::checkInstallStatus(const QString &appName)
{
    qCDebug(logDBus) << "Checking install status for app:" << appName;
    
    QProcess m_pProcess;
    m_pProcess.start("dpkg-query", QStringList() << "-W" << QString("-f='${db:Status-Status}\n'") << appName);
    m_pProcess.waitForFinished();
    QByteArray reply = m_pProcess.readAllStandardOutput();
    bool InstallStatus = (reply == "'installed\n'" ? true : false);
    return InstallStatus;
}

void LocalModelServer::openInstallWidgetOnTimer(const QString &appname)
{
    qCDebug(logDBus) << "Opening install widget with timer for app:" << appname;
    
    emit sigToLaunchMgmtNoShow();
    openInstallWidget(appname);
    QTimer::singleShot(2000, this, [=](){
        emit sigToLaunchTimer(720);//启动定时器检查安装状态
    });
}
