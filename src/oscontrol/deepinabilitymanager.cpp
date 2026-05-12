#include "deepinabilitymanager.h"
#include "deepinlauncher.h"
#include "deepincalendar.h"
#include "deepinnotification.h"
#include "deepincontrolcenter.h"
#include "deepinmultimedia.h"
#include "osinfo.h"
#include "storeapi.h"
#include "localmodelserver.h"
#include "deepinmultimedia.h"
#include "global_define.h"

#include <DDesktopEntry>
#include <DSysInfo>

#include <QApplication>
#include <QDBusVariant>
#include <QDBusArgument>
#include <QDateTime>
#include <QDesktopServices>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QProcess>
#include <QThread>
#include <QtConcurrent>
#include <QDir>
#include <QRegularExpression>
#include <QUrlQuery>

#include <math.h>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

DCORE_USE_NAMESPACE
using namespace uos_ai;

static QString formatCap(qulonglong cap, const int size = 1024, quint8 precision = 1)
{
    QStringList type { "B", "KB", "MB", "GB", "TB" };

    qulonglong lc = cap;
    double dc = cap;
    double ds = size;

    for (int p = 0; p < type.size(); ++p) {
        if (cap < pow(size, p + 1) || p == (type.size() - 1)) {
            if (!precision) {
                //! 内存总大小只能是整数所以当内存大小有小数时，就需要向上取整
                int mem = static_cast<int>(ceil(lc / pow(size, p)));
#ifdef __sw_64__
                return QString::number(mem) + type[p];
#else
                //! 如果向上取整后内存大小不为偶数，就向下取整
                if (mem % 2 > 0)
                    mem = static_cast<int>(floor(lc / pow(size, p)));
                return QString::number(mem) + type[p];
#endif
            }

            return QString::number(dc / pow(ds, p), 'f', precision) + type[p];
        }
    }

    return "";
}

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

UOSAbilityManager *UOSAbilityManager::instance()
{
    static UOSAbilityManager ins;
    return &ins;
}

