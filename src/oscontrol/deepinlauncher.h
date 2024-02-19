#ifndef DEEPINLAUNCHER_H
#define DEEPINLAUNCHER_H
#include "ability/launcherability.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDBusInterface>

class DeepinLauncher : public QObject,  public ILauncher
{
    Q_OBJECT
public:
    explicit DeepinLauncher(bool isLinglong,
                            const QStringList &deskPaths = QStringList(),
                            QObject *parent = nullptr);
    virtual ~DeepinLauncher() override;

    virtual int showLauncher() override;
    virtual int launchDesktop(const QString &pathApp) override;
    virtual int launchDefault(const QString &mineType) override;
    virtual int listApps(const QString &mime) override;
protected:
    //V23 new AM utility functions
    QString escapeToObjectPath(const QString &str);
    QString unescapeFromObjectPath(const QString &str);
    QString getAppIdFromAbsolutePath(const QString &path);

    int launchDesktopWithV23AM(const QString &pathApp, bool isAppPath = false);

private:
    QString getAppDesktopFile(const QString &DesktopName);

    QScopedPointer<QDBusInterface> m_osMime;
    QScopedPointer<QDBusInterface> m_oslauncher;
    QScopedPointer<QDBusInterface> m_osStartManager;
    QScopedPointer<QDBusInterface> m_v23NewAM;
    QScopedPointer<QDBusInterface> m_v23NewMIME;

    bool m_fIsLinglong;
    QStringList m_defaultDesktopPaths;

    int m_callTimeout {5000}; //5s
};

#endif // DEEPINLAUNCHER_H
