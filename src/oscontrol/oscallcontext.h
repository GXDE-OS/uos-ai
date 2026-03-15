#ifndef OSCALLCONTEXT_H
#define OSCALLCONTEXT_H

#include "uosai_global.h"

#include <QString>

UOSAI_BEGIN_NAMESPACE
struct OSCallContext {
    enum CallError {
        NonError = 0,
        NotImpl  = 1,
        NonService,
        InvalidArgs,
        AppNotFound,
        AppStartFailed
    } error;

    QString errorInfo;
    QString output;

    OSCallContext() : error(CallError::NonError), errorInfo(""), output("") {}
};
inline constexpr char osCallDbusWmService[] = "com.deepin.wm";
inline constexpr char osCallDbusWmPath[] = "/com/deepin/wm";
inline constexpr char osCallDbusWmInterface[] = "com.deepin.wm";

inline constexpr char osCallDbusBtService[] = "com.deepin.daemon.Bluetooth";
inline constexpr char osCallDbusBtPath[] = "/com/deepin/daemon/Bluetooth";
inline constexpr char osCallDbusBtInterface[] = "com.deepin.daemon.Bluetooth";

inline constexpr char osCallDbusPowerService[] = "com.deepin.system.Power";
inline constexpr char osCallDbusPowerPath[] = "/com/deepin/system/Power";
inline constexpr char osCallDbusPowerInterface[] = "com.deepin.system.Power";

inline constexpr char osCallDbusShutDownService[] = "com.deepin.dde.shutdownFront";
inline constexpr char osCallDbusShutDownPath[] = "/com/deepin/dde/shutdownFront";
inline constexpr char osCallDbusShutDownInterface[] = "com.deepin.dde.shutdownFront";

inline constexpr char osCallDbusScreenshotService[] = "com.deepin.Screenshot";
inline constexpr char osCallDbusScreenshotPath[] = "/com/deepin/Screenshot";
inline constexpr char osCallDbusScreenshotInterface[] = "com.deepin.Screenshot";

inline constexpr char osCallDbusDockService[] = "com.deepin.dde.daemon.Dock";
inline constexpr char osCallDbusDockPath[] = "/com/deepin/dde/daemon/Dock";
inline constexpr char osCallDbusDockInterface[] = "com.deepin.dde.daemon.Dock";

inline constexpr char osCallDbusAppearanceService[] = "com.deepin.daemon.Appearance";
inline constexpr char osCallDbusAppearancePath[] = "/com/deepin/daemon/Appearance";
inline constexpr char osCallDbusAppearanceInterface[] = "com.deepin.daemon.Appearance";

inline constexpr char osCallDbusDisplayService[] = "com.deepin.daemon.Display";
inline constexpr char osCallDbusDisplayPath[] = "/com/deepin/daemon/Display";
inline constexpr char osCallDbusDisplayInterface[] = "com.deepin.daemon.Display";

inline constexpr char osCallDbusCtrCenterService[] = "com.deepin.dde.ControlCenter";
inline constexpr char osCallDbusCtrCentertPath[] = "/com/deepin/dde/ControlCenter";
inline constexpr char osCallDbusCtrCenterInterface[] = "com.deepin.dde.ControlCenter";

inline constexpr char osCallDbusNotifyService[] = "com.deepin.dde.Notification";
inline constexpr char osCallDbusNotifyPath[] = "/com/deepin/dde/Notification";
inline constexpr char osCallDbusNotifyInterface[] = "com.deepin.dde.Notification";
// Launcher
inline constexpr char deepinLancherService[] = "com.deepin.dde.Launcher";
inline constexpr char deepinLancherPath[] = "/com/deepin/dde/Launcher";
inline constexpr char deepinLancherInterface[] = "com.deepin.dde.Launcher";

inline constexpr char deepinStartManagerService[] = "com.deepin.SessionManager";
inline constexpr char deepinStartManagerPath[] = "/com/deepin/StartManager";
inline constexpr char deepinStartManagerInterface[] = "com.deepin.StartManager";

inline constexpr char deepinMimeService[] = "com.deepin.daemon.Mime";
inline constexpr char deepinMimePath[] = "/com/deepin/daemon/Mime";
inline constexpr char deepinMimeInterface[] = "com.deepin.daemon.Mime";

inline constexpr char osCallDbusAudioService[] = "com.deepin.daemon.Audio";
inline constexpr char osCallDbusAudioPath[] = "/com/deepin/daemon/Audio";
inline constexpr char osCallDbusAudioInterface[] = "com.deepin.daemon.Audio";

inline constexpr char osCallDbusNetworkService[] = "com.deepin.daemon.Network";
inline constexpr char osCallDbusNetworkPath[] = "/com/deepin/daemon/Network";
inline constexpr char osCallDbusNetworkInterface[] = "com.deepin.daemon.Network";

UOSAI_END_NAMESPACE
#endif // OSCALLCONTEXT_H
