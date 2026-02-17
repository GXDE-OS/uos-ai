#include "session.h"
#include "session_p.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "utils/util.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logWrapper)

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
    auto llm  = LLMVendor()->queryValidServerAccount(m_private->m_appServerProxy.id);
    if (!llm.isValid()) {
        auto supLLM = LLMVendor()->queryServerAccountByRole(m_private->m_assistantProxy);
        if (!supLLM.isEmpty()) {
            llm = supLLM.first();
        } else {
            qCWarning(logWrapper) << "No LLM found for assistant:" << m_private->m_assistantProxy.id 
                                << "name:" << m_private->m_assistantProxy.displayName;
        }
    }

    if (m_private->m_assistantProxy.type == AssistantType::UOS_AI) {
        m_uosaiTopAccountId = llm.id;
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

QPair<AIServer::ErrorType, QStringList> Session::requestChatText(const QString &llmId, const QString &conversation, const QVariantHash &params)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return {AIServer::AuthenticationRequiredError, QStringList()};
    }

    LLMServerProxy tmpLLMAccount = m_private->queryValidServerAccount(llmId);
    if (tmpLLMAccount.model == PLUGIN_MODEL) {
        qCDebug(logWrapper) << "Using plugin model for request";
        return m_private->requestPlugin(tmpLLMAccount, conversation, params);
    }

    return m_private->requestChatFunctionText(tmpLLMAccount, conversation, params);
}

QPair<AIServer::ErrorType, QStringList> Session::requestFunction(const QString &llmId, const QString &conversation, const QJsonArray &funcs, const QVariantHash &params)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return {AIServer::AuthenticationRequiredError, QStringList()};
    }

    LLMServerProxy tmpLLMAccount = m_private->queryValidServerAccount(llmId);
//    if (tmpLLMAccount.model == PLUGIN_MODEL)
//        return m_private->requestPlugin(tmpLLMAccount, conversation, stream, temperature);

    return m_private->requestInstFunction(tmpLLMAccount, conversation, funcs, params);
}

QPair<AIServer::ErrorType, QStringList> Session::requestMcpAgent(const QString &llmId, const QString &conversation, const QVariantHash &params)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return {AIServer::AuthenticationRequiredError, QStringList()};
    }

    LLMServerProxy tmpLLMAccount = m_private->queryValidServerAccount(llmId);
    if (tmpLLMAccount.model == PLUGIN_MODEL) {
        qCCritical(logWrapper) << "Agent don't support plugin model";
        return {AIServer::ContentAccessDenied, QStringList()};
    }

    return m_private->requestAgent(tmpLLMAccount, conversation, params);
}

QPair<AIServer::ErrorType, QStringList> Session::requestRag(const QString &llmId, const QString &conversation, const QVariantHash &params)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return {AIServer::AuthenticationRequiredError, QStringList()};
    }

    LLMServerProxy tmpLLMAccount = m_private->queryValidServerAccount(llmId);
    if (tmpLLMAccount.model == PLUGIN_MODEL) {
        qCCritical(logWrapper) << "RAG don't support plugin model";
        return {AIServer::ContentAccessDenied, QStringList()};
    }

    return m_private->requestRag(tmpLLMAccount, conversation, params);
}

QString Session::chatRequest(const QString &llmId, const QString &ctx, const QVariantHash &params)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return "";
    }

    LLMServerProxy llmAccount = m_private->queryValidServerAccount(llmId);
    return m_private->chatRequest(llmAccount, ctx, params);
}

QString Session::searchRequest(const QString &llmId, const QString &ctx)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return "";
    }

    LLMServerProxy llmAccount = m_private->queryValidServerAccount(llmId);
    return m_private->searchRequest(llmAccount, ctx);
}

QString Session::requestGenImage(const QString &llmId, const QString &imageDesc)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
        return "";
    }

    LLMServerProxy llmAccount = m_private->queryValidServerAccount(llmId);
    return m_private->genImageRequest(llmAccount, imageDesc);
}

void Session::claimUsageRequest(const QString &llmId)
{
    // 兜底检查，如果有办法不同意协议走到这里，直接abort
    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen()) {
        qCCritical(logWrapper) << "AI Copilot agreement not accepted, aborting request";
        abort();
    }

    LLMServerProxy llmAccount = m_private->queryValidServerAccount(llmId);
    m_private->claimUsageRequest(llmAccount);
}