OSCallContext UOSAbilityManager::doBluetoothConfig(bool on)
{
    qCDebug(logOsControl) << "Configuring bluetooth, state:" << on;
    QStringList adpters;
    {
        QVariant adptersResult;
        if (!osCtrCallDbus(osCallDbusBtService, osCallDbusBtPath, osCallDbusBtInterface,
                           QString("GetAdapters"), adptersResult)) {
            qCWarning(logOsControl) << "Failed to get bluetooth adapters";
            return ctxByError(OSCallContext::NonError);
        }
        QJsonDocument doc = QJsonDocument::fromJson(adptersResult.toString().toUtf8());
        QJsonArray arr = doc.array();
        for (int index = 0; index < arr.size(); index++)
            adpters.append(arr[index].toObject()["Path"].toString());
    }

    if (adpters.isEmpty()) {
        qCInfo(logOsControl) << "No bluetooth adapter found";
        return ctxByError(OSCallContext::NonService);
    }

    for (auto adpter : adpters) {
        QDBusObjectPath dPath(adpter);
        QVariantList args;
        args << QVariant::fromValue(dPath)
             << QVariant::fromValue(on);
        osCtrCallDbusNoResult(osCallDbusBtService, osCallDbusBtPath, osCallDbusBtInterface, QString("SetAdapterPowered"), args);
    }

    if (!on) {
        osCtrCallDbusNoResult(osCallDbusBtService, osCallDbusBtPath, osCallDbusBtInterface, QString("ClearUnpairedDevice"));
    }

    qCInfo(logOsControl) << "Bluetooth configuration completed successfully";
    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::doScreenMirroring(bool state)
{
    qCDebug(logOsControl) << "Configuring screen mirroring, state:" << state;
    Q_UNUSED(state);

    // display:Casting
    // com.deepin.dde.ControlCenter
    // /com/deepin/dde/ControlCenter
    // com.deepin.dde.ControlCenter.ShowPage 'display' 'Casting'

    OSCallContext ctx;
    int errCode;

#ifdef COMPILE_ON_V23
    errCode = m_uosControlCenterProxy->ShowPage("display/Casting");
#else
    errCode = m_uosControlCenterProxy->ShowPage("display", "Casting");
#endif

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
    qCDebug(logOsControl) << "Configuring do not disturb mode, state:" << state;
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
    qCDebug(logOsControl) << "Switching wallpaper";
    auto covertUrlToLocalPath = [](const QString &url) {
        if (url.startsWith("/"))
            return url;
        else
            return QUrl(QUrl::fromPercentEncoding(url.toUtf8())).toLocalFile();
    };

    QVariantList listArg {QVariant::fromValue(QString("background"))};
    QVariant list;
    if (!osCtrCallDbus(osCallDbusAppearanceService, osCallDbusAppearancePath, osCallDbusAppearanceInterface,
                       QString("List"), list, listArg)) {
        qCWarning(logOsControl) << "Failed to get wallpaper list";
        return ctxByError(OSCallContext::NonService);
    }

    QVariant screen;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("Primary"), screen)) {
        qCWarning(logOsControl) << "Failed to get primary screen";
        return ctxByError(OSCallContext::NonService);
    }

    QVariantList resultArg {screen};
    QVariant result;
    if (!osCtrCallDbus(osCallDbusWmService, osCallDbusWmPath, osCallDbusWmInterface,
                       "GetCurrentWorkspaceBackgroundForMonitor", result, resultArg)) {
        qCWarning(logOsControl) << "Failed to get current workspace background";
        return ctxByError(OSCallContext::NonService);
    }
    QString current = covertUrlToLocalPath(result.toString());

    QList<QString> wallpapers;
    QJsonDocument doc = QJsonDocument::fromJson(list.toString().toUtf8());
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        foreach (QJsonValue val, arr) {
            QJsonObject obj = val.toObject();
            QString id = covertUrlToLocalPath(obj["Id"].toString());
            wallpapers.append(id);
        }
    }

    if (wallpapers.isEmpty() || screen.isNull()) {
        qCWarning(logOsControl) << "No wallpapers found or invalid screen";
        return ctxByError(OSCallContext::NonService);
    } else {
        int idx = wallpapers.indexOf(current);
        if (idx < 0 || idx >= wallpapers.size() - 1)
            idx = 0;
        else
            idx++;

        QString next = wallpapers.at(idx);
        QList<QVariant> argumentList;
        argumentList << screen << QVariant::fromValue(next);
        if (!osCtrCallDbusNoResult(osCallDbusAppearanceService, osCallDbusAppearancePath, osCallDbusAppearanceInterface,
                           "SetMonitorBackground", argumentList)) {
            qCWarning(logOsControl) << "Failed to set monitor background";
            return ctxByError(OSCallContext::NonService);
        }
        qCInfo(logOsControl) << "Wallpaper switched successfully to:" << next;
    }

    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::doDesktopOrganize(bool state)
{
    qCDebug(logOsControl) << "Configuring desktop organization, state:" << state;
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    QProcess process;
    QStringList args;
    args << "--set" << "-a" << "org.deepin.dde.file-manager"
         << "-r" << "org.deepin.dde.file-manager.desktop.organizer"
         << "-k" << "enableOrganizer" << "-v";
    int exitCode = 0;
    if (state) {
        // 先开
        {
            args << QString("1");
            process.start("dde-dconfig", args);
            process.waitForFinished();
            exitCode = process.exitCode();
        }

        QStringList queryArgs;
        queryArgs << "--get" << "-a" << "org.deepin.dde.file-manager"
             << "-r" << "org.deepin.dde.file-manager.desktop.organizer"
             << "-k" << "organizeAction";

        QStringList setArgs;
        setArgs << "--set" << "-a" << "org.deepin.dde.file-manager"
             << "-r" << "org.deepin.dde.file-manager.desktop.organizer"
             << "-k" << "organizeAction" << "-v";
        if (exitCode == 0) {
            //查询集合action
            process.start("dde-dconfig", queryArgs);
            process.waitForFinished();
            exitCode = process.exitCode();
            if (exitCode == 0) {
               QString out = QString::fromUtf8(process.readAllStandardOutput());
               out = out.replace("\n", "");
               out = out.replace("\"","");
               int action = out.toInt();
               if (action == 0) {
                   QStringList setArgsTmp = setArgs;
                   setArgsTmp << QString("1");
                   process.start("dde-dconfig", setArgsTmp);
                   process.waitForFinished();
                   exitCode = process.exitCode();

                   QThread::sleep(1);

                   setArgsTmp = setArgs;
                   setArgsTmp << QString("0");
                   process.start("dde-dconfig", setArgsTmp);
                   process.waitForFinished();
                   exitCode = process.exitCode();
               }
            }
        }
    } else {
        args << QString("0");

        process.start("dde-dconfig", args);

        // Wait for the process to finish
        process.waitForFinished();

        // Get the exit code of the process
        exitCode = process.exitCode();
    }

    if (exitCode != 0) {
        if (process.error() == QProcess::ProcessError::FailedToStart) {
            qCWarning(logOsControl) << "Failed to start dde-dconfig process";
            ctx.error = OSCallContext::NotImpl;
            ctx.errorInfo = m_errMap[ctx.error];
        } else {
            ctx.error = OSCallContext::NonService;
            ctx.errorInfo = m_errMap[ctx.error];
        }

        qCWarning(logOsControl) << "Failed to execute dde-dconfig: state=" << state
               << " exitCode=" << exitCode
               << " stdout=" << QString::fromLocal8Bit(process.readAllStandardOutput())
               << " stderr=" << QString::fromLocal8Bit(process.readAllStandardError());
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doDockModeSwitch(int mode)
{
    qCDebug(logOsControl) << "Switching dock mode to:" << mode;
    if (mode < 0 || mode >= 2) {
        qCWarning(logOsControl) << "Invalid dock mode:" << mode;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    if (!propertiesSet(osCallDbusDockService, osCallDbusDockPath, osCallDbusDockInterface,
                       QString("DisplayMode"), QVariant::fromValue(mode))) {
        qCWarning(logOsControl) << "Failed to set dock display mode";
        return ctxByError(OSCallContext::NonService);
    }

    qCInfo(logOsControl) << "Dock mode switched successfully to:" << mode;
    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::doSystemThemeSwitch(int theme)
{
    qCDebug(logOsControl) << "Switching system theme to:" << theme;
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    if (theme < 0 || theme >= 3) {
        qCWarning(logOsControl) << "Invalid theme value:" << theme;
        return ctxByError(OSCallContext::InvalidArgs);
    }

#ifdef COMPILE_ON_V23
    QVariant globalTheme;
    if (!propertiesGet(osCallDbusAppearanceService, osCallDbusAppearancePath, osCallDbusAppearanceInterface, QString("GlobalTheme"), globalTheme)) {
        qCWarning(logOsControl) << "Failed to get global theme";
        return ctxByError(OSCallContext::NonService);
    }
    QString globalThemeStr = globalTheme.toString();
    if (globalThemeStr.isEmpty()) {
        qCWarning(logOsControl) << "Global theme is empty";
        return ctxByError(OSCallContext::NonService);
    }
    QString themeName;
    if (globalThemeStr.contains(".")) {
        themeName = globalThemeStr.split(".").at(0);
    } else {
        themeName = globalThemeStr;
    }

    QVariantList args;
    args << QVariant::fromValue(QString("GlobalTheme"));
    switch (theme) {
    case 0:
        args << QVariant::fromValue(themeName + ".light");
        break;
    case 1:
        args << QVariant::fromValue(themeName + ".dark");
        break;
    case 2:
        args << QVariant::fromValue(themeName);
        break;
    default:
        break;
    }

    if (!osCtrCallDbusNoResult(osCallDbusAppearanceService, osCallDbusAppearancePath, osCallDbusAppearanceInterface, QString("Set"), args)) {
        qCWarning(logOsControl) << "Failed to set global theme";
        return ctxByError(OSCallContext::NonService);
    }

    return ctxByError(OSCallContext::NonError);
#else
    QVariantList args;
    args << QVariant::fromValue(QString("gtk"));

    switch (theme) {
    case 0:
        args << QVariant::fromValue(QString("deepin"));
        break;
    case 1:
        args << QVariant::fromValue(QString("deepin-dark"));
        break;
    case 2:
        args << QVariant::fromValue(QString("deepin-auto"));
        break;
    default:
        break;
    }

    if (!osCtrCallDbusNoResult(osCallDbusAppearanceService, osCallDbusAppearancePath, osCallDbusAppearanceInterface, QString("Set"), args)) {
        return ctxByError(OSCallContext::NonService);
    }

    return ctxByError(OSCallContext::NonError);
#endif
}

OSCallContext UOSAbilityManager::doDiplayEyesProtection(bool on)
{
    //Eyes Protection
    //TODO:
    //   Brightness:70%
    //   Temperatrue: pos [0-100] 20%
    //      int kelvin = pos > 50 ? (6500 - (pos - 50) * 100) : (6500 + (50 - pos) * 300);
    //   May need adjust other display parameter.
    QVariant result;
    if (!osCtrCallDbus(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("SupportSetColorTemperature"), result)) {
        qCWarning(logOsControl) << "Failed to check color temperature support";
        return ctxByError(OSCallContext::NonService);
    }

    bool isSupEyesProtection = result.toBool();
    if (!isSupEyesProtection) {
        qCWarning(logOsControl) << "Color temperature adjustment not supported";
        return ctxByError(OSCallContext::NonService);
    }

    if (on) {
        QVariantList argCCT {QVariant::fromValue(2)};
        if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface,
                                   QString("SetMethodAdjustCCT"), argCCT)) {
            qCWarning(logOsControl) << "Failed to set color temperature adjustment method";
            return ctxByError(OSCallContext::NonService);
        }

        int pos = 80;
        int kelvin = pos > 50 ? (6500 - (pos - 50) * 100) : (6500 + (50 - pos) * 300);
        QVariantList argTem {QVariant::fromValue(kelvin)};
        if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface,
                                   QString("SetColorTemperature"), argTem)) {
            qCWarning(logOsControl) << "Failed to set color temperature";
            return ctxByError(OSCallContext::NonService);
        }
    } else {
        //Close the eyes protection mode
        // Brightness:80%
        // Temperatrue: disable
        QVariantList argCCT {QVariant::fromValue(0)};
        if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface,
                                   QString("SetMethodAdjustCCT"), argCCT)) {
            qCWarning(logOsControl) << "Failed to disable color temperature adjustment";
            return ctxByError(OSCallContext::NonService);
        }
    }

    qCInfo(logOsControl) << "Eye protection mode configured successfully, state:" << on;
    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::doDiplayBrightness(int value, int adjustment)
{    
    qCDebug(logOsControl) << "Adjusting display brightness - value:" << value << "adjustment:" << adjustment;
    
    // Get primary screen first
    QVariant screen;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("Primary"), screen)) {
        qCWarning(logOsControl) << "Failed to get primary screen";
        return ctxByError(OSCallContext::NonService);
    }
    
    QString primaryScreen = screen.toString();

    // For relative adjustments, get current brightness first
    if (adjustment != 0) {
        // Get brightness for all screens
        QVariant brightnessResult;
        if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, 
                          QString("Brightness"), brightnessResult)) {
            qCWarning(logOsControl) << "Failed to get brightness";
            return ctxByError(OSCallContext::NonService);
        }

        // Handle DBus return value safely
        double currentBrightness = 0.0;
        QVariantMap brightnessMap;        
        // Since we know the return type is QDBusArgument containing a Dict of {String, Double}
        if (brightnessResult.canConvert<QDBusArgument>()) {
            const QDBusArgument &dbusArg = brightnessResult.value<QDBusArgument>();
            if (dbusArg.currentType() == QDBusArgument::MapType) {
                dbusArg.beginMap();
                while (!dbusArg.atEnd()) {
                    QString key;
                    double value;
                    dbusArg.beginMapEntry();
                    dbusArg >> key >> value;
                    dbusArg.endMapEntry();
                    brightnessMap[key] = value;
                }
                dbusArg.endMap();
            }
        }

        // Check if we got any data
        if (brightnessMap.isEmpty()) {
            qCWarning(logOsControl) << "Failed to get brightness map from DBus result";
            return ctxByError(OSCallContext::NonService);
        }

        // Get brightness for primary screen
        QVariant primaryBrightness = brightnessMap.value(primaryScreen);
        if (!primaryBrightness.isValid()) {
            qCWarning(logOsControl) << "No brightness value found for primary screen";
            return ctxByError(OSCallContext::NonService);
        }

        bool ok = false;
        currentBrightness = primaryBrightness.toDouble(&ok);
        if (!ok || currentBrightness < 0.0) {
            qCWarning(logOsControl) << "Invalid brightness value for primary screen:" << primaryBrightness;
            return ctxByError(OSCallContext::NonService);
        }

        // Convert current brightness to percentage
        int currentValue = qRound(currentBrightness * 100);

        // Check boundary conditions for relative adjustments
        if (adjustment == 1) {
            // Trying to increase brightness
            if (currentValue >= 100) {
                qCInfo(logOsControl) << "Brightness is already at maximum (100%), cannot increase further";
                OSCallContext ctx;
                ctx.error = OSCallContext::NonError;
                ctx.output = tr("Brightness is already at maximum and cannot be increased further.");
                ctx.result["brightness"] = currentValue;
                return ctx;
            }
            value = qMin(100, currentValue + 10);
        } else {
            // Trying to decrease brightness
            if (currentValue <= 0) {
                qCInfo(logOsControl) << "Brightness is already at minimum (0%), cannot decrease further";
                OSCallContext ctx;
                ctx.error = OSCallContext::NonError;
                ctx.output = tr("Brightness is already at minimum and cannot be decreased further.");
                ctx.result["brightness"] = currentValue;
                return ctx;
            }
            value = qMax(0, currentValue - 10);
        }
    } else {
        // Validate absolute value
        if (value < 0 || value > 100) {
            qCWarning(logOsControl) << "Invalid brightness value:" << value;
            return ctxByError(OSCallContext::InvalidArgs);
        }
    }

    // Set new brightness
    QVariantList args;
    args << screen
         << QVariant::fromValue(value / 100.0);
    if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface,
                               QString("SetAndSaveBrightness"), args)) {
        qCWarning(logOsControl) << "Failed to set display brightness";
        return ctxByError(OSCallContext::NonService);
    }

    OSCallContext ctx = ctxByError(OSCallContext::NonError);
    ctx.result["brightness"] = value;
    return ctx;
}

