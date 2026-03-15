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
#include <QDBusPendingCall>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDir>
#include <DStandardPaths>
#include <QDBusObjectPath>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

UOSAI_USE_NAMESPACE

DeepinLauncher::DeepinLauncher(const QStringList &deskPaths, QObject *parent) : QObject(parent)
    , m_defaultDesktopPaths(deskPaths)
{
    // Launcher interface
    m_oslauncher.reset(
        new QDBusInterface(
            deepinLancherService,
            deepinLancherPath,
            deepinLancherInterface,
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
            qCWarning(logOsControl) << "Show launcher failed - error:" << reply.error().message();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        }
    } else {
        qCWarning(logOsControl) << "Launcher interface is invalid";
        errorCode = OSCallContext::NonService;
    }

    return errorCode;
}

int DeepinLauncher::launchDesktop(const QString &pathApp)
{
    qCDebug(logOsControl) << "Launching desktop application:" << pathApp;
    int errorCode = OSCallContext::NonError;
    QFileInfo appFile(pathApp);
    //Query the desktop file paths if only supply
    //xxx.desktop file
    if (!appFile.isAbsolute()) {
        QString deskPath = getAppDesktopFile(pathApp);
        appFile.setFile(deskPath);
        qCDebug(logOsControl) << "Resolved desktop file path:" << deskPath;
    }

    if (!appFile.exists()) {
        qCWarning(logOsControl) << "Target file does not exist:" << pathApp;
        errorCode = OSCallContext::AppNotFound;
        return errorCode;
    }

    if (appFile.suffix() != "desktop") {
        qCWarning(logOsControl) << "Target file is not a desktop file:" << pathApp;
        errorCode = OSCallContext::AppNotFound;
        return errorCode;
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
#ifdef COMPILE_ON_V23
    QScopedPointer<QDBusInterface> appAM;
    QString appPath;
    QString appId = getAppIdFromAbsolutePath(pathApp);
    appPath = escapeToObjectPath(appId);
    /* V23 new AM:
     *
     * qdbus --literal org.desktopspec.ApplicationManager1
     *      /org/desktopspec/ApplicationManager1/<appPath>
     *      org.desktopspec.ApplicationManager1.Application.Launch
     *       "", [], {}
     *
     **/
    appAM.reset(new QDBusInterface(
                       deepinStartManagerService,
                       QString(deepinStartManagerPath) + QString("/%1").arg(appPath),
                       deepinStartManagerInterface,
                       QDBusConnection::sessionBus(), this));
    if (appAM->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(QString(""))
                     << QVariant::fromValue<QStringList>(QStringList(""))
                     << QVariant::fromValue<QVariantMap>(QVariantMap());

        auto reply = appAM->asyncCallWithArgumentList(
                         QStringLiteral("Launch"), argumentList);
        reply.waitForFinished();

        if (reply.isError()) {
            qCWarning(logOsControl) << "Failed to launch app with V23 manager - path:" << pathApp
                                  << "error:" << reply.error().message();
            errorCode = OSCallContext::AppStartFailed;
        }

    } else {
        qCWarning(logOsControl) << "V23 application manager interface is invalid";
        errorCode = OSCallContext::NonService;
    }
    //If launch ok with new AM, just exit.
    // else try to use old start manager.
    if (errorCode == OSCallContext::NonError) {
        return errorCode;
    }
#endif

    if (m_osStartManager->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(appFile.filePath());

        auto reply = m_osStartManager->asyncCallWithArgumentList(
                         QStringLiteral("Launch"), argumentList);

        reply.waitForFinished();

        if (reply.isError()) {
            qCWarning(logOsControl) << "Launch failed with start manager - path:" << appFile.filePath()
                                  << "error:" << reply.error().message();

            if (QDBusError::UnknownMethod == reply.error().type()) {
                errorCode = OSCallContext::NotImpl;
            } else {
                errorCode = OSCallContext::NonService;
            }
        } else {
            bool success = reply.reply().arguments().at(0).toBool();
            if (!success) {
                qCWarning(logOsControl) << "Application start failed - path:" << pathApp;
                errorCode = OSCallContext::AppStartFailed;
            }
        }
    } else {
        qCWarning(logOsControl) << "Start manager interface is invalid";
        errorCode = OSCallContext::NonService;
    }
    return errorCode;
}

QString DeepinLauncher::getAppDesktopFile(const QString &DesktopName)
{
    qCDebug(logOsControl) << "Getting desktop file for:" << DesktopName;
    QString appDesktopFile;

    foreach (auto path, m_defaultDesktopPaths) {
        QFileInfo f(path + "/" + DesktopName);

        if (f.isFile() && f.exists()) {
            appDesktopFile = f.filePath();
            qCDebug(logOsControl) << "Found desktop file:" << appDesktopFile;
            break;
        }
    }

    return appDesktopFile;
}

int DeepinLauncher::launchDefault(const QString &mineType)
{
    qCDebug(logOsControl) << "Launching default application for mime type:" << mineType;
    int errorCode = OSCallContext::NonError;
    if (m_osMime->isValid()) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue<QString>(mineType);
        auto reply = m_osMime->asyncCallWithArgumentList(
                         QStringLiteral("GetDefaultApp"), argumentList);
        reply.waitForFinished();
        if (reply.isError()) {
            qCWarning(logOsControl) << "Failed to get default app - type:" << mineType
                                  << "error:" << reply.error().message();

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
                qCWarning(logOsControl) << "Failed to parse default app JSON:" << defaultAppInfo;
                errorCode = OSCallContext::AppStartFailed;
            } else {
                QString strDefaultApp = defaultAppObj["Id"].toString();

                errorCode = launchDesktop(strDefaultApp);
            }
        }
    } else {
        qCWarning(logOsControl) << "Mime interface is invalid";
        errorCode = OSCallContext::NonService;
    }

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
    qCDebug(logOsControl) << "Escaped object path:" << ret;
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
    qCDebug(logOsControl) << "Unescaped object path:" << ret;
    return ret;
}

QString DeepinLauncher::getAppIdFromAbsolutePath(const QString &path)
{
    qCDebug(logOsControl) << "Getting app ID from path:" << path;
    static QString desktopSuffix{u8".desktop"};
    const auto &appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    if (!path.endsWith(desktopSuffix) or
    !std::any_of(appDirs.cbegin(), appDirs.constEnd(), [&path](const QString & dir) { return path.startsWith(dir); })) {
        qCDebug(logOsControl) << "Invalid desktop file path";
        return {};
    }

    auto tmp = path.chopped(desktopSuffix.size());
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
    auto components = tmp.split(QDir::separator(), QString::SkipEmptyParts);
#else
    auto components = tmp.split(QDir::separator(), Qt::SkipEmptyParts);
#endif
    auto location = std::find(components.cbegin(), components.cend(), "applications");
    if (location == components.cend()) {
        qCDebug(logOsControl) << "No applications directory found in path";
        return {};
    }

    QStringList tmpLocation;
    for (auto it = location + 1; it != components.cend(); ++it)
        tmpLocation << *it;

    auto appId = tmpLocation.join('-');
    qCDebug(logOsControl) << "Extracted app ID:" << appId;
    return appId;
}
