// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "llmservicevendor.h"
#include "dbwrapper.h"
#include "llmutils.h"
#include "externalllm/externalllm.h"
#include "externalllm/externalagentloader.h"
#include "modelhub/llminmodelhub.h"
#include "utils/chineseletterhelper.h"

#include <QJsonArray>
#include <QLoggingCategory>
#include <QThread>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logWrapper)

// 单例模式实现
LLMServiceVendor *LLMServiceVendor::instance()
{
    static LLMServiceVendor ins;
    return &ins;
}

LLMServiceVendor::LLMServiceVendor(QObject *parent) : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

void LLMServiceVendor::initExternal()
{
    QMutexLocker locker(&m_mutex);
    if (inited)
        return;

    inited = true;

    extServer.clear();
#ifdef ENABLE_MODEL_PLUGIN
    initModelPlugin();
#endif

#ifdef ENABLE_AGENT_PLUGIN
    initAgent();
#endif

}

void LLMServiceVendor::initModelPlugin()
{
    llmLoader.readPlugins();
    auto plugins = llmLoader.plugins();
    for (QSharedPointer<LLMPlugin> p : plugins) {
        auto models = p->modelList();
        for (const QString &m : models) {
            if (extServer.contains(m)) {
                qCWarning(logWrapper) << "Duplicate model name detected:" << m;
                continue;
            }

            LLMServerProxy sp;
            sp.id = m;
            sp.name = m;
            sp.model = PLUGIN_MODEL;
            sp.type = LOCAL;
            sp.account.apiKey = m;
            extServer.insert(m, sp);

            extPlugins.insert(m, p);

            qCInfo(logWrapper) << "External model loaded:" << m;
        }
    }
}

void LLMServiceVendor::initAgent()
{
    agentLoader.readAgents();
    for (QSharedPointer<ExternalAgent> ptr : agentLoader.agents()) {
        for (const LLMServerProxy &m : ptr->getModels()) {
            if (extServer.contains(m.id)) {
                qCWarning(logWrapper) << "Duplicate model ID detected:" << m.id << "name:" << m.name;
            }
            qCInfo(logWrapper) << "Agent added:" << m.id << "name:" << m.name;
            extServer.insert(m.id, m);
            extPlugins.insert(m.id, ptr);
        }
    }

    connect(&agentLoader, &ExternalAgentLoader::agentChanged, this, &LLMServiceVendor::updateAgent);
}

void LLMServiceVendor::updateAgent()
{
    QMutexLocker locker(&m_mutex);
    QSet<QString> oldList;
    for (QSharedPointer<ExternalAgent> ptr : agentLoader.agents()) {
        for (const LLMServerProxy &m : ptr->getModels())
            oldList.insert(m.id);
    }

    agentLoader.readAgents();
    QSet<QString> newList;
    for (QSharedPointer<ExternalAgent> ptr : agentLoader.agents()) {
        for (const LLMServerProxy &m : ptr->getModels()) {
            if (oldList.contains(m.id))
                qCInfo(logWrapper) << "Agent updated:" << m.id << "name:" << m.name;
            else
                qCInfo(logWrapper) << "New agent added:" << m.id << "name:" << m.name;

            extServer.insert(m.id, m);
            extPlugins.insert(m.id, ptr);

            newList.insert(m.id);
        }
    }

    for (const QString &id : oldList) {
        if (newList.contains(id))
            continue;
        qCInfo(logWrapper) << "Agent removed:" << id;
        extServer.remove(id);
        extPlugins.remove(id);
    }

    locker.unlock();
    emit agentChanged();
}