OSCallContext UOSAbilityManager::doAppLaunch(const QString &appId, bool on)
{
    qCDebug(logOsControl) << "Launching application:" << appId << "state:" << on;
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
                        qCWarning(logOsControl) << "Failed to open default browser";
                        ctx.error = OSCallContext::AppStartFailed;
                    }
                } else {
                    qCWarning(logOsControl) << "No default browser found";
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
                qCDebug(logOsControl) << "Found desktop file:" << appDesktopFile;
                int retCode = m_uosAppLauncher->launchDesktop(appDesktopFile);
                ctx.error = OSCallContext::CallError(retCode);

            } else {
                qCWarning(logOsControl) << "Application not found:" << appId;
                ctx.error = OSCallContext::AppNotFound;
            }
        }
    } else { //off
        // 关闭的应用范围为：应用商店、音乐、影院、语音记事本、计算器
        static QMap<QString, QString> appMap = {
            {"appstore", "deepin-home-appstore-client"},
            {"musicPlayer", "deepin-music"},
            {"moviePlayer", "deepin-movie"},
            {"deepinVoiceNote", "deepin-voice-note"},
            {"calculator", "deepin-calculator"}};

        qCDebug(logOsControl) << "Preparing to close application:" << appId;
        if (appMap.contains(appId)) {
            QString bin = appMap[appId];
            qCDebug(logOsControl) << "Executing killall for:" << bin;
            QStringList argvList;
            argvList.append(bin);
            QProcess::execute("killall", argvList);
        } else {
            qCWarning(logOsControl) << "Application not in closeable list:" << appId;
            ctx.error = OSCallContext::NotImpl;
        }
    }

    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::doCreateSchedule(const QString &title,
                                                  const QString &startTime,
                                                  const QString &endTime)
{
    qCDebug(logOsControl) << "Creating schedule - title:" << title
                         << "start:" << startTime
                         << "end:" << endTime;
    OSCallContext ctx;

    auto st = QDateTime::fromString(startTime, "yyyy-MM-ddThh:mm:ss");
    auto et = QDateTime::fromString(endTime, "yyyy-MM-ddThh:mm:ss");

    //TODO:
    //  Ai may reply invalid time format.So check if the times
    //are valid format.
    if (st.isValid() && et.isValid()) {
        
        QJsonObject schedObj;
        //Use the default title if title is missed
        schedObj["Title"] = title.isEmpty() ? tr("AI Meeting Schedule") : title;
        schedObj["Description"] = "Uos Ai " + title;
        schedObj["AllDay"] = false;
        schedObj["Type"] = 1;
        schedObj["Start"] = startTime;
        schedObj["End"] = endTime;
        schedObj["Remind"] = "15";

        qCDebug(logOsControl) << "Schedule object created:" << schedObj;

        if (0 == m_uosCalendarScheduler->createSchedule(schedObj)) {
            qCInfo(logOsControl) << "Schedule created successfully";
            ctx.error = OSCallContext::NonError;
        } else {
            qCWarning(logOsControl) << "Failed to create schedule";
            ctx.error = OSCallContext::NonService;
        }
    } else {
        qCWarning(logOsControl) << "Invalid time format - start:" << startTime
                               << "end:" << endTime;
        ctx.error = OSCallContext::InvalidArgs;
    }

    ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::switchWifi(bool on)
{
    qCDebug(logOsControl) << "Switching WiFi, state:" << on;
    QString systemBusNetworkService = "org.freedesktop.NetworkManager";
    QString systemBusNetworkPath = "/org/freedesktop/NetworkManager";
    QString systemBusNetworkInterface = "org.freedesktop.NetworkManager";

    auto conn = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(systemBusNetworkService, systemBusNetworkPath, systemBusNetworkInterface, "GetDevices");
    QDBusMessage reply = conn.call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return ctxByError(OSCallContext::NonService);
    }
    qCInfo(logOsControl) << "Devices fetched successfully.";

    QList<QDBusObjectPath> allPaths;
    const QVariant &deviceRes = reply.arguments().at(0);
    if (deviceRes.canConvert<QDBusArgument>()) {
        deviceRes.value<QDBusArgument>() >> allPaths;
    }

    if (allPaths.isEmpty()) {
        qCWarning(logOsControl) << "No device paths found";
        return ctxByError(OSCallContext::NonService);
    }

    QList<QDBusObjectPath> wirelessPaths;
    for (const auto &p : allPaths) {
        QString path = p.path();
        QDBusMessage propMsg = QDBusMessage::createMethodCall(systemBusNetworkService, path, "org.freedesktop.DBus.Properties", "Get");
        propMsg << "org.freedesktop.NetworkManager.Device" << "DeviceType";

        QDBusMessage propReply = conn.call(propMsg);
        if (propReply.type() != QDBusMessage::ReplyMessage) {
            continue;
        }

        // 提取 DeviceType
        QVariant propVal = propReply.arguments().value(0).value<QDBusVariant>().variant();
        if (propVal.toUInt() == 2) {
            qCInfo(logOsControl) << "Found Wi-Fi device:" << path;
            wirelessPaths << p;
        }
    }

    if (wirelessPaths.isEmpty()) {
        qCWarning(logOsControl) << "No wireless adapters found";
        return ctxByError(OSCallContext::NonService);
    }

    for (const auto &path : wirelessPaths) {
#ifdef COMPILE_ON_V25
        QVariantList args;
        args << QVariant::fromValue(path.path()) << QVariant::fromValue(on);
        QDBusMessage message = QDBusMessage::createMethodCall(osCallDbusNetworkService, osCallDbusNetworkPath, osCallDbusNetworkInterface, "EnableDevice");
        message.setArguments(args);
        QDBusMessage callReply = conn.call(message);
        if (callReply.type() != QDBusMessage::ReplyMessage) {
            return ctxByError(OSCallContext::NonService);
        }
#else
        QVariantList args;
        args << QVariant::fromValue(path) << QVariant::fromValue(on);
        osCtrCallDbusNoResult(
            osCallDbusNetworkService,
            osCallDbusNetworkPath,
            osCallDbusNetworkInterface,
            "EnableDevice",
            args
        );
#endif
    }

    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.output = textForCommnand();

    return ctx;
}

