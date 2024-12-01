#include "session.h"
#include "session_p.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "utils/util.h"

UOSAI_USE_NAMESPACE

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
    auto llm  = m_private->m_llmVendor.queryValidServerAccount(m_private->m_appServerProxy.id);
    if (!llm.isValid()) {
        auto supLLM = m_private->m_llmVendor.queryServerAccountByRole(m_private->m_assistantProxy);
        if (!supLLM.isEmpty()) {
            llm = supLLM.first();
        } else {
            qWarning() << "no llm for" << m_private->m_assistantProxy.id << m_private->m_assistantProxy.displayName;
        }
    }

    m_private->m_appServerProxy = llm;
    emit llmAccountLstChanged(m_private->m_appServerProxy.id, queryLLMAccountListWithRole());
}

QString Session::appId() const
{
    return m_private->m_appId;
}

LLMServerProxy Session::llmServerProxy() const
{
    return m_private->m_appServerProxy;
}

QPair<AIServer::ErrorType, QStringList> Session::requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream, bool isFAQGeneration)
{
    LLMServerProxy tmpLLMAccount = m_private->queryValidServerAccount(llmId);
    if (tmpLLMAccount.model == PLUGIN_MODEL)
        return m_private->requestPlugin(tmpLLMAccount, conversation, stream, temperature);

    return m_private->requestChatFunctionText(tmpLLMAccount, conversation, stream, temperature, isFAQGeneration);
}

bool Session::setCurrentLLMAccountId(const QString &id)
{
    if (id.isEmpty())
        return false;

    {
        QList<LLMServerProxy> llms = m_private->m_llmVendor.queryServerAccountByRole(m_private->m_assistantProxy);
        if (!llms.isEmpty()) {
            const auto it = std::find_if(llms.begin(), llms.end(), [id](const LLMServerProxy & account) {
                return account.id == id;
            });

            if (it == llms.end()) {
                qWarning() << "llm" << id << "is not support for " << m_private->m_assistantProxy.id << m_private->m_assistantProxy.displayName;
                return false;
            }
        }
    }

    LLMServerProxy account = m_private->m_llmVendor.setCurrentLLMAccountId(id, m_private->m_appId);
    if (!account.isValid())
        return false;

    m_private->m_appServerProxy = account;

    //update role-model table
    DbWrapper::localDbWrapper().updateAssistantLlm(m_private->m_assistantProxy.id, account.id);
    return true;
}

QString Session::currentLLMAccountId()
{
    return m_private->m_appServerProxy.id;
}

LLMChatModel Session::currentLLMModel()
{
    return m_private->m_appServerProxy.model;
}

ModelType Session::currentModelType()
{
    return m_private->m_appServerProxy.type;
}

bool Session::setCurrentAssistantId(const QString &id)
{
    AssistantProxy assistant = DbWrapper::localDbWrapper().queryAssistantByid(id);
    if (!assistant.isValid()) {
        assistant = m_private->m_llmVendor.queryAssistantById(id);
        if (!assistant.isValid())
            return false;
    }

    m_private->m_assistantProxy = assistant;
    DbWrapper::localDbWrapper().updateAppCurAssistantId(m_private->m_appId, id);

    QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(m_private->m_assistantProxy.id);
    if (!setCurrentLLMAccountId(llmId)) {
        auto supLLM = m_private->m_llmVendor.queryServerAccountByRole(m_private->m_assistantProxy);
        if (!supLLM.isEmpty()) {
            setCurrentLLMAccountId(supLLM.first().id);
        } else {
            qWarning() << "no llm for" << m_private->m_assistantProxy.id << m_private->m_assistantProxy.displayName;
            m_private->m_appServerProxy = LLMServerProxy();
        }
    }

    emit llmAccountLstChanged(m_private->m_appServerProxy.id, queryLLMAccountListWithRole());
    return true;
}

QString Session::currentAssistantId()
{
    return m_private->m_assistantProxy.id;
}

QString Session::currentAssistantDisplayName(){
    switch (m_private->m_assistantProxy.type) {
    case UOS_AI:
        return tr("UOS AI");
    case UOS_SYSTEM_ASSISTANT:
        return tr("UOS System Assistant");
    case DEEPIN_SYSTEM_ASSISTANT:
        return tr("Deepin System Assistant");
    case PERSONAL_KNOWLEDGE_ASSISTANT:
        return tr("Personal Knowledge Assistant");
    default:
        return m_private->m_assistantProxy.displayName;
    }
}

AssistantType Session::currentAssistantType()
{
    return m_private->m_assistantProxy.type;
}

QString Session::queryLLMAccountList(const QList<LLMChatModel> &excludes)
{
    return m_private->m_llmVendor.queryLLMAccountList(excludes);
}

QString Session::queryLLMAccountListWithRole(const QList<LLMChatModel> &excludes)
{
    return m_private->m_llmVendor.queryLLMAccountListByRole(m_private->m_assistantProxy, excludes);
}

QString Session::queryAssistantList()
{
    QJsonArray assistantArray;
    // 插件助手角色
    QList<AssistantProxy> assistantLst = m_private->m_llmVendor.queryAssistantList();
    // 固定角色
    assistantLst << DbWrapper::localDbWrapper().queryAssistantList();

    for (const AssistantProxy &assistant : assistantLst) {
        QJsonObject assistantObj;
        assistantObj["id"] = assistant.id;
        assistantObj["type"] = assistant.type;
        assistantObj["displayname"] = assistant.assistantDisplayName(assistant.type);
        assistantObj["description"] = assistant.assistantDescrption(assistant.type);
        assistantObj["icon"] = assistant.assistantIcon(assistant.type);
        assistantObj["iconPrefix"] = assistant.iconPrefix;

        assistantArray << assistantObj;
    }
    return QJsonDocument(assistantArray).toJson(QJsonDocument::Compact);
}

QString Session::queryAssistantIdByType(AssistantType type)
{
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assistant : assistantLst) {
        if (assistant.type == type)
            return assistant.id;
    }

    return "";
}

void Session::launchLLMUiPage(bool showAddllmPage, bool onlyUseAgreement)
{
    emit ServerWrapper::instance()->sigToLaunchMgmt(showAddllmPage, onlyUseAgreement);
}

void Session::launchAboutWindow()
{
    emit ServerWrapper::instance()->sigToLaunchAbout();
}

QVariant Session::getFAQ()
{
    return m_private->getFAQ();
}