bool Session::setCurrentLLMAccountId(const QString &id)
{
    qCDebug(logWrapper) << "Setting current LLM account ID:" << id;
    
    if (id.isEmpty()) {
        qCWarning(logWrapper) << "Empty LLM account ID provided";
        return false;
    }

    {
        QList<LLMServerProxy> llms = LLMVendor()->queryServerAccountByRole(m_private->m_assistantProxy);
        bool found = false;
        if (!llms.isEmpty()) {
            const auto it = std::find_if(llms.begin(), llms.end(), [id](const LLMServerProxy & account) {
                return account.id == id;
            });

            found = it != llms.end();
        }

        if (!found) {
            qCWarning(logWrapper) << "LLM" << id << "not supported for assistant:" 
                                << m_private->m_assistantProxy.id 
                                << "name:" << m_private->m_assistantProxy.displayName;
            return false;
        }
    }

    LLMServerProxy account = LLMVendor()->setCurrentLLMAccountId(id, m_private->m_appId);
    if (!account.isValid()) {
        qCWarning(logWrapper) << "Invalid LLM account:" << id;
        return false;
    }

    if (m_private->m_assistantProxy.type == AssistantType::UOS_AI) {
        m_uosaiTopAccountId = account.id;
    }
    m_private->m_appServerProxy = account;

    //update role-model table
    DbWrapper::localDbWrapper().updateAssistantLlm(m_private->m_assistantProxy.id, account.id);
    emit llmAccountLstChanged(account.id, queryLLMAccountListWithRole());
    return true;
}

bool Session::setUosAiLLMAccountId(const QString &id)
{
    qCDebug(logWrapper) << "Setting UOS AI LLM account ID:" << id;
    
    if (id.isEmpty()) {
        qCWarning(logWrapper) << "Empty UOS AI LLM account ID provided";
        return false;
    }

    // 固定角色
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assis : assistantLst) {
        if (assis.type == AssistantType::UOS_AI) {
            {
                QList<LLMServerProxy> llms = LLMVendor()->queryServerAccountByRole(assis);
                bool found = false;
                if (!llms.isEmpty()) {
                    const auto it = std::find_if(llms.begin(), llms.end(), [id](const LLMServerProxy & account) {
                        return account.id == id;
                    });

                    found = it != llms.end();
                }

                if (!found) {
                    qCWarning(logWrapper) << "LLM" << id << "not supported for UOS AI assistant:" 
                                        << assis.id << "name:" << assis.displayName;
                    return false;
                }
            }

            LLMServerProxy account = LLMVendor()->setCurrentLLMAccountId(id, assis.id);
            if (!account.isValid()) {
                qCWarning(logWrapper) << "Invalid UOS AI LLM account:" << id;
                return false;
            }

            //update role-model table
            DbWrapper::localDbWrapper().updateAssistantLlm(assis.id, account.id);
            if (m_private->m_assistantProxy.type == AssistantType::UOS_AI) {
                emit llmAccountLstChanged(account.id, queryLLMAccountListWithRole());
            } else {
                emit uosAiLlmAccountLstChanged();
            }
            return true;
        }
    }

    qCWarning(logWrapper) << "No UOS AI assistant found";
    return false;
}

QString Session::currentLLMAccountId()
{
    return m_private->m_appServerProxy.id;
}

QString Session::uosAiLLMAccountId() {
    if (!m_uosaiTopAccountId.isEmpty()) {
        return m_uosaiTopAccountId;
    }

    // 固定角色
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assis : assistantLst) {
        if (assis.type == AssistantType::UOS_AI) {
            return DbWrapper::localDbWrapper().queryLlmIdByAssistantId(assis.id);
        }
    }

    return "";
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
    qCDebug(logWrapper) << "Setting current assistant ID:" << id;
    
    AssistantProxy assistant = DbWrapper::localDbWrapper().queryAssistantByid(id);
    if (!assistant.isValid()) {
        qCDebug(logWrapper) << "Assistant not found in local DB, querying from vendor";
        assistant = LLMVendor()->queryAssistantById(id);
        if (!assistant.isValid()) {
            qCWarning(logWrapper) << "Invalid assistant ID:" << id;
            return false;
        }
    }

    m_private->m_assistantProxy = assistant;
    qCDebug(logWrapper) << "Updating app's current assistant ID";
    DbWrapper::localDbWrapper().updateAppCurAssistantId(m_private->m_appId, id);

    QString llmId = DbWrapper::localDbWrapper().queryLlmIdByAssistantId(m_private->m_assistantProxy.id);

    //兼容 uos ai助手尝试去appid配置的模型
    if (llmId.isEmpty() && m_private->m_assistantProxy.type == UOS_AI) {
        qCDebug(logWrapper) << "No LLM found for assistant, trying app's configured model";
        llmId = DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appId());
    }

    if (!setCurrentLLMAccountId(llmId)) {
        qCDebug(logWrapper) << "Failed to set LLM account, trying first available LLM";
        auto supLLM = LLMVendor()->queryServerAccountByRole(m_private->m_assistantProxy);
        if (!supLLM.isEmpty()) {
            setCurrentLLMAccountId(supLLM.first().id);
        } else {
            qCWarning(logWrapper) << "No LLM available for assistant:" 
                                << m_private->m_assistantProxy.id 
                                << "name:" << m_private->m_assistantProxy.displayName;
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
    case AI_WRITING:
        return tr("AI Writing");
    case AI_TEXT_PROCESSING:
        return tr("AI Text Processing");
    case AI_TRANSLATION:
        return tr("AI Translation");
    default:
        return m_private->m_assistantProxy.displayName;
    }
}