OSCallContext UOSAbilityManager::getSystemMemory()
{
    qCDebug(logOsControl) << "Getting system memory information";
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    qint64 mem = DSysInfo::memoryInstalledSize();

    if (mem < 1) {
        qCWarning(logOsControl) << "Invalid memory size:" << mem;
        ctx.error = OSCallContext::InvalidArgs;
        ctx.errorInfo = m_errMap[ctx.error];
    } else {
        QString strMem = formatCap(mem, 1024, 0);
        ctx.output = tr("Your system memory is %0.") .arg(strMem);
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doSystemLanguageSetting()
{
    qCDebug(logOsControl) << "Opening system language settings";
    OSCallContext ctx;
    int errCode = 0;

#ifdef COMPILE_ON_V23
    errCode = m_uosControlCenterProxy->ShowPage("keyboard/keyboardLanguage");
#else
    errCode = m_uosControlCenterProxy->ShowPage("keyboard", "System Language");
#endif
    ctx.error = OSCallContext::CallError(errCode);

    if (errCode == 0)
        ctx.output = tr("The language setting interface has been opened. Please set it in this interface.");
    else
        ctx.errorInfo = m_errMap[ctx.error];

    return ctx;
}

OSCallContext UOSAbilityManager::doPerformanceModeSwitch(const QString &mode, bool isOpen)
{
    qCDebug(logOsControl) << "Switching performance mode - mode:" << mode << "isOpen:" << isOpen;
    const QString performanceStr = "performance";
    const QString balanceStr = "balance";
    const QString powersaveStr = "powersave";

    auto conn = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(osCallDbusPowerService, osCallDbusPowerPath, QString("org.freedesktop.DBus.Properties"),
                                                      QString("Get"));
    QVariantList args;
    args << QVariant::fromValue(QString(osCallDbusPowerInterface))
         << QVariant::fromValue(QString("Mode"));
    msg.setArguments(args);
    QDBusReply<QVariant> reply = conn.call(msg);
    QString currentMode = reply.value().toString();

    // set mode
    auto setMode = [](const QString &mode){
        auto conn = QDBusConnection::systemBus();
        QDBusMessage setMsg = QDBusMessage::createMethodCall(osCallDbusPowerService, osCallDbusPowerPath, osCallDbusPowerInterface,
                                                          QString("SetMode"));
        QVariantList setArgs {QVariant::fromValue(mode)};
        setMsg.setArguments(setArgs);
        QDBusMessage setReply = conn.call(setMsg);
        if (setReply.type() != QDBusMessage::ReplyMessage) {
            qCWarning(logOsControl) << "Failed to set power mode:" << mode;
            return false;
        }

        return true;
    };

    if (isOpen) {
        // 切换
        if (mode == currentMode) {
            OSCallContext ctx;
            ctx.output = tr("The current mode is already %1 mode.").arg(mode);
            return ctx;
        }

        bool res = setMode(mode);
        if (!res) {
            qCWarning(logOsControl) << "Failed to switch to mode:" << mode;
            return ctxByError(OSCallContext::NonService);
        }
    } else {
        // 关闭
        if (mode != currentMode) {
            qCWarning(logOsControl) << "Current mode" << currentMode << "does not match target mode" << mode;
            OSCallContext ctx;
            ctx.output = tr("Unable to close because the current mode %1 does not match the target mode.").arg(mode);
            return ctx;
        }

        if (mode == performanceStr || mode == powersaveStr) {
            bool res = setMode(balanceStr);
            if (!res) {
                qCWarning(logOsControl) << "Failed to switch to balance mode";
                return ctxByError(OSCallContext::NonService);
            }
        } else if (mode == balanceStr) {
            qCWarning(logOsControl) << "Cannot turn off balance mode";
            OSCallContext ctx;
            ctx.output = tr("Balance mode cannot be turned off.");
            return ctx;
        }
    }

    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::openShutdownFront()
{
    qCDebug(logOsControl) << "Opening shutdown interface";
    if (!osCtrCallDbusNoResult(osCallDbusShutDownService, osCallDbusShutDownPath, osCallDbusShutDownInterface, QString("Show"))) {
        qCWarning(logOsControl) << "Failed to open shutdown interface";
        return ctxByError(OSCallContext::NonService);
    }

    qCInfo(logOsControl) << "Shutdown interface opened successfully";
    OSCallContext ctx;
    ctx.output = tr("The lock screen has been opened for you");
    return ctx;
}

OSCallContext UOSAbilityManager::openScreenShot()
{
    qCDebug(logOsControl) << "Opening screenshot interface";
    if (!osCtrCallDbusNoResult(osCallDbusScreenshotService, osCallDbusScreenshotPath, osCallDbusScreenshotInterface, QString("StartScreenshot"))) {
        qCWarning(logOsControl) << "Failed to open screenshot interface";
        return ctxByError(OSCallContext::NonService);
    }

    qCInfo(logOsControl) << "Screenshot interface opened successfully";
    OSCallContext ctx;
    ctx.output = tr("Screen shotting or recording has been completed");
    return ctx;
}

OSCallContext UOSAbilityManager::doDisplayModeSwitch(int mode)
{
    qCDebug(logOsControl) << "Switching display mode to:" << mode;
    QVariant result;
    if (!osCtrCallDbus(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("ListOutputNames"), result)) {
        qCWarning(logOsControl) << "Failed to get output names";
        return ctxByError(OSCallContext::NonService);
    }
    QStringList screenList = result.toStringList();

    if (screenList.count() <= 1) {
        qCWarning(logOsControl) << "Only one screen available, cannot switch display mode";
        OSCallContext ctx;
        ctx.output = tr("Only one screen, can't switch screen mode.");
        return ctx;
    }

    QVariant currentMode;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("DisplayMode"), currentMode)) {
        qCWarning(logOsControl) << "Failed to get current display mode";
        return ctxByError(OSCallContext::NonService);
    }
    if (mode == currentMode.toInt()) {
        qCInfo(logOsControl) << "Already in requested display mode:" << mode;
        OSCallContext ctx;
        ctx.output = tr("It is the same as the current display mode. Please try again.");
        return ctx;
    }

    QVariant primary;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("Primary"), primary)) {
        qCWarning(logOsControl) << "Failed to get primary display";
        return ctxByError(OSCallContext::NonService);
    }

    QVariantList args;
    args << QVariant::fromValue(mode)
         << primary;
    if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("SwitchMode"), args)) {
        qCWarning(logOsControl) << "Failed to switch display mode";
        return ctxByError(OSCallContext::NonService);
    }

    qCInfo(logOsControl) << "Display mode switched successfully to:" << mode;
    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::openGrandSearch()
{
    qCDebug(logOsControl) << "Opening grand search interface";
    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.errorInfo = m_errMap[ctx.error];

    QDBusInterface *grandSearch = new QDBusInterface(
                "com.deepin.dde.GrandSearch",
                "/com/deepin/dde/GrandSearch",
                "com.deepin.dde.GrandSearch",
                QDBusConnection::sessionBus(), this);

    if (!grandSearch->isValid()) {
        qCWarning(logOsControl) << "Failed to connect to grand search service";
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = m_errMap[ctx.error];
    } else {
        qCInfo(logOsControl) << "Grand search interface opened successfully";
    }

    return ctx;
}

