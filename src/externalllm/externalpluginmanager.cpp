// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externalpluginmanager.h"
#include "llmplugin.h"

#include <QCryptographicHash>

using namespace uos_ai;

ExternalPluginManager::ExternalPluginManager(QObject *parent)
    : QObject(parent)
{

}

ExternalPluginManager::~ExternalPluginManager()
{
}

ExternalPluginManager *ExternalPluginManager::instance()
{
    static ExternalPluginManager instance;
    return &instance;
}

void ExternalPluginManager::init()
{
    if (m_initialized)
        return;
#ifdef ENABLE_MODEL_PLUGIN
    m_llmLoader.readPlugins();
#endif

#ifdef ENABLE_AGENT_PLUGIN
    m_agentLoader.readAgents();
    connect(&m_agentLoader, &ExternalAgentLoader::agentChanged, this, [this](){
        m_agents.clear();
        m_agentLoader.readAgents();
        refresh();
    });
#endif

    m_initialized = true;
}

ModelAccountPtr ExternalPluginManager::getModel(const QString &id) const
{
    for (auto it = m_assistantModels.begin(); it != m_assistantModels.end(); ++it) {
        for (const ModelAccountPtr &acc : it.value()) {
            if (acc->id == id)
                return acc;
        }
    }
    
    return {};
}

void ExternalPluginManager::loadPlugins()
{
#ifdef ENABLE_MODEL_PLUGIN
    m_plugins.clear();

    auto plugins = m_llmLoader.plugins();
    for (auto it = plugins.begin(); it != plugins.end() ; ++it) {
        QString path = it.value();
        QSharedPointer<LLMPlugin> plugin = it.key();
        if (plugin.isNull())
            continue;

        QStringList models = plugin->modelList();
        for (const QString &model : models) {
            ModelAccountPtr acc(new ModelAccount);
            acc->id = QString::fromUtf8(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex()) + "#" + model;
            acc->account.provider = STR_KEY_LLM_PLUGIN;
            acc->model.modelId = model;
            acc->model.arch = MaLanguage;
            acc->model.ability = ModelAbilities(ModelAbility::MaText);
            acc->model.name = plugin->queryInfo(QUERY_DISPLAY_NAME, model).toString();
            if (acc->model.name.isEmpty())
                acc->model.name = model;
            acc->network = STR_KEY_ONLINE;
            m_plugins.insert(acc->id, plugin);

            QStringList roles = plugin->roles(model);
            for (const QString &roleid : roles) {
                QString id = QString::fromUtf8(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex()) + "#" + roleid;
                if (!m_assistants.contains(id)) {
                    AssistantInfo info;
                    info.id = id;
                    info.name = plugin->queryInfo(QUERY_DISPLAY_NAME, roleid).toString();
                    info.description = plugin->queryInfo(QUERY_DESCRIPTION, roleid).toString();
                    info.path = plugin->queryInfo(QUERY_ICON_PREFIX, roleid).toString();

                    QVariantHash icons;
                    QString iconName = plugin->queryInfo(QUERY_ICON_NAME, roleid).toString();
                    if (!iconName.isEmpty()) {
                        icons["color"] = iconName;
                    }
                    info.icons = icons;

                    m_assistants.insert(id, info);
                }

                auto it = m_assistantModels.find(id);
                if (it != m_assistantModels.end())
                    it->append(acc);
                else
                    m_assistantModels.insert(id, {acc});
            }
        }
    }
#endif
}

void ExternalPluginManager::loadAgents()
{
#ifdef ENABLE_AGENT_PLUGIN
    m_agents.clear();

    auto agents = m_agentLoader.agents();
    for (auto it = agents.begin(); it != agents.end(); ++it) {
        QString path = it.value();
        QSharedPointer<ExternalAgent> agent = it.key();
        if (agent.isNull())
            continue;

        QList<ModelAccountPtr> models = agent->getModels();
        for (const ModelAccountPtr &model : models) {
            m_agents.insert(model->id, agent);

            QStringList roles = agent->roles(model->id);
            for (const QString &roleid : roles) {
                QString id = QString::fromUtf8(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex()) + "#" + roleid;
                if (!m_assistants.contains(id)) {
                    AssistantInfo info;
                    info.id = id;
                    info.name = agent->queryInfo(QUERY_DISPLAY_NAME, roleid).toString();
                    info.description = agent->queryInfo(QUERY_DESCRIPTION, roleid).toString();
                    info.path = agent->queryInfo(QUERY_ICON_PREFIX, roleid).toString();

                    QVariantHash icons;
                    QString iconName = agent->queryInfo(QUERY_ICON_NAME, roleid).toString();
                    if (!iconName.isEmpty()) {
                        icons["color"] = iconName;
                    }
                    info.icons = icons;
                    m_assistants.insert(id, info);
                }

                auto it = m_assistantModels.find(id);
                if (it != m_assistantModels.end())
                    it->append(model);
                else
                    m_assistantModels.insert(id, {model});
            }
        }
    }
#endif
}

void ExternalPluginManager::refresh()
{
    m_assistantModels.clear();
    m_assistants.clear();

    loadAgents();
    loadPlugins();

    emit assistantChanged();
}

