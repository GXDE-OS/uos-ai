#ifndef DEEPINABILITYMANAGER_H
#define DEEPINABILITYMANAGER_H
#include "esingleton.h"
#include "oscallcontext.h"
#include "ability/launcherability.h"
#include "ability/scheduleability.h"
#include "ability/notificationability.h"
#include "ability/controlcenterability.h"
#include "deepinmultimedia.h"

#include <QObject>
#include <QMap>
#include <QDBusInterface>
#include <QScopedPointer>

UOSAI_USE_NAMESPACE

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
     * @param adjustment 0 = absolute, 1 = increase, 2 = decrease
     * @return
     */
    OSCallContext doDiplayBrightness(int value, int adjustment=0);
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
    OSCallContext doPerformanceModeSwitch(const QString &mode, bool isOpen);
    OSCallContext openShutdownFront();
    OSCallContext openScreenShot();
    OSCallContext doDisplayModeSwitch(int mode);
    OSCallContext openGrandSearch();
    OSCallContext switchScreen();
    OSCallContext volumeAdjustment(const QJsonObject &argsObj);

    // 音乐播放器功能
    OSCallContext doStateControl(const QString &control);
    OSCallContext doSeek(int offset);

public:
    QString textForCommnand();
    QStringList getAppsDesc();
signals:

public:
    static bool osCtrCallDbus(const QString &service, const QString &path, const QString &interface, const QString &method,
                      QVariant &result, const QVariantList &arguments = QVariantList());
    static bool osCtrCallDbusNoResult(const QString &service, const QString &path, const QString &interface, const QString &method, const QVariantList &arguments = QVariantList());
    static bool propertiesGet(const QString &service, const QString &path, const QString &interface, const QString &propertyName, QVariant &value);
    static bool propertiesGetAll(const QString &interface, QVariantMap &values);
    static bool propertiesSet(const QString &service, const QString &path, const QString &interface, const QString &propertyName, const QVariant &value);

protected:
    void loadErrMap();
    void loadApp2Desktop();
    void initUosProxys();
    void initDesktopPaths();
protected:
    QMap<OSCallContext::CallError, QString> m_errMap;

    QScopedPointer<IControlCenter> m_uosControlCenterProxy;
    QScopedPointer<INotification> m_uosNotificationProxy;
    QScopedPointer<ILauncher> m_uosAppLauncher;
    QScopedPointer<ISchedule> m_uosCalendarScheduler;
    QScopedPointer<DeepinMultimedia> m_uosMultimediaProxy;

    struct UosAppInfo {
        QString appId;
        QString appName;
        QString appIcon;
        QString appDesc;
        //App may have different versions
        //Uos,WINE,or andorid.
        QStringList desktopFiles;
    };

    QStringList m_defaultDesktopPaths;
    QMap<QString, UosAppInfo> m_app2Desktop;
private:
    inline OSCallContext ctxByError(OSCallContext::CallError error) {
        OSCallContext ctx;
        ctx.error = error;
        ctx.errorInfo = m_errMap[error];
        return ctx;
    }
};

#define UosAbility() (ESingleton<UOSAbilityManager>::getInstance())
#endif // DEEPINABILITYMANAGER_H