OSCallContext UOSAbilityManager::switchScreen()
{
    qCDebug(logOsControl) << "Switching screen";
    uchar singleMode = 3;
    QVariant currentMode;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("DisplayMode"), currentMode)) {
        qCWarning(logOsControl) << "Failed to get current display mode";
        return ctxByError(OSCallContext::NonService);
    }

    QVariant primaryScreen;
    if (!propertiesGet(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("Primary"), primaryScreen)) {
        qCWarning(logOsControl) << "Failed to get primary screen";
        return ctxByError(OSCallContext::NonService);
    }
    QString primary = primaryScreen.toString();

    QVariant screenList;
    if (!osCtrCallDbus(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("ListOutputNames"), screenList)) {
        qCWarning(logOsControl) << "Failed to get screen list";
        return ctxByError(OSCallContext::NonService);
    }
    QStringList screens = screenList.toStringList();

    if (screens.length() <= 1) {
        qCWarning(logOsControl) << "Only one screen available, cannot switch";
        OSCallContext ctx;
        ctx.output = tr("Only one screen, can't switch screen.");
        return ctx;
    }

    QVariantList switchArgs;
    switchArgs << QVariant::fromValue(singleMode);
    if (singleMode == currentMode) {
        int screenIdx = screens.indexOf(primary);
        if (screens.endsWith(primary))
            switchArgs << QVariant::fromValue(screens.first());
        else
            switchArgs << QVariant::fromValue(screens[screenIdx + 1]);
    }
    else
        switchArgs << primaryScreen;

    if (!osCtrCallDbusNoResult(osCallDbusDisplayService, osCallDbusDisplayPath, osCallDbusDisplayInterface, QString("SwitchMode"), switchArgs)) {
        qCWarning(logOsControl) << "Failed to switch screen mode";
        return ctxByError(OSCallContext::NonService);
    }

    qCInfo(logOsControl) << "Screen switched successfully";
    return ctxByError(OSCallContext::NonError);
}

OSCallContext UOSAbilityManager::volumeAdjustment(const QJsonObject &argsObj)
{
    qCDebug(logOsControl) << "Adjusting volume with args:" << argsObj;
    QVariant audioSink;
    if (!propertiesGet(osCallDbusAudioService, osCallDbusAudioPath, osCallDbusAudioInterface, QString("DefaultSink"), audioSink)) {
        qCWarning(logOsControl) << "Failed to get default audio sink";
        return ctxByError(OSCallContext::NonService);
    }
    QString audioPath = audioSink.value<QDBusObjectPath>().path();

    if (audioPath.isEmpty()) {
        qCWarning(logOsControl) << "Audio path is empty";
        return ctxByError(OSCallContext::NonService);
    }

    int volume = 0;

    if (argsObj.contains("mute")) {
        // 静音
        bool isMute = argsObj.value("mute").toBool();
        qCDebug(logOsControl) << "Setting mute state to:" << isMute;
        QVariantList muteArgs;
        muteArgs << QVariant::fromValue(isMute);
        if (!osCtrCallDbusNoResult(osCallDbusAudioService, audioPath, osCallDbusAudioInterface + QString(".Sink"), QString("SetMute"), muteArgs)) {
            qCWarning(logOsControl) << "Failed to set mute state";
            return ctxByError(OSCallContext::NonService);
        }
        qCInfo(logOsControl) << "Mute state set successfully to:" << isMute;

        // 获取当前音量
        QVariant volVariant;
        if (propertiesGet(osCallDbusAudioService, audioPath, osCallDbusAudioInterface + QString(".Sink"), QString("Volume"), volVariant)) {
            volume = static_cast<int>(volVariant.toDouble() * 100);
        }
        OSCallContext ctx = ctxByError(OSCallContext::NonError);
        ctx.result["volume"] = volume;
        return ctx;
    }

    if (argsObj.contains("approximate")) {
        // 模糊调节
        QString approximate = argsObj.value("approximate").toString();
        QVariant volVariant;
        if (!propertiesGet(osCallDbusAudioService, audioPath, osCallDbusAudioInterface + QString(".Sink"), QString("Volume"), volVariant)) {
            qCWarning(logOsControl) << "Failed to get current volume";
            return ctxByError(OSCallContext::NonService);
        }
        double vol = volVariant.toDouble();
        if (approximate == "Add") {
            vol += 0.1;
        } else if (approximate == "Reduce") {
            vol -= 0.1;
        } else {
            qCWarning(logOsControl) << "Invalid approximate value:" << approximate;
            return ctxByError(OSCallContext::InvalidArgs);
        }
        vol = qBound(0.0, vol, 1.0);
        QVariantList volArgs;
        volArgs << QVariant::fromValue(vol);
        volArgs << QVariant::fromValue(true);
        if (!osCtrCallDbusNoResult(osCallDbusAudioService, audioPath, osCallDbusAudioInterface + QString(".Sink"), QString("SetVolume"), volArgs)) {
            qCWarning(logOsControl) << "Failed to set volume";
            return ctxByError(OSCallContext::NonService);
        }
        qCInfo(logOsControl) << "Volume adjusted successfully to:" << vol;
        volume = static_cast<int>(vol * 100);
        OSCallContext ctx = ctxByError(OSCallContext::NonError);
        ctx.result["volume"] = volume;
        return ctx;
    }

    if (argsObj.contains("volume")) {
        double vol = argsObj.value("volume").toInt() / 100.0;
        if (vol < 0 || vol > 100) {
            qCWarning(logOsControl) << "Invalid volume value:" << vol;
            return ctxByError(OSCallContext::InvalidArgs);
        }

        QVariantList volArgs;
        volArgs << QVariant::fromValue(vol);
        volArgs << QVariant::fromValue(true);
        if (!osCtrCallDbusNoResult(osCallDbusAudioService, audioPath, osCallDbusAudioInterface + QString(".Sink"), QString("SetVolume"), volArgs)) {
            qCWarning(logOsControl) << "Failed to set volume";
            return ctxByError(OSCallContext::NonService);
        }
        qCInfo(logOsControl) << "Volume set successfully to:" << vol;
        volume = static_cast<int>(vol * 100);
        OSCallContext ctx = ctxByError(OSCallContext::NonError);
        ctx.result["volume"] = volume;
        return ctx;
    }

    return ctxByError(OSCallContext::NonError);
}

QString UOSAbilityManager::textForCommnand()
{
    return tr("Your command has been issued.");
}

OSCallContext UOSAbilityManager::openControlCenter(const QString &module)
{
    qCDebug(logOsControl) << "Opening control center to module:" << module;

    if (!m_uosControlCenterProxy) {
        qCWarning(logOsControl) << "Control center proxy is not initialized";
        return ctxByError(OSCallContext::NonService);
    }

    QString actualModule = module;
    QString actualPage = "";
    if (module == "font") {
        actualModule = "personalization";
#ifdef COMPILE_ON_V20
        actualPage = "Font";
#elif COMPILE_ON_V25
        actualPage = "font";
#endif
    }

    QtConcurrent::run([this, actualModule, actualPage]() {
        QProcess::startDetached("dde-control-center", {});

        int result = m_uosControlCenterProxy->ShowPage(actualModule, actualPage);
        if (result != OSCallContext::NonError) {
            qCWarning(logOsControl) << "Failed to open control center module:" << actualModule << "result:" << result;
        } else {
            qCInfo(logOsControl) << "Control center module opened successfully:" << actualModule;
        }
    });

    return ctxByError(OSCallContext::NonError);
}

