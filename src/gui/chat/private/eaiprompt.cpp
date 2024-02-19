#include "eaiprompt.h"

EAiPrompt::EAiPrompt(const QString &userData, const QString &cond, int llmType)
    : m_userParam(userData)
    , m_userCond(cond)
    , m_llm(LLMChatModel(llmType))
{
    m_sysLang = QLocale::system();
}

EAiPrompt::~EAiPrompt()
{
}

void EAiPrompt::setLLM(int llm)
{
    m_llm = LLMChatModel(llm);
}
