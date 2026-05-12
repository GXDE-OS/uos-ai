#ifndef FREEACCOUNTSERVICE_H
#define FREEACCOUNTSERVICE_H

#include <QObject>
#include <QVariantHash>
#include <QSharedPointer>

namespace uos_ai {

class FreeAccountService : public QObject
{
    Q_OBJECT
public:
    static FreeAccountService *instance();

    void incrementUsage(const QString &id);
    QVariantHash validateUsage(const QString &id);
    void showClaimUsageDlg(const QString &modelId);

signals:
    void claimUsageComplete(bool ok, const QString &msg);

public slots:
    void claimUsageRequest(const QString &modelId);

private:
    explicit FreeAccountService(QObject *parent = nullptr);
};

}

#endif // FREEACCOUNTSERVICE_H
