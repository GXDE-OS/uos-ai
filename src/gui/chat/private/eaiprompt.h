#ifndef EAIPROMPT_H
#define EAIPROMPT_H

#include <QString>
#include <QLocale>
#include <QJsonArray>

class EAiPrompt
{
public:
    enum RequstType {
        General       = 0,      // 通用 - 文本、Fc、文生图(旧接口)

        TextPlain     = 1,      // 纯文本聊天
        FunctionCall  = 2,      // FunctionCall
        Text2Image    = 3,       // 文生图
        Search        = 4,
        McpAgent      = 5,
        Rag           = 6,
    };

    explicit EAiPrompt(const QString &userData = QString(""), const QString &aiData = QString(""), int llmType = 1);

    virtual ~EAiPrompt();

    virtual QString getAiPrompt() = 0;
    virtual QString getUserParam() const;

    void setLLM(int llm);

    void setSingleReqCtx(bool enable);
    bool singleReqCtx();

    void setReqType(RequstType type);
    RequstType reqType();

    void setFunctions(const QJsonArray &functions);
    QJsonArray functions();

    void setInstType(int type);
    int instType();

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

    QString lengthValid(const QString &prompt);

protected:
    QString m_userParam;
    QString m_aiParam;

    LLMChatModel m_llm;

    QLocale m_sysLang;

    bool m_singleReqCtx {false};
    RequstType m_reqType {RequstType::General};
    QJsonArray m_funcs;
    int m_instType {-1};
};

#endif // EAIPROMPT_H
