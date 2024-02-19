#include "session.h"
#include "session_p.h"
#include "dbwrapper.h"
#include "serverwrapper.h"

Session::Session(const QString &appId, QObject *parent)
    : QObject(parent)
{
    m_private.reset(new SessionPrivate(this, appId));
}

Session::~Session()
{
    emit executionAborted();
}

void Session::cancelRequestTask(const QString &id)
{
    m_private->cancelRequestTask(id);
}

void Session::updateLLMAccount()
{
    m_private->checkUpdateLLmAccount(m_private->m_appServerProxy.id);
    emit llmAccountLstChanged(m_private->m_appServerProxy.id, queryLLMAccountList());
}

QString Session::appId() const
{
    return m_private->m_appId;
}

LLMServerProxy Session::llmServerProxy() const
{
    return m_private->m_appServerProxy;
}

QPair<AIServer::ErrorType, QStringList> Session::requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream)
{
    return m_private->requestChatFunctionText(llmId, conversation, stream, temperature);
}

bool Session::setCurrentLLMAccountId(const QString &id)
{
    LLMServerProxy aacount = DbWrapper::localDbWrapper().queryLlmByLlmid(id);
    if (!aacount.isValid())
        return false;

    m_private->m_appServerProxy = aacount;
    DbWrapper::localDbWrapper().updateAppCurllmId(m_private->m_appId, id);

    return true;
}

QString Session::currentLLMAccountId()
{
    return m_private->m_appServerProxy.id;
}

QString Session::queryLLMAccountList(const QList<LLMChatModel> &excludes)
{
    QJsonArray llmAccountArray;
    const QList<LLMServerProxy> &llmAccountLst = DbWrapper::localDbWrapper().queryLlmList();
    for (const LLMServerProxy &account : llmAccountLst) {
        if (excludes.contains(account.model))
            continue;

        QJsonObject accountObj;
        accountObj["id"] = account.id;
        accountObj["displayname"] = account.name;
        accountObj["model"] = account.model;
        accountObj["llmname"] = LLMServerProxy::llmName(account.model);
        accountObj["type"] = account.type;
        accountObj["icon"] = account.llmIcon(account.model);

        llmAccountArray << accountObj;
    }
    return QJsonDocument(llmAccountArray).toJson(QJsonDocument::Compact);
}

void Session::launchLLMUiPage(bool showAddllmPage)
{
    emit ServerWrapper::instance()->sigToLaunchMgmt(showAddllmPage);
}

void Session::launchAboutWindow()
{
    emit ServerWrapper::instance()->sigToLaunchAbout();
}