QString Session::currentAssistantDisplayNameEn(){
    switch (m_private->m_assistantProxy.type) {
    case UOS_AI:
        return "UOS AI";
    case UOS_SYSTEM_ASSISTANT:
        return "UOS System Assistant";
    case DEEPIN_SYSTEM_ASSISTANT:
        return "Deepin System Assistant";
    case PERSONAL_KNOWLEDGE_ASSISTANT:
        return "Personal Knowledge Assistant";
    case AI_WRITING:
        return "AI Writing";
    case AI_TEXT_PROCESSING:
        return "AI Text Processing";
    case AI_TRANSLATION:
        return "AI Translation";
    default:
        return "3rdparty";
    }
}

AssistantType Session::currentAssistantType()
{
    return m_private->m_assistantProxy.type;
}

QString Session::currentAssistantInstList()
{
    return m_private->m_assistantProxy.instList;
}

QString Session::queryLLMAccountList(const QList<LLMChatModel> &excludes)
{
    qCDebug(logWrapper) << "Querying LLM account list with" << excludes.size() << "excluded models";
    return LLMVendor()->queryLLMAccountList(excludes);
}

QString Session::queryLLMAccountListWithRole(const QList<LLMChatModel> &excludes)
{
    qCDebug(logWrapper) << "Querying LLM account list for role:" << m_private->m_assistantProxy.id 
                       << "with" << excludes.size() << "excluded models";
    return LLMVendor()->queryLLMAccountListByRole(m_private->m_assistantProxy, excludes);
}

QString Session::queryUosAiLLMAccountList() {
    // 固定角色
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assis : assistantLst) {
        if (assis.type == AssistantType::UOS_AI) {
            return LLMVendor()->queryLLMAccountListByRole(assis, QList<LLMChatModel>());
        }
    }

    return "[]";
}

QString Session::queryAssistantList()
{
    QJsonArray assistantArray;
    // 固定角色
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    // 插件助手角色
    assistantLst << LLMVendor()->queryAssistantList();

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

QString Session::queryDisplayNameByType(AssistantType type)
{
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assistant : assistantLst) {
        if (assistant.type == type)
            return assistant.displayName;
    }

    return "";
}

AssistantType Session::queryAssistantTypeById(const QString &id)
{
    QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
    for (const AssistantProxy &assistant : assistantLst) {
        if (assistant.id == id)
            return assistant.type;
    }

    return AssistantType();
}

QString Session::queryIconById(const QString &assistantId, const QString &modelId)
{
    qCDebug(logWrapper) << "Querying icon for assistant:" << assistantId << "model:" << modelId;
    return LLMVendor()->queryIconById(assistantId, modelId);
}

QString Session::queryDisplayNameById(const QString &assistantId)
{
    qCDebug(logWrapper) << "Querying display name for assistant:" << assistantId;
    return LLMVendor()->queryDisplayNameById(assistantId);
}

void Session::launchLLMUiPage(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString &locateTitle)
{
    qCDebug(logWrapper) << "Launching LLM UI page - showAddllmPage:" << showAddllmPage 
                       << "onlyUseAgreement:" << onlyUseAgreement 
                       << "isFromAiQuick:" << isFromAiQuick;
    emit ServerWrapper::instance()->sigToLaunchMgmt(showAddllmPage, onlyUseAgreement, isFromAiQuick, locateTitle);
}

void Session::launchGetFreeAccountDlg()
{
    emit ServerWrapper::instance()->sigToLaunchGetFreeAccountDlg();
}

void Session::launchKnowledgeBaseUiPage(const QString &locateTitle)
{
    qCDebug(logWrapper) << "Launching knowledge base UI page";
    emit ServerWrapper::instance()->sigToLaunchMgmt(false, false, false, locateTitle);
}

void Session::launchAboutWindow()
{
    emit ServerWrapper::instance()->sigToLaunchAbout();
}

QVariant Session::getFAQ()
{
    return m_private->getFAQ();
}

LLMServerProxy Session::queryValidServerAccount(const QString &llmId)
{
    return m_private->queryValidServerAccount(llmId);
}
