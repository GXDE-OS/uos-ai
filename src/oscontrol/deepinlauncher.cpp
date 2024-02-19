/*
 * Use QDBus to launch the application
 *
 * Deepin system's application in /usr/share/applications/xxx.desktop has
 * read and write permission, you can't launch it even if you're root.
 * There is a right method to launch application through DBus system.
 *
 */
#include "deepinlauncher.h"
#include "oscallcontext.h"

#include <QFileInfo>
#include <QDebug>
#include <QDBusPendingCall>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDir>
#include <DStandardPaths>
#include <QDBusObjectPath>

DeepinLauncher::DeepinLauncher(bool isLinglong,
                               const QStringList &deskPaths,
                               QObject *parent)
    : QObject(parent)
    , m_fIsLinglong(isLinglong)
    , m_defaultDesktopPaths(deskPaths)
{
    QString deepinLauncherService;
    QString deepinLauncherPath;
    QString deepinLauncherInterface;

    QString deepinStartManagerService;
    QString deepinStartManagerPath;
    QString deepinStartManagerInterface;

    QString deepinMimeService;
    QString deepinMimePath;
    QString deepinMimeInterface;

    QString deepinNewAppManagerService;
    QString deepinNewAppManagerPath;
    QString deepinNewAppManagerInterface;

    QString deepinNewMimeService;
    QString deepinNewMimePath;
    QString deepinNewMimeInterface;

    /*V23
     *
     * Show OS launcher:
     * qdbus --literal org.deepin.dde.Launcher1
     *      /org/deepin/dde/Launcher1
     *      org.deepin.dde.Launcher1.Show
     *
     *
     * Start application:
     * qdbus --literal org.deepin.dde.StartManager1
     *      /org/deepin/dde/StartManager1
     *      org.deepin.dde.StartManager1.Launch
     *          "/usr/share/applications/dde-computer.desktop"
     *
     * Mime service:
     *  qdbus --literal org.deepin.dde.Mime1
     *      /org/deepin/dde/Mime1
     *      org.deepin.dde.Mime1.ListApps "x-scheme-handler/https"
     *
     * */
    if (m_fIsLinglong) {
        deepinLauncherService = QString("org.deepin.dde.Launcher1");
        deepinLauncherPath = QString("/org/deepin/dde/Launcher1");
        deepinLauncherInterface = deepinLauncherService;

        deepinStartManagerService = QString("org.deepin.dde.StartManager1");
        deepinStartManagerPath = QString("/org/deepin/dde/StartManager1");;
        deepinStartManagerInterface = deepinStartManagerService;

        deepinMimeService = QString("org.deepin.dde.Mime1");
        deepinMimePath = QString("/org/deepin/dde/Mime1");;
        deepinMimeInterface = deepinMimeService;

        /*V23 new Application manager:
         *
         * qdbus --literal org.desktopspec.ApplicationManager1
         *      /org/desktopspec/ApplicationManager1
         *      org.desktopspec.ApplicationManager1.List
         **/

        deepinNewAppManagerService = QString("org.desktopspec.ApplicationManager1");
        deepinNewAppManagerPath = QString("/org/desktopspec/ApplicationManager1");;
        deepinNewAppManagerInterface = deepinNewAppManagerService;

        m_v23NewAM.reset(
            new QDBusInterface(
                deepinNewAppManagerService,
                deepinNewAppManagerPath,
                deepinNewAppManagerInterface,
                QDBusConnection::sessionBus(), this));
        m_v23NewAM->setTimeout(m_callTimeout);

        QString deepinNewMimeService = QString("org.desktopspec.ApplicationManager1");
        QString deepinNewMimePath = QString("/org/desktopspec/ApplicationManager1/MimeManager1");;
        QString deepinNewMimeInterface = QString("org.desktopspec.MimeManager1");

        m_v23NewMIME.reset(
            new QDBusInterface(
                deepinNewMimeService,
                deepinNewMimePath,
                deepinNewMimeInterface,
                QDBusConnection::sessionBus(), this));
        m_v23NewMIME->setTimeout(m_callTimeout);
    } else {
        deepinLauncherService = QString("com.deepin.dde.Launcher");
        deepinLauncherPath = QString("/com/deepin/dde/Launcher");
        deepinLauncherInterface = deepinLauncherService;

        deepinStartManagerService = QString("com.deepin.SessionManager");
        deepinStartManagerPath = QString("/com/deepin/StartManager");;
        deepinStartManagerInterface = QString("com.deepin.StartManager");

        deepinMimeService = QString("com.deepin.daemon.Mime");
        deepinMimePath = QString("/com/deepin/daemon/Mime");;
        deepinMimeInterface = deepinMimeService;
    }

    // Launcher interface
    m_oslauncher.reset(
        new QDBusInterface(
            deepinLauncherService,
            deepinLauncherPath,
            deepinLauncherInterface,
            QDBusConnection::sessionBus(), this));
    m_oslauncher->setTimeout(m_callTimeout);

    // Start manager interface
    m_osStartManager.reset(
        new QDBusInterface(
            deepinStartManagerService,
            deepinStartManagerPath,
            deepinStartManagerInterface,
            QDBusConnection::sessionBus(), this));
    m_osStartManager->setTimeout(m_callTimeout);

    // Mime interface
    m_osMime.reset(
        new QDBusInterface(
            deepinMimeService,
            deepinMimePath,
            deepinMimeInterface,
            QDBusConnection::sessionBus(), this));
    m_osMime->setTimeout(m_callTimeout);
}