QStringList UOSAbilityManager::getAppsDesc()
{
    if (m_app2Desktop.isEmpty())
        return {};

    QStringList appsDesc;
    for (auto iter = m_app2Desktop.begin(); iter != m_app2Desktop.end(); iter++) {
        appsDesc << iter.value().appDesc;
    }

    return appsDesc;
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
    qCDebug(logOsControl) << "Loading app to desktop mappings";
    // 读取JSON文件
    QFile file(":/assets/app/deepin-app-infos.json");

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logOsControl) << "Failed to load uos app infos";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (!jsonDoc.isObject()) {
        qCWarning(logOsControl) << "Uos app infos file format error";
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
            appInfo.appDesc = app["appDesc"].toString();

            foreach (auto desktopFile, app["desktopFile"].toArray()) {
                appInfo.desktopFiles << desktopFile.toString();
            }

            m_app2Desktop.insert(appInfo.appId, appInfo);
        }
    }

    qCInfo(logOsControl) << "Loaded" << m_app2Desktop.size() << "app mappings from" << m_defaultDesktopPaths.size() << "desktop paths";
}

void UOSAbilityManager::initUosProxys()
{
    m_uosControlCenterProxy.reset(new DeepinControlCenter(this));
    m_uosNotificationProxy.reset(new DeepinNotification(this));
    m_uosAppLauncher.reset(new DeepinLauncher(m_defaultDesktopPaths, this));
    m_uosCalendarScheduler.reset(new DeepinCalendar(this));
    m_uosMultimediaProxy.reset(new DeepinMultimedia(this));
    qCInfo(logOsControl) << "UOS proxies initialized successfully";
}

void UOSAbilityManager::initDesktopPaths()
{
    QString systemDataDirs = UosInfo()->pureEnvironment().value("XDG_DATA_DIRS");
    QStringList systemDataPaths = systemDataDirs.split(":", PARAM_SKIP_EMPTY);

    foreach (auto p, systemDataPaths) {
        if (p.endsWith('/')) {
            p.chop(1);
        }
        m_defaultDesktopPaths << (p + QString("/applications"));
    }
    qCInfo(logOsControl) << "Initialized" << m_defaultDesktopPaths.size() << "desktop paths";
}

bool UOSAbilityManager::osCtrCallDbus(const QString &service, const QString &path, const QString &interface, const QString &method,
                                 QVariant &result, const QVariantList &arguments)
{
    qCDebug(logOsControl) << "Calling DBus method:" << method << "on service:" << service;
    auto conn = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, interface,  method);

    if (!arguments.isEmpty())
        msg.setArguments(arguments);

    // sync
    QDBusMessage reply = conn.call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qCWarning(logOsControl) << "DBus call failed - service:" << service
                               << "path:" << path
                               << "interface:" << interface
                               << "method:" << method
                               << "error:" << reply.errorMessage();
        return false;
    }

    if (reply.arguments().isEmpty()) {
        qCWarning(logOsControl) << "DBus call returned empty result";
        return false;
    }

    result = reply.arguments().at(0);
    qCDebug(logOsControl) << "DBus call completed successfully";
    return true;
}

bool UOSAbilityManager::osCtrCallDbusNoResult(const QString &service, const QString &path, const QString &interface, const QString &method, const QVariantList &arguments)
{
    qCDebug(logOsControl) << "Calling DBus method (no result):" << method << "on service:" << service;
    auto conn = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, interface,  method);

    if (!arguments.isEmpty())
        msg.setArguments(arguments);

    // sync
    QDBusMessage reply = conn.call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qCWarning(logOsControl) << "DBus call failed - service:" << service
                               << "path:" << path 
                               << "interface:" << interface 
                               << "method:" << method 
                               << "error:" << reply.errorMessage();
        return false;
    }

    qCDebug(logOsControl) << "DBus call completed successfully";
    return true;
}

bool UOSAbilityManager::propertiesGet(const QString &service, const QString &path, const QString &interface, const QString &propertyName, QVariant &value)
{
    qCDebug(logOsControl) << "Getting property:" << propertyName << "from service:" << service;
    auto conn = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, QString("org.freedesktop.DBus.Properties"), QString("Get"));

    QVariantList args;
    args << QVariant::fromValue(interface)
         << QVariant::fromValue(propertyName);
    msg.setArguments(args);

    QDBusMessage reply = conn.call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        qCWarning(logOsControl) << "Failed to get property:" << propertyName
                               << "error:" << reply.errorMessage();
        return false;
    }

    if (reply.arguments().isEmpty()) {
        qCWarning(logOsControl) << "Property get returned empty result";
        return false;
    }

    if (reply.arguments().at(0).canConvert<QDBusVariant>()) {
        value = QDBusReply<QVariant>(reply);
        qCDebug(logOsControl) << "Property get completed successfully";
        return true;
    }

    qCWarning(logOsControl) << "Property value cannot be converted to QDBusVariant";
    return false;
}

bool UOSAbilityManager::propertiesGetAll(const QString &interface, QVariantMap &values)
{
    //TODO
    return true;
}

bool UOSAbilityManager::propertiesSet(const QString &service, const QString &path, const QString &interface, const QString &propertyName, const QVariant &value)
{
    qCDebug(logOsControl) << "Setting property:" << propertyName << "on service:" << service;
    auto conn = QDBusConnection::sessionBus();
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path, QString("org.freedesktop.DBus.Properties"), QString("Set"));

    QVariantList args;
    args << QVariant::fromValue(interface)
         << QVariant::fromValue(propertyName)
         << QVariant::fromValue(QDBusVariant(QVariant::fromValue(value)));
    msg.setArguments(args);

    QDBusMessage reply = conn.call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qCWarning(logOsControl) << "Failed to set property:" << propertyName
                               << "error:" << reply.errorMessage();
        return false;
    }

    qCDebug(logOsControl) << "Property set completed successfully";
    return true;
}

// 音乐播放器功能实现
OSCallContext UOSAbilityManager::doStateControl(const QString &control)
{
    qCDebug(logOsControl) << "Controlling playback state:" << control;

    if (!m_uosMultimediaProxy) {
        return ctxByError(OSCallContext::NonService);
    }

    QString errorInfo;
    bool success = m_uosMultimediaProxy->stateControl(control, errorInfo);

    OSCallContext ctx;
    if (success) {
        ctx.error = OSCallContext::NonError;
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = errorInfo;
    }
    return ctx;
}

OSCallContext UOSAbilityManager::doSeek(int offset)
{
    qCDebug(logOsControl) << "Seeking to position:" << offset;

    if (!m_uosMultimediaProxy) {
        return ctxByError(OSCallContext::NonService);
    }

    QString errorInfo;
    bool success = m_uosMultimediaProxy->seek(offset, errorInfo);

    OSCallContext ctx;
    if (success) {
        ctx.error = OSCallContext::NonError;
    } else {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = errorInfo;
    }
    return ctx;
}

