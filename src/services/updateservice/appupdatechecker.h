#ifndef APPUPDATECHECKER_H
#define APPUPDATECHECKER_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QDateTime>
#include <QString>

namespace uos_ai {

class StoreAPI;

struct AppUpdateInfo
{
    QString packageId;
    QString updateDescription;
    QString currentVersion;
    QString availableVersion;
};

class AppUpdateChecker : public QObject
{
    Q_OBJECT
public:
    static AppUpdateChecker *instance();

    void checkForUpdate();

    Q_INVOKABLE void markUpdateReminderConsumed(const QString &availableVersion);

signals:
    void updateAvailable(const QJsonObject &info);
    void checkFinished();

private slots:
    void queryStoreAppBriefInfo();
    void handleStoreAppBriefInfoFinished(bool success, const QString &error, const QJsonObject &appInfo);

private:
    explicit AppUpdateChecker(QObject *parent = nullptr);

    bool canStartUpdateCheck() const;
    bool shouldShowReminder(const AppUpdateInfo &info) const;
    void notifyUpdateAvailable(const AppUpdateInfo &info);
    QString lastConsumedVersion() const;
    void saveLastConsumedVersion(const QString &normalizedVersion);
    QJsonObject updateInfoToJson(const AppUpdateInfo &info) const;
    void finishCheck();

    StoreAPI *m_storeApi = nullptr;
    QDateTime m_lastUpdateCheckTime;
    bool m_updateCheckInProgress = false;
};

}

#endif // APPUPDATECHECKER_H
