#include "browswenativedbusobject.h"
#include "session.h"
#include "llmutils.h"
#include "dbwrapper.h"
#include "uosfreeaccounts.h"

#include <QtDBus>

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
        qWarning() << LLMUtils::queryAppId(pid) << " no permission to access!";
        sendErrorReply(QDBusError::AccessDenied, "no permission to access!");
        return QString();
    }

    return QJsonDocument(m_chatSession->llmServerProxy().toJson()).toJson(QJsonDocument::Compact);
}

int BrowsweNativeDbusObject::isFreeAccountValid(const QString &llmId)
{
    LLMServerProxy aacount = DbWrapper::localDbWrapper().queryLlmByLlmid(llmId);
    if (!aacount.isValid())
        return 0;

    // 用户自己的账号不限制失效和次数，理论上来说应用会判断账号类型决定是否调用此接口
    if (aacount.type == ModelType::USER) {
        return 0;
    }

    int available;
    if (QNetworkReply::NoError != UosFreeAccounts::instance().getDeterAccountLegal(aacount.account.apiKey, available)) {
        return 9999;
    }

    return available;
}

void BrowsweNativeDbusObject::incrementUsageCount(const QString &llmId)
{
    LLMServerProxy aacount = DbWrapper::localDbWrapper().queryLlmByLlmid(llmId);
    if (!aacount.isValid())
        return;

    // 用户自己的账号不限制失效和次数，理论上来说应用会判断账号类型决定是否调用此接口
    if (aacount.type == ModelType::USER) {
        return;
    }
    UosFreeAccounts::instance().increaseUse(aacount.account.apiKey);
}
