#include "eappaiprompt.h"
#include "eparserdocument.h"
#include "embeddingserver.h"
#include "global.h"

#include <QJsonArray>
#include <QStandardPaths>

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

EAiDocSummaryPrompt::EAiDocSummaryPrompt(const QString &userData, const QString &aiData)
    : EAiPrompt(userData, aiData)
{
}

QString EAiDocSummaryPrompt::getAiPrompt()
{
    QString promptTemplate = "你是一个文档助手，根据文档内容回答问题。\n" \
                     "文档内容：%1 \n" \
                     "问题：%2 \n";
    QString aiPrompt = promptTemplate.arg(m_aiParam).arg(m_userParam);

    return lengthValid(aiPrompt);
}

EAiGenerateQuestionPrompt::EAiGenerateQuestionPrompt()
{
}

QString EAiGenerateQuestionPrompt::getAiPrompt()
{
    QStringList docContents = EmbeddingServer::getInstance().getDocContent();
    QString chunk;
    int it = 1;
    for (const QString &docContent : docContents) {
        chunk += QString::number(it) + ":" + docContent + "\n";
        it++;
    }

    QString context;
    QString promptTemplate ="你是问答任务的助手。\n" \
                            "使用以下背景知识生成问题，中文一组，英文一组，两种语言各20个问题。输出格式为JSON，示例如下：\n" \
                            "```json\n{\n    \"chinese\": [\n    \"问题1\",\n    \"问题2\"\n]\n\"english\": [\n    \"question1\",\n    \"question2\"\n]\n}```\n" \
                            "背景知识：%1\n" \
                            "问题：";
    context = promptTemplate.arg(chunk);

    return context;
}

EAiWordSelectionPrompt::EAiWordSelectionPrompt(const QString &prompt)
    : EAiPrompt(prompt)
{
}

QString EAiWordSelectionPrompt::getAiPrompt()
{
    return m_userParam;
}

EAiInstructionPrompt::EAiInstructionPrompt(const QString &prompt)
    : EAiPrompt(prompt)
{
}

QString EAiInstructionPrompt::getAiPrompt()
{
    return m_userParam;
}

EAiWritingPrompt::EAiWritingPrompt(const QString &userData)
    : EAiPrompt(userData)
{
}

QString EAiWritingPrompt::getAiPrompt()
{
    QString promptTemplate = QCoreApplication::translate("EAiPrompt", "---Role---\n" \
                                                  "You are a professional writer with expertise in various writing styles and formats.\n\n" \
                                                  "---Goal---\n" \
                                                  "Based on the user's request, create high-quality content that meets their specific needs.\n\n" \
                                                  "---Writing Types---\n" \
                                                  "1. Article: Write well-structured articles with clear arguments and engaging content\n" \
                                                  "2. Speeches: Create compelling speeches with proper opening, body, and conclusion\n\n" \
                                                  "3. Outlines: Create a structured outline for the given topic\n" \
                                                  "4. Notifications: Write formal notices or announcements\n" \
                                                  "5. Posts: Create engaging social media content\n" \
                                                  "6. Work Report: Write comprehensive work reports\n" \
                                                  "7. Research Report: Create detailed research reports\n" \
                                                  "---Input---\n" \
                                                  "%1\n\n" \
                                                  "---Output Format---\n" \
                                                  "Please provide the content in the following format:\n" \
                                                  "Title:\n" \
                                                  "Content:");

    return promptTemplate.arg(m_userParam);
}

EAiTextProcessingPrompt::EAiTextProcessingPrompt(const QString &userData)
    : EAiPrompt(userData)
{
}

QString EAiTextProcessingPrompt::getAiPrompt()
{
    QString promptTemplate = QCoreApplication::translate("EAiPrompt", "---Role---\n" \
                                                  "You are a text processing expert with deep expertise in language analysis, writing techniques, and text optimization.\n\n" \
                                                  "---Goal---\n" \
                                                  "Process and improve the input text while maintaining its original meaning and style.\n\n" \
                                                  "---Processing Types---\n" \
                                                  "1. Summary: Create a concise summary of the main points\n" \
                                                  "2. Proofread: Identify and fix grammatical, spelling, and stylistic errors\n" \
                                                  "3. Explain: Provide detailed explanations of complex concepts or passages\n" \
                                                  "4. Expand: Elaborate on key points while maintaining the original style\n" \
                                                  "5. Continue: Continue the text in a coherent and natural way\n" \
                                                  "6. Polish: Enhance the text's clarity, flow, and impact while preserving its essence\n\n" \
                                                  "---Input---\n" \
                                                  "%1\n\n" \
                                                  "---Output Format---\n" \
                                                  "Please provide the processed text directly without any additional characters or formatting.");

    return promptTemplate.arg(m_userParam);
}

EAiTranslationPrompt::EAiTranslationPrompt(const QString &userData)
    : EAiPrompt(userData)
{
}

QString EAiTranslationPrompt::getAiPrompt()
{
    QString promptTemplate = QCoreApplication::translate("EAiPrompt",
            "---Role---\n"
            "You are a professional translator and language expert. Your task is to:\n"
            "1. Accurately translate text as requested\n"
            "2. Answer language-related questions\n"
            "3. Identify and handle specific translation requests\n\n"
            "---Goal---\n"
            "Carefully analyze the user's input to determine their intent:\n"
            "1. If the input contains phrases like 'what does this mean', 'explain this phrase', or similar - provide ONLY a language explanation (no translation)\n"
            "2. If the input specifies a particular sentence/paragraph to translate (e.g. 'Translate this sentence:...') - translate ONLY the specified portion (no full translation)\n"
            "3. Only when no specific request is detected, perform full translation between Chinese and English\n\n"
            "Always maintain the original meaning, tone, and style. For translations:\n"
            "- Chinese → English\n"
            "- English → Chinese\n\n"
            "For language questions, provide clear, concise explanations.\n\n"
            "---Input---\n"
            "%1\n\n"
            "---Output Format---\n"
            "1. For full translations: Provide only the translation without additional formatting\n"
            "2. For partial translations: Provide only the specified portion's translation\n"
            "3. For explanations: Provide only the explanation in the same language as the question\n"
            "Never combine explanation with translation or provide full translation after partial translation"
        );

    return promptTemplate.arg(m_userParam);
}
