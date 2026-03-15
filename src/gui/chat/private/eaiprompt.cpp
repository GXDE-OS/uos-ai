#include "eaiprompt.h"

#define MAX_LENGTH 2e6

EAiPrompt::EAiPrompt(const QString &userData, const QString &aiData, int llmType)
    : m_userParam(userData)
    , m_aiParam(aiData)
    , m_llm(LLMChatModel(llmType))
{
    m_sysLang = QLocale::system();
}

EAiPrompt::~EAiPrompt()
{
}

QString EAiPrompt::getUserParam() const
{
    return m_userParam;
}

void EAiPrompt::setLLM(int llm)
{
    m_llm = LLMChatModel(llm);
}

void EAiPrompt::setSingleReqCtx(bool enable)
{
    m_singleReqCtx = enable;
}

bool EAiPrompt::singleReqCtx()
{
    return m_singleReqCtx;
}

void EAiPrompt::setReqType(EAiPrompt::RequstType type)
{
    m_reqType = type;
}

EAiPrompt::RequstType EAiPrompt::reqType()
{
    return m_reqType;
}

void EAiPrompt::setFunctions(const QJsonArray &functions)
{
    m_funcs = functions;
}

QJsonArray EAiPrompt::functions()
{
    return m_funcs;
}

void EAiPrompt::setInstType(int type)
{
    m_instType = type;
}

int EAiPrompt::instType()
{
    return m_instType;
}

void EAiPrompt::setParams(const QVariantHash &params)
{
    m_params = params;
}

QVariantHash EAiPrompt::getParams()
{
    return m_params;
}

QString EAiPrompt::lengthValid(const QString &prompt)
{
    if (prompt.length() > MAX_LENGTH) {
        return prompt.left(MAX_LENGTH);
    }

    return prompt;
}
