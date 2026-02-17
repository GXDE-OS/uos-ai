#include "agent.h"

namespace uos_ai {

Agent::Agent()
{
}

Agent::~Agent()
{
}

QString Agent::name() const
{
    return m_name;
}

QString Agent::description() const
{
    return m_description;
}

QString Agent::systemPrompt() const
{
    return m_systemPrompt;
}

} // namespace uos_ai 