LLMServerProxy LLMServiceVendor::checkUpdateLLmAccount(const QString &llmId)
{
    QMutexLocker locker(&m_mutex);
    LLMServerProxy tmpLLMAccount;
    const QList<LLMServerProxy> &accountList = DbWrapper::localDbWrapper().queryLlmList();

    bool find = false;
    if (!accountList.isEmpty()) {
        const auto it = std::find_if(accountList.begin(), accountList.end(), [llmId](const LLMServerProxy & account) {
            return account.id == llmId;
        });

        if (it != accountList.end()) {
            tmpLLMAccount = *it;
            find = true;
        } else {
            tmpLLMAccount = accountList.value(0);
        }
    }

    if (!find) {
        for (const LLMServerProxy &sp : modelhub.modelList()) {
            if (sp.id == llmId) {
                tmpLLMAccount = sp;
                find = true;
            }
        }
    }

    if (!find) {
        auto ite = extServer.find(llmId);
        if (ite != extServer.end())
            tmpLLMAccount = ite.value();
    }

    return tmpLLMAccount;
}

LLMServerProxy LLMServiceVendor::queryValidServerAccount(const QString &llmId)
{
    QMutexLocker locker(&m_mutex);
    const QList<LLMServerProxy> &accountLst = DbWrapper::localDbWrapper().queryLlmList();
    LLMServerProxy tmpLLMAccount{};
    if (!accountLst.isEmpty()) {
        const auto it = std::find_if(accountLst.begin(), accountLst.end(), [llmId](const LLMServerProxy & account) {
            return account.id == llmId;
        });

        if (it != accountLst.end())
            tmpLLMAccount = *it;
    }

    if (!tmpLLMAccount.isValid()) {
        for (const LLMServerProxy &sp : modelhub.modelList()) {
            if (sp.id == llmId)
                tmpLLMAccount = sp;
        }
    }

    if (!tmpLLMAccount.isValid()) {
        auto ite = extServer.find(llmId);
        if (ite != extServer.end())
            tmpLLMAccount = ite.value();
    }

    return tmpLLMAccount;
}

QList<LLMServerProxy> LLMServiceVendor::queryServerAccountByRole(const AssistantProxy &role)
{
    QMutexLocker locker(&m_mutex);
    QList<LLMServerProxy> accountLst;

    if (role.type < PLUGIN_ASSISTANT) {
        accountLst << DbWrapper::localDbWrapper().queryLlmList();
        accountLst << modelhub.modelList();
    }

    for (const LLMServerProxy &sp : extServer.values()) {
        auto plugin = extPlugins.value(sp.id);
        if (plugin.isNull())
            continue;

        auto roles = plugin->roles(sp.id);
        //插件没有role,默认加入uosai
        if (roles.isEmpty() && role.type == UOS_AI)
            accountLst << sp;
        else if (roles.contains(role.id))
            accountLst<< sp;
    }

    return accountLst;
}

LLMServerProxy LLMServiceVendor::setCurrentLLMAccountId(const QString &id, const QString &app)
{
    QMutexLocker locker(&m_mutex);
    LLMServerProxy account;
    if (extServer.contains(id)) {
        account = extServer.value(id);
        DbWrapper::localDbWrapper().updateAppCurllmId(app, id);
        return account;
    }

    for (const LLMServerProxy &tmp : modelhub.modelList()) {
        if (tmp.id == id) {
            account = tmp;
            DbWrapper::localDbWrapper().updateAppCurllmId(app, id);
            return account;
        }
    }

    account = DbWrapper::localDbWrapper().queryLlmByLlmid(id);
    if (account.isValid()) {
        DbWrapper::localDbWrapper().updateAppCurllmId(app, id);
        return account;
    }

    return account;
}

