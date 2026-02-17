#include "browswenativedbusobject.h"
#include "session.h"
#include "llmutils.h"
#include "dbwrapper.h"
#include "uosfreeaccounts.h"

#include <QLoggingCategory>
#include <QtDBus>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

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
    if (m_chatSession->appId() != LLMUtils::queryAppId(pid)) {
        qCWarning(logDBus) << "Access denied for appId:" << LLMUtils::queryAppId(pid);
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
        return QString();
    }

    QString result = QJsonDocument(m_chatSession->llmServerProxy().toJson()).toJson(QJsonDocument::Compact);
    qCDebug(logDBus) << "Querying current detail LLM account:" << result;
    return result;
}

int BrowsweNativeDbusObject::isFreeAccountValid(const QString &llmId)
{
    qCDebug(logDBus) << "Checking free account validity for llmId:" << llmId;
    LLMServerProxy aacount = DbWrapper::localDbWrapper().queryLlmByLlmid(llmId);
    if (!aacount.isValid()) {
        qCWarning(logDBus) << "Invalid account for llmId:" << llmId;
        return 0;
    }

    // 用户自己的账号不限制失效和次数，理论上来说应用会判断账号类型决定是否调用此接口
    if (aacount.type == ModelType::USER) {
        qCDebug(logDBus) << "User account, no restrictions applied";
        return 0;
    }

    int available;
    QString modelUrl;
    bool claimAgain;
    if (QNetworkReply::NoError != UosFreeAccounts::instance().getDeterAccountLegal(aacount.account.apiKey, available, modelUrl, claimAgain)) {
        qCWarning(logDBus) << "Failed to determine account legality";
        return 9999;
    }

    qCDebug(logDBus) << "Account validity check result:" << available;
    return available;
}

void BrowsweNativeDbusObject::incrementUsageCount(const QString &llmId)
{
    qCDebug(logDBus) << "Incrementing usage count for llmId:" << llmId;
    LLMServerProxy aacount = DbWrapper::localDbWrapper().queryLlmByLlmid(llmId);
    if (!aacount.isValid()) {
        qCWarning(logDBus) << "Invalid account for llmId:" << llmId;
        return;
    }

    // 用户自己的账号不限制失效和次数，理论上来说应用会判断账号类型决定是否调用此接口
    if (aacount.type == ModelType::USER) {
        qCDebug(logDBus) << "User account, no usage count increment needed";
        return;
    }

    UosFreeAccounts::instance().increaseUse(aacount.account.apiKey, 0);
    qCDebug(logDBus) << "Usage count incremented successfully";
}
