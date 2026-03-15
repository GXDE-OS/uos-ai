#include "reasoninguse.h"
#include "llmagent.h"

#include <QJsonObject>
#include <QJsonDocument>

using namespace uos_ai;

void ReasoningUse::reasoningUseContent(LlmAgent *agent, const QString &content)
{
    QJsonObject message;

    message.insert("content", content);
    message.insert("chatType", ChatAction::AgentReasoning);  // 任务进度内容

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit agent->readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

void ReasoningUse::reasoningUseTitle(LlmAgent *agent, const QString &content, TitleStatus status)
{
    QJsonObject message;

    message.insert("content", content);
    message.insert("status", status);
    message.insert("chatType", ChatAction::AgentReasonTitle); // 任务标题

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit agent->readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}
