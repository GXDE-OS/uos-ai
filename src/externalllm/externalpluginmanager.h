// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERNALPLUGINMANAGER_H
#define EXTERNALPLUGINMANAGER_H

#include "externalllmloader.h"
#include "externalagentloader.h"
#include "externalagent.h"
#include "assistant/assistantinfo.h"
#include "model/modelinfo.h"

#include <QObject>
#include <QSharedPointer>
#include <QMap>

namespace uos_ai {

class ExternalPluginManager : public QObject
{
    Q_OBJECT
public:
    static ExternalPluginManager *instance();

    void init();
    ModelAccountPtr getModel(const QString &id) const;
    inline bool hasAssistant(const QString &assistantId) const { return m_assistants.contains(assistantId); }
    inline QList<AssistantInfo> getExternalAssistants() const { return m_assistants.values(); }
    inline QList<ModelAccountPtr> getModelsByAssistant(const QString &assistantId) const { return m_assistantModels.value(assistantId); }

    inline QSharedPointer<ExternalAgent> getAgent(const QString &modelId) const { return m_agents.value(modelId); }
    inline QSharedPointer<LLMPlugin> getPlugin(const QString &modelId) const { return m_plugins.value(modelId); }

public slots:
    void refresh();

signals:
    void assistantChanged();

private:
    explicit ExternalPluginManager(QObject *parent = nullptr);
    ~ExternalPluginManager();
    void loadPlugins();
    void loadAgents();
private:
#ifdef ENABLE_MODEL_PLUGIN
    ExternalLLMLoader m_llmLoader;
#endif
#ifdef ENABLE_AGENT_PLUGIN
    ExternalAgentLoader m_agentLoader;
#endif

    QMap<QString, QSharedPointer<LLMPlugin>> m_plugins;
    QMap<QString, QSharedPointer<ExternalAgent>> m_agents;

    QMap<QString, QList<ModelAccountPtr>> m_assistantModels;
    QMap<QString, AssistantInfo> m_assistants;

    bool m_initialized = false;
};

}

#endif // EXTERNALPLUGINMANAGER_H