DeepinLauncher::~DeepinLauncher()
{
}

int DeepinLauncher::showLauncher()
{
    /* V23:
     *  qdbus --literal org.deepin.dde.Launcher1
     *      /org/deepin/dde/Launcher1
     *      org.deepin.dde.Launcher1.Show
     *
     * Other:
     *  qdbus --literal com.deepin.dde.Launcher
     *      /com/deepin/dde/Launcher
     *      com.deepin.dde.Launcher.Show
     * */


    int errorCode = OSCallContext::NonError;

    if (m_oslauncher->isValid()) {
        QList<QVariant> argumentList;
        auto reply = m_oslauncher->asyncCallWithArgumentList(
                         QStringLiteral("Show"), argumentList);

        reply.waitForFinished();

        if (reply.isError()) {
            qCritical() << "Show call failed:"
                        << reply.error();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}

int DeepinLauncher::launchDesktop(const QString &pathApp)
{
    int errorCode = OSCallContext::NonError;

    QFileInfo appFile(pathApp);

    //Query the desktop file paths if only supply
    //xxx.desktop file
    if (!appFile.isAbsolute()) {
        QString deskPath = getAppDesktopFile(pathApp);
        appFile.setFile(deskPath);
    }

    if (!appFile.exists()) {
        qCritical() << "Target file does not exist: " << pathApp;
        errorCode = OSCallContext::AppNotFound;
        goto EXIT_ENTERY;
    }

    if (appFile.suffix() != "desktop") {
        qCritical() << "Target file's is not *.desktop: " << pathApp;
        errorCode = OSCallContext::AppNotFound;
        goto EXIT_ENTERY;
    }

    /*
     * V23:
     *  qdbus --literal org.deepin.dde.StartManager1 /
     *      org/deepin/dde/StartManager1
     *      org.deepin.dde.StartManager1.Launch "<abs path>/xxx.desktop"
     *
     * Other:
     *  qdbus --literal com.deepin.SessionManager
     *      /com/deepin/StartManager
     *      com.deepin.StartManager.Launch "<abs path>/xxx.desktop"
     * */
    //Try launch app use v23's new application manager
    if (m_fIsLinglong) {
        if (m_v23NewAM->isValid()) {
            errorCode = launchDesktopWithV23AM(appFile.filePath());

            //If launch ok with new AM, just exit.
            // else try to use old start manager.
            if (errorCode == OSCallContext::NonError) {
                goto EXIT_ENTERY;
            }
        } else {
            qWarning() << "V23 new application service isn't avaiable.";
        }
    }

    if (m_osStartManager->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(appFile.filePath());

        auto reply = m_osStartManager->asyncCallWithArgumentList(
                         QStringLiteral("Launch"), argumentList);

        reply.waitForFinished();

        if (reply.isError()) {
            qCritical() << "Launch call failed:" << appFile.filePath()
                        << " " << reply.error();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        } else {
            bool success = reply.reply().arguments().at(0).toBool();
            if (!success) {
                qCritical() << "Start the " << pathApp
                            << " failed, Status: "
                            << success;
                errorCode = OSCallContext::AppStartFailed;
            }
        }
    } else {
        qCritical() << "m_osStartManager is invalid!";
        errorCode = OSCallContext::NonService;
    }

EXIT_ENTERY:

    return errorCode;
}

QString DeepinLauncher::getAppDesktopFile(const QString &DesktopName)
{
    QString appDesktopFile;

    foreach (auto path, m_defaultDesktopPaths) {
        QFileInfo f(path + "/" + DesktopName);

        if (f.isFile() && f.exists()) {
            appDesktopFile = f.filePath();
            break;
        }
    }

    return appDesktopFile;
}

int DeepinLauncher::launchDefault(const QString &mineType)
{
    int errorCode = OSCallContext::NonError;

    if (m_fIsLinglong) {
        if (m_v23NewMIME->isValid()) {
            QList<QVariant> argumentList;
            argumentList << QVariant::fromValue<QString>(mineType);
            auto reply = m_v23NewMIME->asyncCallWithArgumentList(
                             QStringLiteral("queryDefaultApplication"), argumentList);

            reply.waitForFinished();

            if (reply.isError()) {
                qCritical() << "Failed (NewMIME) to get default apps:" << mineType
                            << " Error message: " << reply.error();
            } else {
                QString mimeType = reply.reply().arguments().value(0).toString();
                QString defaultAppPath = reply.reply().arguments().value(1).value<QDBusObjectPath>().path();

                errorCode = launchDesktopWithV23AM(defaultAppPath, true);

                //If launch ok with new AM, just exit.
                // else try to use old start manager.
                if (errorCode == OSCallContext::NonError) {
                    goto EXIT_ENTERY;
                }
            }
        } else {
            qWarning() << "V23 new application service isn't avaiable.";
        }
    }

    if (m_osMime->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(mineType);
        auto reply = m_osMime->asyncCallWithArgumentList(
                         QStringLiteral("GetDefaultApp"), argumentList);

        reply.waitForFinished();

        if (reply.isError()) {
            qCritical() << "Failed to get default apps:" << mineType
                        << " Error message: " << reply.error();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        } else {
            QString defaultAppInfo = reply.reply().arguments().at(0).toString();

            QJsonParseError jsonError;
            QJsonDocument jsonApp = QJsonDocument::fromJson(
                                        defaultAppInfo.toUtf8(), &jsonError);
            QJsonObject defaultAppObj = jsonApp.object();

            if (jsonApp.isNull() || !jsonApp.isObject()) {
                qCritical() << "Parse default app json error:" << defaultAppInfo;
                errorCode = OSCallContext::AppStartFailed;
            } else {
                QString strDefaultApp = defaultAppObj["Id"].toString();

                errorCode = launchDesktop(strDefaultApp);
            }
        }
    } else {
        qCritical() << "m_osMime is invalid!";
        errorCode = OSCallContext::NonService;
    }

EXIT_ENTERY:

    return errorCode;
}

int DeepinLauncher::listApps(const QString &mime)
{
    int count = 0;

    int errorCode = OSCallContext::NonError;

    if (m_osMime->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(mime);
        auto reply = m_osMime->asyncCallWithArgumentList(
                         QStringLiteral("ListApps"), argumentList);

        reply.waitForFinished();

        if (reply.isError()) {
            qCritical() << "Failed to get default apps list:" << mime
                        << " Error message: " << reply.error();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        } else {
            QString defaultAppInfos = reply.reply().arguments().at(0).toString();

            QJsonParseError jsonError;
            QJsonDocument jsonApp = QJsonDocument::fromJson(
                                        defaultAppInfos.toUtf8(), &jsonError);
            QJsonArray defaultAppArr = jsonApp.array();

            if (jsonApp.isNull() || !jsonApp.isArray()) {
                qCritical() << "Parse default app json error:" << defaultAppInfos;
                errorCode = OSCallContext::AppStartFailed;
            } else {
                count = defaultAppArr.size();
            }
        }
    } else {
        errorCode = OSCallContext::NonService;
    }

    Q_UNUSED(errorCode);

    return count;
}

QString DeepinLauncher::escapeToObjectPath(const QString &str)
{
    if (str.isEmpty()) {
        return "_";
    }

    auto ret = str;
    QRegularExpression re{R"([^a-zA-Z0-9])"};
    auto matcher = re.globalMatch(ret);
    while (matcher.hasNext()) {
        auto replaceList = matcher.next().capturedTexts();
        replaceList.removeDuplicates();
        for (const auto &c : replaceList) {
            auto hexStr = QString::number(static_cast<uint>(c.front().toLatin1()), 16);
            ret.replace(c, QString{R"(_%1)"}.arg(hexStr));
        }
    }
    return ret;
}

QString DeepinLauncher::unescapeFromObjectPath(const QString &str)
{
    auto ret = str;
    for (int i = 0; i < str.size(); ++i) {
        if (str[i] == '_' and i + 2 < str.size()) {
            auto hexStr = str.mid(i + 1, 2);
            ret.replace(QString{"_%1"}.arg(hexStr), QChar::fromLatin1(hexStr.toUInt(nullptr, 16)));
            i += 2;
        }
    }
    return ret;
}

QString DeepinLauncher::getAppIdFromAbsolutePath(const QString &path)
{
    static QString desktopSuffix{u8".desktop"};
    const auto &appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    if (!path.endsWith(desktopSuffix) or
    !std::any_of(appDirs.cbegin(), appDirs.constEnd(), [&path](const QString & dir) { return path.startsWith(dir); })) {
        return {};
    }

    auto tmp = path.chopped(desktopSuffix.size());
    auto components = tmp.split(QDir::separator(), Qt::SkipEmptyParts);
    auto location = std::find(components.cbegin(), components.cend(), "applications");
    if (location == components.cend()) {
        return {};
    }

    auto appId = QStringList{location + 1, components.cend()}.join('-');
    return appId;
}

int DeepinLauncher::launchDesktopWithV23AM(const QString &pathApp, bool isAppPath)
{
    int errorCode = OSCallContext::NonError;

    QScopedPointer<QDBusInterface> v23AppAM;

    QString appPath;

    if (!isAppPath) {
        QString appId = getAppIdFromAbsolutePath(pathApp);
        appPath = escapeToObjectPath(appId);
    }

    /* V23 new AM:
     *
     * qdbus --literal org.desktopspec.ApplicationManager1
     *      /org/desktopspec/ApplicationManager1/<appPath>
     *      org.desktopspec.ApplicationManager1.Application.Launch
     *       "", [], {}
     *
     **/
    QString v23AppManagerService = QString("org.desktopspec.ApplicationManager1");
    QString v23AppManagerPath = isAppPath ? pathApp :
                                QString("/org/desktopspec/ApplicationManager1/%1").arg(appPath);
    QString v23AppManagerInterface = QString("org.desktopspec.ApplicationManager1.Application");

    qInfo() << "launchDesktopWithV23AM->"
            << v23AppManagerService
            << v23AppManagerPath
            << v23AppManagerInterface;

    if (!v23AppManagerPath.isEmpty()) {
        v23AppAM.reset(new QDBusInterface(
                           v23AppManagerService,
                           v23AppManagerPath,
                           v23AppManagerInterface,
                           QDBusConnection::sessionBus(), this));
        if (v23AppAM->isValid()) {

            QList<QVariant> argumentList;
            argumentList << QVariant::fromValue<QString>(QString(""))
                         << QVariant::fromValue<QStringList>(QStringList(""))
                         << QVariant::fromValue<QVariantMap>(QVariantMap());

            auto reply = v23AppAM->asyncCallWithArgumentList(
                             QStringLiteral("Launch"), argumentList);

            reply.waitForFinished();

            if (reply.isError()) {
                qCritical() << "Failed to lauch app:" << pathApp
                            << " Error message: " << reply.error();
                errorCode = OSCallContext::AppStartFailed;
            }

        } else {
            errorCode = OSCallContext::NonService;
        }
    }

    return errorCode;
}
