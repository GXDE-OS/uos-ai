#ifndef DEEPINLAUNCHER_H
#define DEEPINLAUNCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDBusInterface>
namespace uos_ai {
class DeepinLauncher : public QObject
{
    Q_OBJECT
public:
    explicit DeepinLauncher(const QStringList &deskPaths = QStringList(),
                            QObject *parent = nullptr);
    virtual ~DeepinLauncher() override;

    virtual int showLauncher();
    virtual int launchDesktop(const QString &pathApp);
    virtual int launchDefault(const QString &mineType);
    virtual int listApps(const QString &mime);
protected:
    //V23 new AM utility functions
    QString escapeToObjectPath(const QString &str);
    QString unescapeFromObjectPath(const QString &str);
    QString getAppIdFromAbsolutePath(const QString &path);

private:
    QString getAppDesktopFile(const QString &DesktopName);

    QScopedPointer<QDBusInterface> m_osMime;
    QScopedPointer<QDBusInterface> m_oslauncher;
    QScopedPointer<QDBusInterface> m_osStartManager;

    QStringList m_defaultDesktopPaths;
    int m_callTimeout {5000}; //5s
};
} // namespace uos_ai
#endif // DEEPINLAUNCHER_H
