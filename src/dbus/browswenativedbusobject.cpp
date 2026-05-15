#include "browswenativedbusobject.h"
#include "model/modelvendor.h"
#include "global_key_define.h"
#include "tas/uosfreeaccounts.h"
#include "services/accountservice/freeaccountservice.h"
#include "util.h"

#include <QLoggingCategory>
#include <QtDBus>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

using namespace uos_ai;

BrowsweNativeDbusObject::BrowsweNativeDbusObject(const QString &appId)
    : AppDbusObject(appId)
{

}

BrowsweNativeDbusObject::~BrowsweNativeDbusObject()
{

}

QString BrowsweNativeDbusObject::queryCurrentDetailLLMAccount()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(message().service());
    if (app != Util::queryProcessName(pid)) {
        qCWarning(logDBus) << "Access denied for appId:" << Util::queryProcessName(pid);
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
        return QString();
    }

    auto model = ModelVendor::instance()->getModel(currentLLMAccountId());
    QString result;
    if (model.data()) {
        QJsonObject accountObj;
        accountObj.insert("id", model->id);
        accountObj["displayname"] = model->model.name;
        accountObj["model"] = model->model.modelId;
        accountObj["type"] = ModelVendor::isUosProvider(model) ? 1 : 0;
        accountObj["icon"] = model->model.icon;

        result = QJsonDocument(accountObj).toJson(QJsonDocument::Compact);
    }

    qCDebug(logDBus) << "Querying current detail LLM account:" << result;
    return result;
}

int BrowsweNativeDbusObject::isFreeAccountValid(const QString &llmId)
{
    qCDebug(logDBus) << "Checking free account validity for llmId:" << llmId;
    auto model = ModelVendor::instance()->getModel(llmId);
    if (!model.data()) {
        qCWarning(logDBus) << "Invalid account for llmId:" << llmId;
        return 0;
    }

    // 用户自己的账号不限制失效和次数，理论上来说应用会判断账号类型决定是否调用此接口
    if (!ModelVendor::isUosProvider(model)) {
        qCDebug(logDBus) << "User account, no restrictions applied";
        return 0;
    }

    QString apiKey = model->account.auth.value(STR_KEY_API_KEY).toString();
    int available;
    QString modelUrl;
    bool claimAgain;
    if (QNetworkReply::NoError != UosFreeAccounts::instance().getDeterAccountLegal(apiKey, available, modelUrl, claimAgain)) {
        qCWarning(logDBus) << "Failed to determine account legality";
        return 9999;
    }

    qCDebug(logDBus) << "Account validity check result:" << available;
    return available;
}

void BrowsweNativeDbusObject::incrementUsageCount(const QString &llmId)
{
    FreeAccountService::instance()->incrementUsage(llmId);
}
