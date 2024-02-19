#include "eappaiprompt.h"

EConversationPrompt::EConversationPrompt(const QString &userData)
    : EAiPrompt(userData)
{
}

QString EConversationPrompt::getAiPrompt()
{
    QString output;

    switch (m_llm) {
    case CHATGPT_3_5:
    case CHATGPT_3_5_16K:
    case CHATGPT_4:
    case CHATGPT_4_32K:
//        output += "You are playing the role of an intelligent AI. I will input a piece of content, and your task is to give a best reply based on the input content and provide the output in the same language as the input.";
//        output += "\nInput:\n";
//        output +=  m_userParam + "\n";
//        output += "Output:";
        output += m_userParam;
        break;
    default:
        output += m_userParam;
        break;
    }

    return output;
}
