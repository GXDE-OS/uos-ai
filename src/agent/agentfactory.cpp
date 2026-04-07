#include "agentfactory.h"

#include "appagent.h"
#include "defaultagentwithskills.h"
#include "research/writingmasteragent.h"
#include "chatbotagent.h"

#include <QDebug>
#include <QMutexLocker>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

AgentFactory *AgentFactory::instance()
{
    static AgentFactory ins;
    return &ins;
}

bool AgentFactory::registerAgent(const QString &name, CreateAgent creator)
{
    QMutexLocker locker(&m_mutex);

    if (!creator) {
        qCWarning(logAgent) << "Failed to register agent:" << name << "due to null creator.";
        return false;
    }

    if (m_agents.contains(name)) {
        qCWarning(logAgent) << "Agent already registered:" << name;
        return false;
    }

    m_agents.insert(name, creator);

    qCInfo(logAgent) << "Agent registered successfully:" << name;
    return true;
}

QSharedPointer<LlmAgent> AgentFactory::getAgent(const QString &name)
{
    QMutexLocker locker(&m_mutex);
    if (!m_agents.contains(name)) {
        qCWarning(logAgent) << "Agent not found:" << name;
        return nullptr;
    }

    QSharedPointer<LlmAgent> agent = m_agents[name]();
    return agent;
}

QSharedPointer<MCPServer> AgentFactory::getMCPServer(const QString &name)
{
    auto agent = qSharedPointerDynamicCast<MCPAgent>(getAgent(name));
    if (agent)
        return agent->mcpServer();
    return nullptr;
}

QStringList AgentFactory::agentNames() const
{
    QMutexLocker locker(&m_mutex);
    return m_agents.keys();
}

AgentFactory::AgentFactory(QObject *parent) : QObject(parent)
{
    if (0)
    {
        AppAgent tmp;

        if (registerAgent(tmp.name(), AppAgent::create)) {
            qCInfo(logAgent) << "AppAgent registered during initialization.";
        }
    }

    {
        DefaultAgentWithSkills dflt;

        if (registerAgent(dflt.name(), DefaultAgentWithSkills::create)) {
            qCInfo(logAgent) << "DefaultAgentWithSkills registered during initialization.";
        }
    }

    {
        WritingMasterAgent writingMaster;
        if (registerAgent(writingMaster.name(), WritingMasterAgent::create)) {
            qCInfo(logAgent) << "WritingMasterAgent registered during initialization.";
        }
    }

    {
        uos_ai::chatbot::ChatbotAgent chatbotAgent;
        if (registerAgent(chatbotAgent.name(), uos_ai::chatbot::ChatbotAgent::create)) {
            qCInfo(logAgent) << "ChatbotAgent registered during initialization.";
        }
    }

    qCInfo(logAgent) << "AgentFactory initialized.";
}

AgentFactory::~AgentFactory()
{

}

} // namespace uos_ai 
