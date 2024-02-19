#ifndef LAUNCHERABILITY_H
#define LAUNCHERABILITY_H

#include <QString>
#include <QStringList>

class ILauncher
{
public:
    virtual ~ILauncher() {}

    /* 显示启动页面 deepin的启动，windows等WIN按钮 */
    virtual int showLauncher() = 0;
    /* 启动*.desktop文件 */
    virtual int launchDesktop(const QString &pathApp) = 0;
    /* 启动默认程序 */
    virtual int launchDefault(const QString &category) = 0;
    /* Get default app list of MIME */
    virtual int listApps(const QString &mime) = 0;
};

#endif // LAUNCHERABILITY_H
