#ifndef DEEPINABILITYMANAGER_H
#define DEEPINABILITYMANAGER_H

#include "oscallcontext.h"

#include <QObject>
#include <QMap>
#include <QDBusInterface>
#include <QScopedPointer>
#include <QEventLoop>
#include <QTimer>

namespace uos_ai {
class DeepinControlCenter;
class DeepinNotification;
class DeepinLauncher;
class DeepinCalendar;
class DeepinMultimedia;

class UOSAbilityManager : public QObject
{
    Q_OBJECT
    explicit UOSAbilityManager(QObject *parent = nullptr);

public:
    static UOSAbilityManager *instance();
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

    // 系统字号
    OSCallContext doSystemFontSize(float size);
    OSCallContext getSystemFontSize();

    // 文件操作
    OSCallContext doOpenFile(const QString &filePath);
    OSCallContext doCopyFile(const QString &sourcePath, const QString &destinationPath);
    OSCallContext doMoveFile(const QString &sourcePath, const QString &destinationPath);
    OSCallContext doCreateFolder(const QString &folderPath);
    OSCallContext doRenameFile(const QString &oldPath, const QString &newName);
    OSCallContext doBatchRename(const QString &folderPath, const QString &newName, const QString &pattern = QString());
    OSCallContext doReadFile(const QString &filePath);
    OSCallContext doGetFileMetadata(const QStringList &fileList);

    // 邮件操作
    OSCallContext doSendMail(const QString &subject, const QString &content, const QStringList &toList, const QStringList &ccList = QStringList(), const QStringList &bccList = QStringList());

    // 蓝牙设备列表
    OSCallContext doGetBluetoothDevices();

    // 应用商店功能
    OSCallContext doSearchApp(const QString &keyword, int page = 1, int maxResults = 3);
    OSCallContext doDownloadApp(const QString &appName);
    OSCallContext doShowStoreTab(const QString &tabName);

    // 控制中心功能
    /**
     * @brief 打开控制中心并定位到指定模块
     * @param module 模块名称 (如 "sound", "display", "appearance", "bluetooth", "wifi", "power" 等)
     */
    OSCallContext openControlCenter(const QString &module);

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

    QScopedPointer<DeepinControlCenter> m_uosControlCenterProxy;
    QScopedPointer<DeepinNotification> m_uosNotificationProxy;
    QScopedPointer<DeepinLauncher> m_uosAppLauncher;
    QScopedPointer<DeepinCalendar> m_uosCalendarScheduler;
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

}

#define UosAbility() UOSAbilityManager::instance()
#endif // DEEPINABILITYMANAGER_H
