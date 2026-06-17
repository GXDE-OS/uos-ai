#include "appupdatechecker.h"
#include "oscontrol/storeapi.h"
#include "updateversion.h"
#include "database/appdatabase.h"

#include <QLoggingCategory>
#include <QApplication>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

inline constexpr const char kUosAiPackageId[] = "uos-ai";
inline constexpr qint64 kUpdateCheckIntervalMs = 24LL * 60 * 60 * 1000;

AppUpdateChecker *AppUpdateChecker::instance()
{
    static AppUpdateChecker checker;
    return &checker;
}

AppUpdateChecker::AppUpdateChecker(QObject *parent)
    : QObject(parent)
    , m_storeApi(new StoreAPI(this))
{
    connect(m_storeApi, &StoreAPI::appBriefInfoFinished,
            this, &AppUpdateChecker::handleStoreAppBriefInfoFinished);
}

void AppUpdateChecker::checkForUpdate()
{
    if (!canStartUpdateCheck()) {
        qCInfo(logService) << "Skip UOS AI update check. Last check time:"
                           << m_lastUpdateCheckTime.toString(Qt::ISODate)
                           << "in progress:" << m_updateCheckInProgress;
        return;
    }

    m_updateCheckInProgress = true;
    m_lastUpdateCheckTime = QDateTime::currentDateTimeUtc();
    queryStoreAppBriefInfo();
}

void AppUpdateChecker::markUpdateReminderConsumed(const QString &availableVersion)
{
    const QString normalizedVersion = UpdateVersion::normalize(availableVersion);
    if (normalizedVersion.isEmpty()) {
        qCWarning(logService) << "Ignoring invalid consumed app update version:" << availableVersion;
        return;
    }

    // 只有消费者确认已经展示提醒后才写库，避免信号早于前端创建时误标记已消费。
    saveLastConsumedVersion(normalizedVersion);
}

void AppUpdateChecker::queryStoreAppBriefInfo()
{
    if (!m_storeApi) {
        qCWarning(logService) << "Store API is unavailable";
        finishCheck();
        return;
    }

    m_storeApi->getAppBriefInfo(QString::fromLatin1(kUosAiPackageId));
}

bool AppUpdateChecker::canStartUpdateCheck() const
{
    if (m_updateCheckInProgress) {
        return false;
    }

    if (!m_lastUpdateCheckTime.isValid()) {
        return true;
    }

    return m_lastUpdateCheckTime.msecsTo(QDateTime::currentDateTimeUtc()) >= kUpdateCheckIntervalMs;
}

bool AppUpdateChecker::shouldShowReminder(const AppUpdateInfo &info) const
{
    if (info.packageId != QLatin1String(kUosAiPackageId)) {
        return false;
    }

    if (info.availableVersion.isEmpty() || info.currentVersion.isEmpty()) {
        return false;
    }

    if (UpdateVersion::compare(info.availableVersion, info.currentVersion) <= 0) {
        return false;
    }

    return info.availableVersion != lastConsumedVersion();
}

void AppUpdateChecker::notifyUpdateAvailable(const AppUpdateInfo &info)
{
    if (!shouldShowReminder(info)) {
        return;
    }

    emit updateAvailable(updateInfoToJson(info));
}

QString AppUpdateChecker::lastConsumedVersion() const
{
    return AppDatabase::instance()->getConfig(CONFIG_APP_UPDATE_LAST_CONSUMED_VERSION).trimmed();
}

void AppUpdateChecker::saveLastConsumedVersion(const QString &normalizedVersion)
{
    AppDatabase::instance()->saveConfig(CONFIG_APP_UPDATE_LAST_CONSUMED_VERSION, normalizedVersion);
}

QJsonObject AppUpdateChecker::updateInfoToJson(const AppUpdateInfo &info) const
{
    return {
        {QStringLiteral("packageId"), info.packageId},
        {QStringLiteral("updateDescription"), info.updateDescription},
        {QStringLiteral("currentVersion"), info.currentVersion},
        {QStringLiteral("availableVersion"), info.availableVersion},
    };
}

void AppUpdateChecker::finishCheck()
{
    m_updateCheckInProgress = false;
    emit checkFinished();
}

void AppUpdateChecker::handleStoreAppBriefInfoFinished(bool success, const QString &error, const QJsonObject &appInfo)
{
    if (!success) {
        qCWarning(logService) << "Store app brief info request failed:" << error;
        finishCheck();
        return;
    }

    const QString availableVersion = appInfo.value(QStringLiteral("package_version")).toString().trimmed();
    if (availableVersion.isEmpty()) {
        qCWarning(logService) << "Store app brief info did not include package_version";
        finishCheck();
        return;
    }

    AppUpdateInfo info;
    info.packageId = appInfo.value(QStringLiteral("package_name")).toString().trimmed();
    if (info.packageId.isEmpty()) {
        info.packageId = QString::fromLatin1(kUosAiPackageId);
    }

    info.updateDescription = appInfo.value(QStringLiteral("update_desc")).toString().trimmed();
    if (info.updateDescription.isEmpty()) {
        info.updateDescription = tr("Unknown");
    }

    const QString currentVersion = qApp->applicationVersion().trimmed();
    info.currentVersion = UpdateVersion::normalize(currentVersion);
    info.availableVersion = UpdateVersion::normalize(availableVersion);

    if (info.currentVersion.isEmpty() || info.availableVersion.isEmpty()) {
        qCWarning(logService) << "Invalid app version data from store:"
                              << "current:" << currentVersion
                              << "available:" << availableVersion;
        finishCheck();
        return;
    }

    if (shouldShowReminder(info)) {
        notifyUpdateAvailable(info);
    } else {
        qCInfo(logService) << "No pending UOS AI update reminder for version:"
                           << info.availableVersion
                           << "current:" << info.currentVersion
                           << "last consumed:" << lastConsumedVersion();
    }

    finishCheck();
}
