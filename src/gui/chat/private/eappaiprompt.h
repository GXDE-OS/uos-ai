#ifndef EAIEAIPROMPT_H
#define EAIEAIPROMPT_H

#include "eaiprompt.h"

//Add all app ai prompt in this file.

//对话模式
class EConversationPrompt : public EAiPrompt
{
public:
    explicit EConversationPrompt(const QString &userData);

    QString getAiPrompt() override;
};

//文档总结
class EAiDocSummaryPrompt : public EAiPrompt
{
public:
    explicit EAiDocSummaryPrompt(const QString &userData, const QString &aiData);

    QString getAiPrompt() override;
};

// AI翻译助手
class EAiTranslationPrompt : public EAiPrompt
{
public:
    explicit EAiTranslationPrompt(const QString &userData);

    QString getAiPrompt() override;
};

// AI写作助手
class EAiWritingPrompt : public EAiPrompt
{
public:
    explicit EAiWritingPrompt(const QString &userData);

    QString getAiPrompt() override;
};

// AI文本处理助手
class EAiTextProcessingPrompt : public EAiPrompt
{
public:
    explicit EAiTextProcessingPrompt(const QString &userData);

    QString getAiPrompt() override;
};

// 生成问题
class EAiGenerateQuestionPrompt : public EAiPrompt
{
public:
    explicit EAiGenerateQuestionPrompt();

    QString getAiPrompt() override;
};

// 划词助手
class EAiWordSelectionPrompt : public EAiPrompt
{
public:
    explicit EAiWordSelectionPrompt(const QString &prompt);

    QString getAiPrompt() override;
};

// 指令
class EAiInstructionPrompt : public EAiPrompt
{
public:
    explicit EAiInstructionPrompt(const QString &prompt);

    QString getAiPrompt() override;
};

#endif // EAIEAIPROMPT_H