QString LLMServiceVendor::queryLLMAccountList(const QList<LLMChatModel> &excludes)
{
    QMutexLocker locker(&m_mutex);
    QJsonArray llmAccountArray;
    for (const LLMServerProxy &account : DbWrapper::localDbWrapper().queryLlmList()) {
        if (excludes.contains(account.model))
            continue;

        QJsonObject accountObj;
        accountObj["id"] = account.id;
        accountObj["displayname"] = account.name;
        accountObj["model"] = account.model;
        accountObj["llmname"] = LLMServerProxy::llmName(account.model, !account.url.isEmpty());
        accountObj["type"] = account.type;
        accountObj["icon"] = account.llmIcon(account.model);

        llmAccountArray << accountObj;
    }

    for (const LLMServerProxy &account : modelhub.modelList()) {
        if (excludes.contains(account.model))
            continue;

        QJsonObject accountObj;
        accountObj["id"] = account.id;
        accountObj["displayname"] = account.name;
        accountObj["model"] = account.model;
        accountObj["llmname"] = LLMServerProxy::llmName(account.model, !account.url.isEmpty());
        accountObj["type"] = account.type;
        accountObj["icon"] = account.llmIcon(account.model);

        llmAccountArray << accountObj;
    }

    for (const LLMServerProxy &account : extServer.values()) {
        if (excludes.contains(account.model))
            continue;

        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QString tmp;
        QJsonObject accountObj;
        accountObj["id"] = account.id;
        tmp = plugin->queryInfo(QUERY_DISPLAY_NAME, account.id).toString();
        accountObj["displayname"] = tmp.isEmpty() ? account.name : tmp;
        accountObj["model"] = account.model;
        accountObj["llmname"] = tmp.isEmpty() ? account.name : tmp;
        accountObj["type"] = account.type;
        tmp = plugin->queryInfo(QUERY_ICON_NAME, account.id).toString();
        accountObj["icon"] = tmp.isEmpty() ? account.llmIcon(LLMChatModel::PLUGIN_MODEL): tmp;

        llmAccountArray << accountObj;
    }

    return QJsonDocument(llmAccountArray).toJson(QJsonDocument::Compact);
}

QString LLMServiceVendor::queryLLMAccountListByRole(const AssistantProxy &role, const QList<LLMChatModel> &excludes)
{
    QJsonArray llmAccountArray;
    QList<LLMServerProxy> llmAccountLst = queryServerAccountByRole(role);

    QMutexLocker locker(&m_mutex);
    for (const LLMServerProxy &account : llmAccountLst) {
        if (excludes.contains(account.model))
            continue;

        QJsonObject accountObj;
        if (account.model == PLUGIN_MODEL) {
            auto plugin = extPlugins.value(account.id);
            Q_ASSERT(plugin);

            QString tmp;
            accountObj["id"] = account.id;
            tmp = plugin->queryInfo(QUERY_DISPLAY_NAME, account.id).toString();
            accountObj["displayname"] = tmp.isEmpty() ? account.name : tmp;
            accountObj["model"] = account.model;
            accountObj["llmname"] = tmp.isEmpty() ? account.name : tmp;
            accountObj["type"] = account.type;
            tmp = plugin->queryInfo(QUERY_ICON_NAME, account.id).toString();
            accountObj["icon"] = tmp.isEmpty() ? account.llmIcon(LLMChatModel::PLUGIN_MODEL): tmp;
        } else {
            accountObj["id"] = account.id;
            accountObj["displayname"] = account.name;
            accountObj["model"] = account.model;
            accountObj["llmname"] = LLMServerProxy::llmName(account.model, !account.url.isEmpty());
            accountObj["type"] = account.type;
            accountObj["icon"] = account.llmIcon(account.model);
        }

        llmAccountArray << accountObj;
    }

    return QJsonDocument(llmAccountArray).toJson(QJsonDocument::Compact);
}

QString LLMServiceVendor::queryCurLlmIdByAppId(const QString &appid)
{
    QMutexLocker locker(&m_mutex);
    return DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appid);
}

