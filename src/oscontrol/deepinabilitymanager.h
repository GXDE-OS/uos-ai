#ifndef DEEPINABILITYMANAGER_H
#define DEEPINABILITYMANAGER_H
#include "esingleton.h"
#include "oscallcontext.h"
#include "ability/launcherability.h"
#include "ability/scheduleability.h"
#include "ability/notificationability.h"
#include "ability/controlcenterability.h"

#include <QObject>
#include <QMap>
#include <QDBusInterface>
#include <QScopedPointer>

//From dframeworkdbus2
#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_daemon_appearance.h>
#include <com_deepin_daemon_display.h>
#include <com_deepin_dde_controlcenter.h>
#include <com_deepin_dde_notification.h>
#include <com_deepin_wm.h>
#include <com_deepin_daemon_bluetooth.h>
#include <com_deepin_daemon_network.h>
#include <com_deepin_system_systempower.h>


class UOSAbilityManager : public QObject
{
    Q_OBJECT

    SINGLETONIZE_CLASS(UOSAbilityManager);

    explicit UOSAbilityManager(QObject *parent = nullptr);

public:
    /**
     * @brief doBluetoothConfig
     * @return Do bluetooth config,
     *      show bluetooth config UI
     */
    OSCallContext doBluetoothConfig(bool on);
    /**
     * @brief doScreenMirroring
     * @return Do screen mirror
     */
    OSCallContext doScreenMirroring(bool state = true);
    /**
     * @brief doNoDisturb
     * @param state
     *    True for Open os no disturb feature
     *    False for close.
     * @return
     */
    OSCallContext doNoDisturb(bool state = true);
    /**
     * @brief doSwitchWallpaper
     * @return Switch to next wallpaper
     */
    OSCallContext doWallpaperSwitch();
    /**
     * @brief doDesktopOrganize
     * @return Organize desktop
     */
    OSCallContext doDesktopOrganize(bool state = true);
    /**
     * @brief doDockModeSwitch
     * @param mode  0 Fashion mode; 1 Efficent mode
     * @return
     */
    OSCallContext doDockModeSwitch(int mode);

    /**
     * @brief doDockModeSwitch
     * @param mode  0 Light; 1 Dark; 2 Auto
     * @return
     */
    OSCallContext doSystemThemeSwitch(int theme);

    /**
     * @brief doDiplayEyesProtection
     * @return on
     *      True for open,False for close
     */
    OSCallContext doDiplayEyesProtection(bool on = true);
    /**
     * @brief doDiplayBrightness
     * @param value 0-100
     * @return
     */
    OSCallContext doDiplayBrightness(int value);
    /**
     * @brief doAppLaunch
     * @param appId application id Ai supplied
     * @return
     */
    OSCallContext doAppLaunch(const QString &appId, bool on);
    /**
     * @brief doCreateSchedule
     * @param title
     * @param startTime
     * @param endTime
     * @return
     */
    OSCallContext doCreateSchedule(const QString &title
                                   , const QString &startTime
                                   , const QString &endTime);

    OSCallContext switchWifi(bool on);
    OSCallContext getSystemMemory();
    OSCallContext doSystemLanguageSetting();
    OSCallContext doPerformanceModeSwitch(const QString &mode);
    OSCallContext openShutdownFront();
    OSCallContext openScreenShot();
    OSCallContext doDisplayModeSwitch(int mode);
    OSCallContext openGrandSearch();
    OSCallContext switchScreen();
public:
    QString textForCommnand();
signals:

protected:
    void loadErrMap();
    void loadApp2Desktop();
    void initUosProxys();
    void initDesktopPaths();
protected:
    QMap<OSCallContext::CallError, QString> m_errMap;

    using UosDock = com::deepin::dde::daemon::Dock;
    using UosAppearance = com::deepin::daemon::Appearance;
    using UosDisplay = com::deepin::daemon::Display;
    using UosControlCenter = com::deepin::dde::ControlCenter;
    using UosNotification = com::deepin::dde::Notification;
    using UosWM = com::deepin::wm;
    using UosBluetooth = com::deepin::daemon::Bluetooth;
    using UosNetwork = com::deepin::daemon::Network;
    using UosPower = com::deepin::system::Power;

    QScopedPointer<UosDock> m_uosDockProxy;
    QScopedPointer<UosAppearance> m_uosAppearanceProxy;
    QScopedPointer<UosDisplay> m_uosDisplayProxy;
    QScopedPointer<UosWM> m_uosWM;
    QScopedPointer<UosBluetooth> m_bluetooth;
    QScopedPointer<UosNetwork> m_network;
    QScopedPointer<UosPower> m_power;

    //Control center isn't a resident service
    //so don't need to check if the dbus is valid
    //when call it.
    QScopedPointer<IControlCenter> m_uosControlCenterProxy;
    QScopedPointer<INotification> m_uosNotificationProxy;
    QScopedPointer<QDBusInterface> m_uosDesktopProxy;
    QScopedPointer<QDBusInterface> m_shutdownFrontProxy;
    QScopedPointer<QDBusInterface> m_screenShotProxy;

    QScopedPointer<ILauncher> m_uosAppLauncher;
    QScopedPointer<ISchedule> m_uosCalendarScheduler;

    struct UosAppInfo {
        QString appId;
        QString appName;
        QString appIcon;
        //App may have different versions
        //Uos,WINE,or andorid.
        QStringList desktopFiles;
    };

    QStringList m_defaultDesktopPaths;
    QMap<QString, UosAppInfo> m_app2Desktop;

    bool m_fIsLinglong;
};

#define UosAbility() (ESingleton<UOSAbilityManager>::getInstance())
#endif // DEEPINABILITYMANAGER_H
