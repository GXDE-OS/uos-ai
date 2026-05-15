#include "onlinesearchagent.h"
#include <QCoreApplication>

using namespace uos_ai;

OnlineSearchAgent::OnlineSearchAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "OnlineSearchAgent";
    m_description = "Online search agent for UOS AI.";
    m_systemPrompt = R"(You are %1.)";
}

QString OnlineSearchAgent::systemPrompt() const
{
    return m_systemPrompt.arg(QCoreApplication::translate("uos_ai::AssistantManager", "UOS AI"));
}