// 系统字号功能实现
OSCallContext UOSAbilityManager::doSystemFontSize(float size)
{
    qCDebug(logOsControl) << "Setting system font size:" << size;

    // 字号到真实值的映射
    QMap<int, double> SIZE_MAPPING = {
        {11, 8.25},
        {12, 9.0},
        {13, 9.75},
        {14, 10.5},
        {15, 11.25},
        {16, 12.0},
        {18, 13.5},
        {20, 15.0}
    };

    // 检查字号是否有效
    int intSize = static_cast<int>(size);
    if (!SIZE_MAPPING.contains(intSize)) {
        qCWarning(logOsControl) << "Invalid font size:" << size;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 设置字体大小
    double realSize = SIZE_MAPPING[intSize];
    if (!propertiesSet(osCallDbusAppearanceService, osCallDbusAppearancePath,
                      osCallDbusAppearanceInterface, "FontSize", realSize)) {
        qCWarning(logOsControl) << "Failed to set font size";
        return ctxByError(OSCallContext::NonService);
    }

    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.result["fontSize"] = intSize;
    return ctx;
}

OSCallContext UOSAbilityManager::getSystemFontSize()
{
    qCDebug(logOsControl) << "Getting system font size";

    // 字号到真实值的映射
    QMap<int, double> SIZE_MAPPING = {
        {11, 8.25},
        {12, 9.0},
        {13, 9.75},
        {14, 10.5},
        {15, 11.25},
        {16, 12.0},
        {18, 13.5},
        {20, 15.0}
    };

    // 获取当前字体大小
    QVariant currentSizeVariant;
    if (!propertiesGet(osCallDbusAppearanceService, osCallDbusAppearancePath,
                      osCallDbusAppearanceInterface, "FontSize", currentSizeVariant)) {
        qCWarning(logOsControl) << "Failed to get font size";
        return ctxByError(OSCallContext::NonService);
    }

    double currentValue = currentSizeVariant.toDouble();
    qCDebug(logOsControl) << "Current font size value:" << currentValue;

    // 查找最接近的预设字号
    int closestSize = 11;
    double minDiff = qAbs(SIZE_MAPPING[11] - currentValue);
    for (auto iter = SIZE_MAPPING.begin(); iter != SIZE_MAPPING.end(); ++iter) {
        double diff = qAbs(iter.value() - currentValue);
        if (diff < minDiff) {
            minDiff = diff;
            closestSize = iter.key();
        }
    }

    qCInfo(logOsControl) << "Current system font size:" << closestSize;

    OSCallContext ctx;
    ctx.error = OSCallContext::NonError;
    ctx.result["fontSize"] = closestSize;
    ctx.output = QString("Current system font size: %1").arg(closestSize);
    return ctx;
}

// 文件操作功能实现
OSCallContext UOSAbilityManager::doOpenFile(const QString &filePath)
{
    qCDebug(logOsControl) << "Opening file:" << filePath;

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        qCWarning(logOsControl) << "File not found:" << filePath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 使用xdg-open打开文件
    if (!QProcess::startDetached("xdg-open", {filePath})) {
        qCWarning(logOsControl) << "Failed to start xdg-open";
        return ctxByError(OSCallContext::NonService);
    }

   return OSCallContext{};
}

OSCallContext UOSAbilityManager::doCopyFile(const QString &sourcePath, const QString &destinationPath)
{
    qCDebug(logOsControl) << "Copying file from" << sourcePath << "to" << destinationPath;

    // 检查源文件是否存在
    if (!QFile::exists(sourcePath)) {
        qCWarning(logOsControl) << "Source file not found:" << sourcePath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 使用QFile复制文件
    if (!QFile::copy(sourcePath, destinationPath)) {
        qCWarning(logOsControl) << "Failed to copy file";
        return ctxByError(OSCallContext::NonService);
    }

    return OSCallContext{};
}

OSCallContext UOSAbilityManager::doMoveFile(const QString &sourcePath, const QString &destinationPath)
{
    qCDebug(logOsControl) << "Moving file from" << sourcePath << "to" << destinationPath;

    // 检查源文件是否存在
    if (!QFile::exists(sourcePath)) {
        qCWarning(logOsControl) << "Source file not found:" << sourcePath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 使用QFile重命名文件实现移动
    if (!QFile::rename(sourcePath, destinationPath)) {
        qCWarning(logOsControl) << "Failed to move file";
        return ctxByError(OSCallContext::NonService);
    }

    return OSCallContext{};
}

OSCallContext UOSAbilityManager::doCreateFolder(const QString &folderPath)
{
    qCDebug(logOsControl) << "Creating folder:" << folderPath;

    // 使用QDir创建文件夹
    QDir dir;
    if (!dir.mkpath(folderPath)) {
        qCWarning(logOsControl) << "Failed to create folder";
        return ctxByError(OSCallContext::NonService);
    }

    return OSCallContext{};
}

OSCallContext UOSAbilityManager::doRenameFile(const QString &oldPath, const QString &newName)
{
    qCDebug(logOsControl) << "Renaming file:" << oldPath << "to" << newName;

    // 检查源文件是否存在
    if (!QFile::exists(oldPath)) {
        qCWarning(logOsControl) << "File not found:" << oldPath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 构建新路径
    QFileInfo fileInfo(oldPath);
    QString newPath = fileInfo.absolutePath() + "/" + newName;

    // 使用QFile重命名文件
    if (QFile::rename(oldPath, newPath)) {
        qCWarning(logOsControl) << "Failed to rename file";
        return ctxByError(OSCallContext::NonService);
    }

    return OSCallContext{};
}

OSCallContext UOSAbilityManager::doBatchRename(const QString &folderPath, const QString &newName, const QString &pattern)
{
    qCDebug(logOsControl) << "Batch renaming files in" << folderPath << "with pattern:" << pattern;

    // 检查文件夹是否存在
    QDir dir(folderPath);
    if (!dir.exists()) {
        qCWarning(logOsControl) << "Folder not found:" << folderPath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 获取文件夹中的所有文件
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    if (files.isEmpty()) {
        qCWarning(logOsControl) << "Folder is empty:" << folderPath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 如果有正则表达式模式，过滤文件
    if (!pattern.isEmpty()) {
        QRegularExpression regex(pattern);
        if (!regex.isValid()) {
            qCWarning(logOsControl) << "Invalid regex pattern:" << pattern;
            return ctxByError(OSCallContext::InvalidArgs);
        }
        QFileInfoList filteredFiles;
        for (const QFileInfo &fileInfo : files) {
            if (regex.match(fileInfo.fileName()).hasMatch()) {
                filteredFiles.append(fileInfo);
            }
        }
        if (filteredFiles.isEmpty()) {
            return ctxByError(OSCallContext::InvalidArgs);
        }
        files = filteredFiles;
    }

    int renamedCount = 0;
    QStringList errors;

    for (int i = 0; i < files.size(); ++i) {
        const QFileInfo &fileInfo = files[i];
        QString oldPath = fileInfo.absoluteFilePath();
        QString extension = fileInfo.suffix();
        QString newFileName;

        // 构建新文件名
        if (files.size() == 1) {
            newFileName = QString("%1.%2").arg(newName).arg(extension);
        } else {
            newFileName = QString("%1_%2.%3").arg(newName).arg(i + 1).arg(extension);
        }

        QString newPath = fileInfo.absolutePath() + "/" + newFileName;

        // 重命名文件
        if (QFile::rename(oldPath, newPath)) {
            renamedCount++;
        } else {
            errors.append(tr("Failed to rename file %1").arg(fileInfo.fileName()));
        }
    }

    OSCallContext ctx;
    if (!errors.isEmpty()) {
        ctx.output = tr("Batch rename completed, successfully renamed %1 files, failed %2 files.\nError details:\n%3")
                     .arg(renamedCount).arg(errors.size()).arg(errors.join("\n"));
    }

    return ctx;
}

OSCallContext UOSAbilityManager::doReadFile(const QString &filePath)
{
    qCDebug(logOsControl) << "Reading file:" << filePath;

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        qCWarning(logOsControl) << "File not found:" << filePath;
        return ctxByError(OSCallContext::InvalidArgs);
    }

    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(logOsControl) << "Failed to open file for reading:" << filePath;
        return ctxByError(OSCallContext::NonService);
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    OSCallContext ctx;
    ctx.output = content;
    return ctx;
}

OSCallContext UOSAbilityManager::doGetFileMetadata(const QStringList &fileList)
{
    qCDebug(logOsControl) << "Getting metadata for files:" << fileList;

    if (fileList.isEmpty()) {
        qCWarning(logOsControl) << "No files provided";
        return ctxByError(OSCallContext::InvalidArgs);
    }

    QJsonArray results;
    for (const QString &filePath : fileList) {
        if (!QFile::exists(filePath)) {
            QJsonObject errorObj;
            errorObj["path"] = filePath;
            errorObj["error"] = "File not found";
            results.append(errorObj);
            continue;
        }

        QFileInfo fileInfo(filePath);
        QJsonObject metadata;
        metadata["name"] = fileInfo.fileName();
        metadata["size"] = static_cast<qint64>(fileInfo.size());
        metadata["created"] = fileInfo.birthTime().toString(Qt::ISODate);
        metadata["modified"] = fileInfo.lastModified().toString(Qt::ISODate);

        // 确定文件类型
        QString suffix = fileInfo.suffix().toLower();
        QString type = "other";
        if (QStringList({"jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"}).contains(suffix)) {
            type = "image";
        } else if (QStringList({"mp3", "wav", "ogg", "flac", "aac", "m4a"}).contains(suffix)) {
            type = "audio";
        } else if (QStringList({"mp4", "avi", "mkv", "mov", "webm"}).contains(suffix)) {
            type = "video";
        } else if (suffix == "desktop") {
            type = "app";
        } else if (QStringList({"pdf", "doc", "docx", "txt", "rtf", "odt"}).contains(suffix)) {
            type = "document";
        } else if (QStringList({"py", "js", "java", "c", "cpp", "h", "html", "css"}).contains(suffix)) {
            type = "code";
        }
        metadata["type"] = type;

        results.append(metadata);
    }

    OSCallContext ctx;
    ctx.output = QJsonDocument(results).toJson(QJsonDocument::Indented);
    return ctx;
}

// 邮件操作功能实现
OSCallContext UOSAbilityManager::doSendMail(const QString &subject, const QString &content, const QStringList &toList, const QStringList &ccList, const QStringList &bccList)
{
    qCDebug(logOsControl) << "Sending mail to:" << toList;

    // 创建一个mailto链接
    QUrl mailtoUrl;
    QUrlQuery queryItem;
    mailtoUrl.setScheme("mailto");

    // 设置收件人
    if (!toList.join(",").isEmpty()) {
        mailtoUrl.setPath(toList.join(","));
    }

    // 设置抄送人
    if (!ccList.join(",").isEmpty()) {
        queryItem.addQueryItem("cc", ccList.join(","));
    }

    // 设置密送人
    if (!bccList.join(",").isEmpty()) {
        queryItem.addQueryItem("bcc", bccList.join(","));
    }

    // 设置主题
    if (!subject.isEmpty()) {
        queryItem.addQueryItem("subject", QUrl::toPercentEncoding(subject));
    }

    // 设置正文
    if (!content.isEmpty()) {
        queryItem.addQueryItem("body", QUrl::toPercentEncoding(content));
    }

    queryItem.addQueryItem("type", "1");
    mailtoUrl.setQuery(queryItem);

    // 使用xdg-open打开邮件客户端
    if (!QProcess::startDetached("xdg-open", {mailtoUrl.toString()})) {
        qCWarning(logOsControl) << "Failed to start xdg-open";
        return ctxByError(OSCallContext::NonService);
    }

    OSCallContext ctx;
    ctx.output = tr("Email client has been opened, please confirm to send");
    return ctx;
}

// 蓝牙设备列表功能实现
OSCallContext UOSAbilityManager::doGetBluetoothDevices()
{
    qCDebug(logOsControl) << "Getting bluetooth devices";

    // 获取适配器列表
    QVariant adaptersResult;
    if (!osCtrCallDbus(osCallDbusBtService, osCallDbusBtPath, osCallDbusBtInterface,
                      "GetAdapters", adaptersResult)) {
        qCWarning(logOsControl) << "Failed to get bluetooth adapters";
        return ctxByError(OSCallContext::NonService);
    }

    QJsonDocument doc = QJsonDocument::fromJson(adaptersResult.toString().toUtf8());
    QJsonArray adapters = doc.array();

    QJsonArray allDevices;

    for (const QJsonValue adapterValue : adapters) {
        QJsonObject adapter = adapterValue.toObject();
        QString adapterPath = adapter["Path"].toString();

        // 获取每个适配器的设备列表
        QVariant devicesResult;
        QVariantList args;
        args << QVariant::fromValue(QDBusObjectPath(adapterPath));

        if (!osCtrCallDbus(osCallDbusBtService, osCallDbusBtPath, osCallDbusBtInterface,
                          "GetDevices", devicesResult, args)) {
            qCWarning(logOsControl) << "Failed to get devices for adapter:" << adapterPath;
            continue;
        }

        QJsonDocument devicesDoc = QJsonDocument::fromJson(devicesResult.toString().toUtf8());
        QJsonArray devices = devicesDoc.array();

        // 添加所有设备到结果列表
        for (const QJsonValue deviceValue : devices) {
            allDevices.append(deviceValue);
        }
    }

    OSCallContext ctx;
    ctx.output = QJsonDocument(allDevices).toJson(QJsonDocument::Indented);
    return ctx;
}

// 应用商店功能实现
OSCallContext UOSAbilityManager::doSearchApp(const QString &keyword, int page, int maxResults)
{
    qCDebug(logOsControl) << "Searching app with keyword:" << keyword;

    StoreAPI *storeAPI = new StoreAPI(this);

    QEventLoop eventLoop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(30000);

    OSCallContext ctx;
    bool searchCompleted = false;
    QString searchError;
    QList<SearchResult> searchResults;

    connect(storeAPI, &StoreAPI::searchFinished, [&](bool success, const QString &error, const QList<SearchResult> &results) {
        searchCompleted = true;
        searchError = error;
        searchResults = results;

        if (success) {
            QJsonArray resultsArray;
            for (const SearchResult &result : results) {
                QJsonObject appObj;
                appObj["appName"] = result.appName;
                appObj["packageName"] = result.packageName;
                appObj["version"] = result.version;
                appObj["icon"] = result.icon;
                appObj["description"] = result.description;
                appObj["score"] = result.score;
                appObj["downloadCount"] = result.downloadCount;
                appObj["category"] = result.category;
                appObj["appid"] = result.appid;
                resultsArray.append(appObj);
            }

            QJsonObject responseObj;
            responseObj["keyword"] = keyword;
            responseObj["page"] = page;
            responseObj["maxResults"] = maxResults;
            responseObj["count"] = results.size();
            responseObj["results"] = resultsArray;

            ctx.output = QJsonDocument(responseObj).toJson(QJsonDocument::Indented);
            ctx.error = OSCallContext::NonError;
        } else {
            ctx.error = OSCallContext::NonService;
            ctx.errorInfo = error;
        }

        eventLoop.quit();
    });

    connect(&timer, &QTimer::timeout, [&]() {
        if (!searchCompleted) {
            qCWarning(logOsControl) << "Search app timeout";
            ctx.error = OSCallContext::NonService;
            ctx.errorInfo = "Search timeout";
            eventLoop.quit();
        }
    });

    storeAPI->searchApps(keyword, page, maxResults);

    eventLoop.exec();

    storeAPI->deleteLater();

    return ctx;
}

OSCallContext UOSAbilityManager::doDownloadApp(const QString &appName)
{
    qCDebug(logOsControl) << "Downloading app:" << appName;

    // 调用LocalModelServer的openInstallWidget方法打开应用商店安装界面
    LocalModelServer::getInstance().openInstallWidget(appName);

    OSCallContext ctx;
    ctx.output = tr("App store has been opened, preparing to install application: %1").arg(appName);

    return ctx;
}

OSCallContext UOSAbilityManager::doShowStoreTab(const QString &tabName)
{
    OSCallContext ctx;
    QString normalizedTarget = tabName.trimmed();

    if (normalizedTarget.isEmpty()) {
        ctx.error = OSCallContext::InvalidArgs;
        ctx.errorInfo = tr("App store target is empty");
        qCWarning(logOsControl) << "Failed to open app store: target is empty";
        return ctx;
    }

    normalizedTarget = "tab/" + normalizedTarget;
    qCDebug(logOsControl) << "Opening app store target:" << normalizedTarget;

    StoreAPI storeAPI;
    if (!storeAPI.openTargetInAppStore(normalizedTarget)) {
        ctx.error = OSCallContext::NonService;
        ctx.errorInfo = tr("Failed to open App Store target: %1").arg(normalizedTarget);
        qCWarning(logOsControl) << "Failed to open app store target:" << normalizedTarget;
        return ctx;
    }

    ctx.output = tr("App store has been opened for: %1").arg(normalizedTarget);

    return ctx;
}
