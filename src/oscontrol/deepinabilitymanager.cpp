#include "deepinabilitymanager.h"
#include "deepinlauncher.h"
#include "deepincalendar.h"
#include "deepinnotification.h"
#include "deepincontrolcenter.h"
#include "osinfo.h"

#include <QApplication>
#include <QDBusVariant>
#include <QDesktopServices>
#include <QDBusPendingReply>

#include <DDesktopEntry>

DCORE_USE_NAMESPACE

UOSAbilityManager::UOSAbilityManager(QObject *parent)
    : QObject{parent}
{
    loadErrMap();
    //Get application paths
    initDesktopPaths();
    //Need load before init proxys
    loadApp2Desktop();

    initUosProxys();
}

OSCallContext UOSAbilityManager::doBluetoothConfig()
{
    OSCallContext ctx;
    int errCode;

    if (!m_fIsLinglong) {
        errCode = m_uosControlCenterProxy->ShowModule("bluetooth");
    } else {
        //V23 call V6.x api
        errCode = m_uosControlCenterProxy->ShowPage("bluetooth");
    }

    ctx.error = OSCallContext::CallError(errCode);
    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::doScreenMirroring(bool state)
{
    Q_UNUSED(state);

    // display:Casting
    // com.deepin.dde.ControlCenter
    // /com/deepin/dde/ControlCenter
    // com.deepin.dde.ControlCenter.ShowPage 'display' 'Casting'

    OSCallContext ctx;
    int errCode;

    if (!m_fIsLinglong) {
        errCode = m_uosControlCenterProxy->ShowPage("display", "Casting");
    } else {
        //V23 call V6.x api
        errCode = m_uosControlCenterProxy->ShowPage("display/Casting");
    }

    ctx.error = OSCallContext::CallError(errCode);
    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

/**
 * @brief UOSAbilityManager::doNoDisturb
 *   Ref:
 *   dde-control-center/
 *   src/frame/window/modules/notification/systemnotifywidget.cpp
 *   typedef enum {
 *        DNDMODE,  //bool
 *        LOCKSCREENOPENDNDMODE,    //bool
 *        OPENBYTIMEINTERVAL,   //bool
 *        STARTTIME,    //time.toString("hh:mm")
 *       ENDTIME        //time.toString("hh:mm")
 *    } SystemConfigurationItem;
 * @return
 */
OSCallContext UOSAbilityManager::doNoDisturb(bool state)
{
    OSCallContext ctx;

    typedef enum {
        DNDMODE,  //bool
        LOCKSCREENOPENDNDMODE,//bool
        OPENBYTIMEINTERVAL,   //bool
        STARTTIME,    //time.toString("hh:mm")
        ENDTIME        //time.toString("hh:mm")
    } SystemConfigurationItem;

    int errCode = m_uosNotificationProxy->SetSystemInfo(
                      SystemConfigurationItem::DNDMODE, state);

    ctx.error = OSCallContext::CallError(errCode);
    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::doWallpaperSwitch()
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    if (m_uosDesktopProxy->isValid()) {
        QList<QVariant> argumentList;;
        m_uosDesktopProxy->asyncCallWithArgumentList(
            QStringLiteral("ShowWallpaperChooser"), argumentList);
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doDesktopClearing(bool state)
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    QProcess process;
    QStringList args;
    args << "--set" << "-a" << "org.deepin.dde.file-manager"
         << "-r" << "org.deepin.dde.file-manager.desktop.organizer"
         << "-k" << "enableOrganizer"
         << "-v" << QString("%1").arg(static_cast<int>(state));

    process.start("dde-dconfig", args);

    // Wait for the process to finish
    process.waitForFinished();

    // Get the exit code of the process
    int exitCode = process.exitCode();

    if (exitCode != 0) {
        if (process.error() == QProcess::ProcessError::FailedToStart) {
            ctx.error = OSCallContext::NotImpl;
            ctx.errorInfo = m_errMap[ctx.error];
        } else {
            ctx.error = OSCallContext::NonService;
            ctx.errorInfo = m_errMap[ctx.error];
        }

        qWarning() << "Failed to execute dde-dconfig: state=" << state
                   << " exitCode=" << exitCode
                   << " stdout=" << QString::fromLocal8Bit(process.readAllStandardOutput())
                   << " stderr=" << QString::fromLocal8Bit(process.readAllStandardError());
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doDockModeSwitch(int mode)
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    if (m_uosDockProxy->isValid()) {
        if (mode >= 0 && mode < 2) {
            m_uosDockProxy->setDisplayMode(mode);
        } else {
            ctx.error = OSCallContext::InvalidArgs;
            ctx.errorInfo = m_errMap[ctx.error];
        }
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doSystemThemeSwitch(int theme)
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    if (m_uosAppearanceProxy->isValid()) {
        if (theme >= 0 && theme < 3) {
            switch (theme) {
            case 0:
                m_uosAppearanceProxy->Set("gtk", "deepin");
                break;
            case 1:
                m_uosAppearanceProxy->Set("gtk", "deepin-dark");
                break;
            case 2:
                m_uosAppearanceProxy->Set("gtk", "deepin-auto");
                break;
            default:
                break;
            }
        } else {
            ctx.error = OSCallContext::InvalidArgs;
            ctx.errorInfo = m_errMap[ctx.error];
        }
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doDiplayEyesProtection(bool on)
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    //Eyes Protection
    //TODO:
    //   Brightness:70%
    //   Temperatrue: pos [0-100] 20%
    //      int kelvin = pos > 50 ? (6500 - (pos - 50) * 100) : (6500 + (50 - pos) * 300);
    //   May need adjust other display parameter.
    if (m_uosDisplayProxy->isValid()) {
        if (on) {
            auto reply = m_uosDisplayProxy->SetMethodAdjustCCT(2);
            reply.waitForFinished();

            if (!reply.isError()) {
                int pos = 80;
                int kelvin = pos > 50 ? (6500 - (pos - 50) * 100) : (6500 + (50 - pos) * 300);
                m_uosDisplayProxy->SetColorTemperature(kelvin);
                reply.waitForFinished();

                if (reply.isError()) {
                    qWarning() << "doDiplayEyesProtection->SetColorTemperature failed:"
                               << reply.error();
                }
            } else {
                qWarning() << "doDiplayEyesProtection->Enble Temperatrue set failed:"
                           << reply.error();
            }
        } else {
            //Close the eyes protection mode
            // Brightness:80%
            // Temperatrue: disable
            auto reply = m_uosDisplayProxy->SetMethodAdjustCCT(0);
            reply.waitForFinished();

            if (reply.isError()) {
                qWarning() << "doDiplayEyesProtection->Disable Temperatrue set failed:"
                           << reply.error();
            }
        }

    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doDiplayBrightness(int value)
{
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    if (m_uosDisplayProxy->isValid()) {
        if (value >= 0 && value <= 100) {
            QString primaryDisplay = m_uosDisplayProxy->primary();
            m_uosDisplayProxy->SetAndSaveBrightness(
                primaryDisplay, value / 100.0);
        } else {
            ctx.error = OSCallContext::InvalidArgs;
            ctx.errorInfo = m_errMap[ctx.error];
        }
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doAppLaunch(const QString &appId, bool on)
{
    qInfo() << "UOSAbilityManager::doAppLaunch->" << appId;

    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;

    if (on) {
        //TODO:
        //    Mail, browser open the default application.
        if ("mail" == appId) {
            int defRet = m_uosAppLauncher->launchDefault("x-scheme-handler/mailto");

            ctx.error = OSCallContext::CallError(defRet);
        } else if ("browser" == appId) {
            //Try to get the default app by MIME service
            //this maybe failed for deepin MIME service
            //in some cases.
            int defRet = m_uosAppLauncher->launchDefault("x-scheme-handler/https");

            if (OSCallContext::NonError != defRet) {
                //if GetDefaultApp failed and there are other applications
                //try use Qt xdg-open to open the default appliction.
                int defApps = m_uosAppLauncher->listApps("x-scheme-handler/https");

                if (defApps != 0) {
                    if (!QDesktopServices::openUrl(QUrl("https://"))) {
                        ctx.error = OSCallContext::AppStartFailed;
                    }
                } else {
                    ctx.error = OSCallContext::AppStartFailed;
                }
            }
        } else {
            //Handle other app start except mail,browser
            auto iter = m_app2Desktop.find(appId);

            if (iter != m_app2Desktop.end()) {
                //Try to find the desktop file path.
                QString appDesktopFile;
                foreach (auto path, m_defaultDesktopPaths) {
                    foreach (auto d, iter->desktopFiles) {
                        QFileInfo f(path + "/" + d);

                        if (f.isFile() && f.exists()) {
                            appDesktopFile = f.filePath();
                            goto FIND_DESKTOP_END;
                        }
                    }
                }

                //Find the proper desktop file end, or not find
            FIND_DESKTOP_END:

                qInfo() << "UOSAbilityManager::doAppLaunch->" << appDesktopFile;

                int retCode = m_uosAppLauncher->launchDesktop(appDesktopFile);
                ctx.error = OSCallContext::CallError(retCode);

            } else {
                ctx.error = OSCallContext::AppNotFound;
            }
        }
    } else { //off
        ctx.error = OSCallContext::NotImpl;
    }

    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::doCreateSchedule(const QString &title,
                                                  const QString &startTime,
                                                  const QString &endTime)
{
    OSCallContext ctx;

    auto st = QDateTime::fromString(startTime, "yyyy-MM-ddThh:mm:ss");
    auto et = QDateTime::fromString(endTime, "yyyy-MM-ddThh:mm:ss");

    //TODO:
    //  Ai may reply invalid time format.So check if the times
    //are valid format.
    if (st.isValid() && et.isValid()) {
        QJsonObject schedObj;
        //Use the default title if title is missed
        schedObj["Title"] = title.isEmpty() ? QString("AI会议日程") : title;
        schedObj["Description"] = "Uos Ai " + title;
        schedObj["AllDay"] = false;
        schedObj["Type"] = 1;
        schedObj["Start"] = startTime;
        schedObj["End"] = endTime;
        schedObj["Remind"] = "15";

        qInfo() << "UOSAbilityManager::doCreateSchedule->" << schedObj;

        if (0 == m_uosCalendarScheduler->createSchedule(schedObj)) {
            ctx.error = OSCallContext::NonError;
        } else {
            ctx.error = OSCallContext::NonService;
        }
    } else {
        ctx.error = OSCallContext::InvalidArgs;

        qWarning() << "UOSAbilityManager::doCreateSchedule->"
                   << " title=" << title
                   << " start=" << startTime
                   << " endTime" << endTime;
    }

    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

void UOSAbilityManager::loadErrMap()
{
    m_errMap[OSCallContext::NonError] = "";
    m_errMap[OSCallContext::NotImpl]
        = QCoreApplication::translate(
              "UOSAbility"
              , "I haven't implemented this feature yet.");
    m_errMap[OSCallContext::NonService]
        = QCoreApplication::translate(
              "UOSAbility"
              , "service is not available!");

    m_errMap[OSCallContext::InvalidArgs]
        = QCoreApplication::translate(
              "UOSAbility"
              , "Invalid parameter!");

    m_errMap[OSCallContext::AppNotFound]
        = QCoreApplication::translate(
              "UOSAbility"
              , "This app cannot be found!");

    m_errMap[OSCallContext::AppStartFailed]
        = QCoreApplication::translate(
              "UOSAbility"
              , "Failed to start application!");

}

void UOSAbilityManager::loadApp2Desktop()
{
    // 读取JSON文件
    QFile file(":/assets/app/deepin-app-infos.json");

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load uos app infos.";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (!jsonDoc.isObject()) {
        qWarning() << "Uos app infos file format error!";
        return;
    }

    QJsonObject appConfigObject = jsonDoc.object();

    //Only use config application paths if system don't
    //set.
    if (m_defaultDesktopPaths.isEmpty()) {
        QJsonArray desktopPaths = appConfigObject["desktopPaths"].toArray();
        foreach (auto path, desktopPaths) {
            QString deskPath = path.toString();

            if (!deskPath.isEmpty()) {
                m_defaultDesktopPaths << deskPath;
            }
        }
    }

    QJsonArray appInfosArray = appConfigObject["appInfos"].toArray();

    foreach (const QJsonValue &value, appInfosArray) {
        if (value.isObject()) {
            UosAppInfo appInfo;

            QJsonObject app = value.toObject();

            appInfo.appId = app["appId"].toString();
            appInfo.appName = app["appName"].toString();
            appInfo.appIcon = app["appIcon"].toString();

            foreach (auto desktopFile, app["desktopFile"].toArray()) {
                appInfo.desktopFiles << desktopFile.toString();
            }

            m_app2Desktop.insert(appInfo.appId, appInfo);
        }
    }

    qInfo() << "loadApp2Desktop->" << m_app2Desktop.size() << m_defaultDesktopPaths;
}

void UOSAbilityManager::initUosProxys()
{
    //Check the OS type
    //TODO:
    //   Now only have two type, linglong and
    //Deepin/UOS. Deepin/UOS use the same interface.
    //May be deepin will change the interface
    //to org.deepin.dde.xxx
    m_fIsLinglong = UosInfo()->isLingLong();

    m_uosDockProxy.reset(
        new UosDock(
            UosDock::staticInterfaceName(),
            "/com/deepin/dde/daemon/Dock",
            QDBusConnection::sessionBus(), this));
    m_uosAppearanceProxy.reset(
        new UosAppearance(
            UosAppearance::staticInterfaceName(),
            "/com/deepin/daemon/Appearance",
            QDBusConnection::sessionBus(), this));

    m_uosDisplayProxy.reset(
        new UosDisplay(
            UosDisplay::staticInterfaceName(),
            "/com/deepin/daemon/Display",
            QDBusConnection::sessionBus(), this));

    m_uosDesktopProxy.reset(
        new QDBusInterface(
            "com.deepin.dde.desktop",
            "/com/deepin/dde/desktop",
            "com.deepin.dde.desktop",
            QDBusConnection::sessionBus(), this));

    m_uosControlCenterProxy.reset(new DeepinControlCenter(m_fIsLinglong, this));
    m_uosNotificationProxy.reset(new DeepinNotification(m_fIsLinglong, this));
    m_uosAppLauncher.reset(new DeepinLauncher(m_fIsLinglong, m_defaultDesktopPaths, this));
    m_uosCalendarScheduler.reset(new DeepinCalendar(this));
}

void UOSAbilityManager::initDesktopPaths()
{
    QString systemDataDirs = UosInfo()->pureEnvironment().value("XDG_DATA_DIRS");

#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
    QStringList systemDataPaths = systemDataDirs.split(":", QString::SkipEmptyParts);
#else
    QStringList systemDataPaths = systemDataDirs.split(":", Qt::SkipEmptyParts);
#endif

    foreach (auto p, systemDataPaths) {
        m_defaultDesktopPaths << (p + QString("/applications"));
    }
}
