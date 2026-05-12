#ifndef CHATBOTASSISTANT_H
#define CHATBOTASSISTANT_H

#include "assistant/abstractassistant.h"

namespace uos_ai {
namespace chatbot {

inline constexpr char STR_KEY_UOS_AI_CHATBOT[] = "uos-ai-chatbot";

/**
 * ChatBotAssistant - 专为 IM chatbot 对话设计的 Assistant
 *
 * 结构与 UOSClaw 类似，但使用 ChatbotAgent：
 *  - ChatbotAgent 在 DefaultAgent 的基础上追加 update_profile、Skill 工具，
 *    并在 processRequest 中注入身份/历史摘要等 system prompt
 *  - 运行参数通过 m_parameters 注入（platform/history_summary/mcpServers）
 */
class ChatBotAssistant : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit ChatBotAssistant(QObject *parent = nullptr);

    void cancel() override;

Q_SIGNALS:
    void requestCancel();

protected:
    QVariantHash run() override;

    void processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry);
};

} // namespace chatbot
} // namespace uos_ai

#endif // CHATBOTASSISTANT_H
