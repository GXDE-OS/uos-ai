#ifndef EAIPROMPT_H
#define EAIPROMPT_H

#include <QString>
#include <QLocale>


class EAiPrompt
{
public:
    explicit EAiPrompt(const QString &userData, const QString &cond = QString(""), int llmType = 1);

    virtual ~EAiPrompt();

    virtual QString getAiPrompt() = 0;

    void setLLM(int llm);

protected:
    enum LLMChatModel {
        CHATGPT_3_5          = 0,   // GPT3.5
        CHATGPT_3_5_16K      = 1,   // GPT3.5 16k
        CHATGPT_4            = 2,   // GPT4
        CHATGPT_4_32K        = 3,   // GPT4 32k
        SPARKDESK            = 10,  // 迅飞星火
        WENXINYIYAN          = 20,  // 百度文心一言
        WENXINWORKSHOP       = 30,  // 百度文心千帆
        GPT360_S2_V9         = 40,  // 360GPT
        ChatZHIPUGLM_PRO     = 50,  // 智谱AIPRO
        ChatZHIPUGLM_STD     = 51,  // 智谱AISTD
        ChatZHIPUGLM_LITE    = 52,  // 智谱AIpLITE
    };

protected:
    QString m_userParam;
    QString m_userCond;

    LLMChatModel m_llm;

    QLocale m_sysLang;
};

#endif // EAIPROMPT_H