QSharedPointer<LLM> LLMServiceVendor::getCopilot(const LLMServerProxy &serverproxy)
{       
    QMutexLocker locker(&m_mutex);
    if (extPlugins.contains(serverproxy.id)) {
        QSharedPointer<LLMPlugin> plugin = extPlugins.value(serverproxy.id);
        Q_ASSERT(plugin);

        if (auto agent = dynamic_cast<ExternalAgent*>(plugin.data()))
            return LLMUtils::getCopilot(serverproxy);

        LLMModel *om = plugin->createModel(serverproxy.id);
        if (!om) {
            qCWarning(logWrapper) << "Failed to create model for server:" << serverproxy.id;
            return nullptr;
        }
        QSharedPointer<LLM> llm(new ExternalLLM(serverproxy, om));
        return llm;
    }

    if (serverproxy.llmManufacturer(serverproxy.model) == ModelManufacturer::MODELHUB) {
        auto llm = new LLMinModelHub(modelhub.getWrapper(serverproxy.id), serverproxy);
        return QSharedPointer<LLM>(llm);
    }

    return LLMUtils::getCopilot(serverproxy);
}

QList<AssistantProxy> LLMServiceVendor::queryAssistantList()
{
    QMutexLocker locker(&m_mutex);
    QList<AssistantProxy> assistantList;

    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.id);
        for (QString role : roles) {
            AssistantProxy assistant;
            assistant.id = role;
            assistant.displayName = plugin->queryInfo(QUERY_DISPLAY_NAME, assistant.id).toString();
            assistant.type = PLUGIN_ASSISTANT;
            assistant.description = plugin->queryInfo(QUERY_DESCRIPTION, assistant.id).toString();;
            assistant.icon = plugin->queryInfo(QUERY_ICON_NAME, assistant.id).toString();
            assistant.iconPrefix = plugin->queryInfo(QUERY_ICON_PREFIX, assistant.id).toString();
            assistant.instList = plugin->queryInfo(QUERY_INSTLIST, QString("")).toString();

            assistantList.append(assistant);
        }
    }

    std::stable_sort(assistantList.begin(), assistantList.end(), [](const AssistantProxy &t1, const AssistantProxy &t2) {
        QString p1;
        QString p2;
        QString tmp;
        Ch2PyIns->convertChinese2Pinyin(t1.displayName, tmp, p1);
        Ch2PyIns->convertChinese2Pinyin(t2.displayName, tmp, p2);

        return p1 < p2;
    });
    return assistantList;
}

AssistantProxy LLMServiceVendor::queryAssistantById(const QString &assistantId)
{
    QMutexLocker locker(&m_mutex);
    AssistantProxy assistant;

    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.id);
        for (QString role : roles) {
            if (role == assistantId) {
                assistant.id = role;
                assistant.displayName = plugin->queryInfo(QUERY_DISPLAY_NAME, assistant.id).toString();
                assistant.type = PLUGIN_ASSISTANT;
                assistant.description = plugin->queryInfo(QUERY_DESCRIPTION, assistant.id).toString();;
                assistant.icon = plugin->queryInfo(QUERY_ICON_NAME, assistant.id).toString();
                assistant.iconPrefix = plugin->queryInfo(QUERY_ICON_PREFIX, QString("")).toString();
                assistant.instList = plugin->queryInfo(QUERY_INSTLIST, QString("")).toString();

                break;
            }
        }
    }

    return  assistant;
}

QString LLMServiceVendor::queryIconById(const QString &assistantId, const QString &modelId)
{
    QMutexLocker locker(&m_mutex);
    QString iconPath;
    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.id);
        for (QString role : roles) {
            if (role == assistantId) {
                iconPath = plugin->queryInfo(QUERY_ICON_NAME, modelId).toString();
                break;
            }
        }
    }
    return iconPath;
}

QString LLMServiceVendor::queryDisplayNameById(const QString &assistantId)
{
    QMutexLocker locker(&m_mutex);
    QString displayName;
    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.id);
        for (QString role : roles) {
            if (role == assistantId) {
                displayName = plugin->queryInfo(QUERY_DISPLAY_NAME, assistantId).toString();
                break;
            }
        }
    }
    return displayName;
}

QVariant LLMServiceVendor::getFAQ(const QString &assistantId)
{
    QMutexLocker locker(&m_mutex);
    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.id);
        for (QString role : roles) {
            if (role == assistantId) {
                return plugin->queryInfo(QUERY_QUESTIONS, assistantId);
            }
        }
    }
    return QVariant();
}
