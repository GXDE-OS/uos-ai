#include "freeaccountservice.h"
#include "model/modelinfo.h"
#include "model/modelvendor.h"
#include "tas/uosfreeaccounts.h"
#include "network/httpcodetranslation.h"
#include "global_key_define.h"

#include <DDialog>

#include <QtConcurrent>
#include <QThreadPool>
#include <QMetaObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logService)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

FreeAccountService::FreeAccountService(QObject *parent)
    : QObject(parent)
{
}

FreeAccountService *FreeAccountService::instance()
{
    static FreeAccountService ins;
    return &ins;
}

void FreeAccountService::incrementUsage(const QString &id)
{
    auto acc = ModelVendor::instance()->getModel(id);
    if (acc.constData() == nullptr)
        return;

    if (!ModelVendor::isUosProvider(acc))
        return;

    QtConcurrent::run([acc] {
        qCDebug(logService) << "Incrementing usage for account:" << acc->account.name;
        const QString apiKey = acc->account.auth.value(STR_KEY_API_KEY).toString();
        UosFreeAccounts::instance().increaseUse(apiKey, 0);
    });
}

QVariantHash FreeAccountService::validateUsage(const QString &id)
{
    QVariantHash error;
    auto acc = ModelVendor::instance()->getModel(id);
    if (acc.constData() == nullptr)
        return  error;

    if (!ModelVendor::isUosProvider(acc))
        return error;

    int available = 1;
    QString modelUrl;
    bool claimAgain = true;

    const QString apiKey = acc->account.auth.value(STR_KEY_API_KEY).toString();
    int code = UosFreeAccounts::instance().getDeterAccountLegal(apiKey, available, modelUrl, claimAgain);

    if (code != QNetworkReply::NoError) {
        error[STR_KEY_ERROR] = HttpError;
        error[STR_KEY_HTTP_ERROR] = code;
        error[STR_KEY_ERROR_MESSAGE] = "⚠️ " + HttpCodeTranslation::translation(
                    static_cast<QNetworkReply::NetworkError>(code), tr("Network error"));
        return error;
    }

    switch (available) {
    case 1:
        qCWarning(logService) << "Free account expired";
        error[STR_KEY_ERROR] = ModelExpired;
        error[STR_KEY_ERROR_MESSAGE] = tr("Your free account has expired, please configure your model account to continue using it.");
        break;
    case 2:
        qCWarning(logService) << "Free account usage limit reached";
        error[STR_KEY_ERROR] = ModelUsageLimitReached;
        error[STR_KEY_ERROR_MESSAGE] = tr("Your free account quota has been exhausted, please configure your model account to continue using it.");
        break;
    case 5:
        qCWarning(logService) << "Free account chat usage limit";
        error[STR_KEY_ERROR] = claimAgain ? ModelChatUsageLimitReached : ModelChatUsageClaimAgain;
        error[STR_KEY_ERROR_MESSAGE] = tr("Your free account quota has been exhausted for chat, please configure your model account to continue using it.");
        break;
    case 6:
        qCDebug(logService) << "Free account text2image usage limit";
        break;
    default:
        qCDebug(logService) << "Account status check passed";
        break;
    }

    return error;
}

void FreeAccountService::showClaimUsageDlg(const QString &modelId)
{
    DDialog dlg;
    dlg.setWindowFlags(dlg.windowFlags() | Qt::Tool | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
#ifdef COMPILE_ON_V25
    dlg.setMaximumWidth(380);
    dlg.setMinimumHeight(180);
#else
    dlg.setMinimumWidth(380);
#endif
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Not Now"), true, DDialog::ButtonNormal);
    dlg.addButton(tr("Claim Credits"), true, DDialog::ButtonRecommend);

    dlg.setTitle(QString(tr("Free Credits Delivered")));
    dlg.setMessage(QString(tr("You've used up the free generation credits for your trial account. We've given you an extra 200 free credits valid this month. Explore more features and unlock UOS AI's limitless capabilities!")));

    if (dlg.exec() == DDialog::Accepted)
        claimUsageRequest(modelId);
}

void FreeAccountService::claimUsageRequest(const QString &modelId)
{
    ModelAccountPtr modelAccount = ModelVendor::instance()->getModel(modelId);
    if (!modelAccount.data()) {
        qCWarning(logService) << "Model account not found for modelId:" << modelId;
        return;
    }

    if (!ModelVendor::isUosProvider(modelAccount)) {
        qCWarning(logService) << "Model account is not FREE for modelId:" << modelId;
        return;
    }

    QtConcurrent::run(QThreadPool::globalInstance(), [this, modelAccount]() {
        QString apiKey = modelAccount->account.auth.value(STR_KEY_API_KEY).toString();
        int result = 0;
        QString resultMessage = "";
        int errorCode = UosFreeAccounts::instance().claimAccountUsage(apiKey, result, resultMessage);
        if (errorCode == QNetworkReply::NoError) {
            bool claimed = false;
            switch(result) {
            case 200:
                claimed = true;
                resultMessage = tr("Successfully Claimed");
                break;
            case 5001:
                resultMessage = tr("Account not found");
                break;
            case 5002:
                resultMessage = tr("Only support trial account");
                break;
            case 5003:
                resultMessage = tr("Successfully Claimed");
                break;
            case 5004:
                claimed = true;
                resultMessage = tr("You have already participated in the event and cannot claim the reward again.");
                break;
            case 500:
                resultMessage = tr("Server system error");
                break;
            default:
                resultMessage = tr("Failed to Claim. Please Try Again.");
                break;
            }

            QMetaObject::invokeMethod(this, "claimUsageComplete", Qt::QueuedConnection,
                                      Q_ARG(bool, claimed),
                                      Q_ARG(QString, resultMessage)
                                      );
        } else {
            qCWarning(logService) << "Network error checking account status:" << errorCode;
            QMetaObject::invokeMethod(this, "claimUsageComplete", Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(QString, tr("Failed to Claim. Please Try Again."))
                                      );
        }
    });
}
