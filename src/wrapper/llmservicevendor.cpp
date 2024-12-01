// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "llmservicevendor.h"
#include "dbwrapper.h"
#include "llmutils.h"
#include "externalllm/exteralllm.h"

#include <QJsonArray>

using namespace uos_ai;

LLMServiceVendor::LLMServiceVendor(QObject *parent) : QObject(parent)
{
}

void LLMServiceVendor::initModelPlugin()
{
    extServer.clear();
    llmLoader.readPlugins();
    auto plugins = llmLoader.plugins();
    for (QSharedPointer<LLMPlugin> p : plugins) {
        auto models = p->modelList();
        for (const QString &m : models) {
            QString id = QString("plugin_model_") + m;
            if (extServer.contains(m)) {
                qWarning() << "Models with duplicate names, please check if there are multiple plugins containing the same model name."
                           << m;
                continue;
            }

            LLMServerProxy sp;
            sp.id = id;
            sp.name = m;
            sp.model = PLUGIN_MODEL;
            sp.type = LOCAL;
            sp.account.apiKey = id;
            extServer.insert(id, sp);

            extPlugins.insert(id, p);

            qInfo() << "loaded external model" << m;
        }
    }
}

LLMServerProxy LLMServiceVendor::checkUpdateLLmAccount(const QString &llmId)
{
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
        auto ite = extServer.find(llmId);
        if (ite != extServer.end())
            tmpLLMAccount = ite.value();
    }

    return tmpLLMAccount;
}

LLMServerProxy LLMServiceVendor::queryValidServerAccount(const QString &llmId)
{
    const QList<LLMServerProxy> &accountLst = DbWrapper::localDbWrapper().queryLlmList();
    LLMServerProxy tmpLLMAccount;
    if (!accountLst.isEmpty()) {
        const auto it = std::find_if(accountLst.begin(), accountLst.end(), [llmId](const LLMServerProxy & account) {
            return account.id == llmId;
        });

        if (it != accountLst.end())
            tmpLLMAccount = *it;
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
    QList<LLMServerProxy> accountLst;
    if (role.type < PLUGIN_ASSISTANT) {
        accountLst << DbWrapper::localDbWrapper().queryLlmList();
    }

    for (const LLMServerProxy &sp : extServer.values()) {
        auto plugin = extPlugins.value(sp.id);
        if (plugin.isNull())
            continue;

        auto roles = plugin->roles(sp.name);
        //插件没有role,默认加入uosai
        if (roles.isEmpty() && role.type == UOS_AI)
            accountLst<< sp;
        else if (roles.contains(role.id))
            accountLst<< sp;
    }

    return accountLst;
}

LLMServerProxy LLMServiceVendor::setCurrentLLMAccountId(const QString &id, const QString &app)
{
    LLMServerProxy account;
    if (extServer.contains(id)) {
        account = extServer.value(id);
        DbWrapper::localDbWrapper().updateAppCurllmId(app, id);
        return account;
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
    QJsonArray llmAccountArray;
    const QList<LLMServerProxy> &llmAccountLst = DbWrapper::localDbWrapper().queryLlmList();
    for (const LLMServerProxy &account : llmAccountLst) {
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
        tmp = plugin->queryInfo(QUERY_DISPLAY_NAME, account.name).toString();
        accountObj["displayname"] = tmp.isEmpty() ? account.name : tmp;
        accountObj["model"] = account.model;
        accountObj["llmname"] = tmp.isEmpty() ? account.name : tmp;
        accountObj["type"] = account.type;
        tmp = plugin->queryInfo(QUERY_ICON_NAME, account.name).toString();
        accountObj["icon"] = tmp.isEmpty() ? account.llmIcon(LLMChatModel::PLUGIN_MODEL): tmp;

        llmAccountArray << accountObj;
    }

    return QJsonDocument(llmAccountArray).toJson(QJsonDocument::Compact);
}

QString LLMServiceVendor::queryLLMAccountListByRole(const AssistantProxy &role, const QList<LLMChatModel> &excludes)
{
    QJsonArray llmAccountArray;
    QList<LLMServerProxy> llmAccountLst = queryServerAccountByRole(role);
    for (const LLMServerProxy &account : llmAccountLst) {
        if (excludes.contains(account.model))
            continue;

        QJsonObject accountObj;
        if (account.model == PLUGIN_MODEL) {
            auto plugin = extPlugins.value(account.id);
            Q_ASSERT(plugin);

            QString tmp;
            accountObj["id"] = account.id;
            tmp = plugin->queryInfo(QUERY_DISPLAY_NAME, account.name).toString();
            accountObj["displayname"] = tmp.isEmpty() ? account.name : tmp;
            accountObj["model"] = account.model;
            accountObj["llmname"] = tmp.isEmpty() ? account.name : tmp;
            accountObj["type"] = account.type;
            tmp = plugin->queryInfo(QUERY_ICON_NAME, account.name).toString();
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
    return DbWrapper::localDbWrapper().queryCurLlmIdByAppId(appid);
}

QSharedPointer<LLM> LLMServiceVendor::getCopilot(const LLMServerProxy &serverproxy)
{
    if (extPlugins.contains(serverproxy.id)) {
        QSharedPointer<LLMPlugin> plugin = extPlugins.value(serverproxy.id);
        Q_ASSERT(plugin);

        LLMModel *om = plugin->createModel(serverproxy.name);
        if (!om)
            return nullptr;
        QSharedPointer<LLM> llm(new ExteralLLM(serverproxy, om));
        return llm;
    }

    return LLMUtils::getCopilot(serverproxy);
}

QList<AssistantProxy> LLMServiceVendor::queryAssistantList()
{
    QList<AssistantProxy> assistantList;

    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.name);
        for (QString role : roles) {
            AssistantProxy assistant;
            assistant.id = role;
            assistant.displayName = plugin->queryInfo(QUERY_DISPLAY_NAME, assistant.id).toString();
            assistant.type = PLUGIN_ASSISTANT;
            assistant.description = plugin->queryInfo(QUERY_DESCRIPTION, assistant.id).toString();;
            assistant.icon = plugin->queryInfo(QUERY_ICON_NAME, assistant.id).toString();
            assistant.iconPrefix = plugin->queryInfo(QUERY_ICON_PREFIX, QString("")).toString();

            assistantList.append(assistant);
        }
    }
    return assistantList;
}

AssistantProxy LLMServiceVendor::queryAssistantById(const QString &assistantId)
{
    AssistantProxy assistant;

    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.name);
        for (QString role : roles) {
            if (role == assistantId) {
                assistant.id = role;
                assistant.displayName = plugin->queryInfo(QUERY_DISPLAY_NAME, assistant.id).toString();
                assistant.type = PLUGIN_ASSISTANT;
                assistant.description = plugin->queryInfo(QUERY_DESCRIPTION, assistant.id).toString();;
                assistant.icon = plugin->queryInfo(QUERY_ICON_NAME, assistant.id).toString();
                assistant.iconPrefix = plugin->queryInfo(QUERY_ICON_PREFIX, QString("")).toString();

                break;
            }
        }
    }

    return  assistant;
}

QVariant LLMServiceVendor::getFAQ(const QString &assistantId)
{
    for (const LLMServerProxy &account : extServer.values()) {
        auto plugin = extPlugins.value(account.id);
        Q_ASSERT(plugin);

        QStringList roles = plugin->roles(account.name);
        for (QString role : roles) {
            if (role == assistantId) {
                return plugin->queryInfo(QUERY_QUESTIONS, assistantId);
            }
        }
    }
    return QVariant();
}
