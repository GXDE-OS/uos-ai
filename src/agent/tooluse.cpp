#include "tooluse.h"
#include "llmagent.h"

using namespace uos_ai;

QJsonObject ToolUse::toJson() const
{
    QJsonObject obj;
    obj.insert("name", name);
    obj.insert("params", params);
    obj.insert("result", result);
    obj.insert("content", content);
    obj.insert("status", static_cast<int>(status));
    obj.insert("index", index);
    return  obj;
}

void ToolUse::toolUseContent(LlmAgent *agent, const ToolUse &tool)
{
    if (!agent)
        return;

    QJsonObject message = tool.toJson();
    message.insert("chatType", ChatAction::ChatToolUse);

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit agent->readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

void ToolUse::actionUseContent(LlmAgent *agent, const ToolUse &tool)
{
    if (!agent)
        return;

    QJsonObject message = tool.toJson();
    message.insert("chatType", ChatAction::AgentAction);

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit agent->readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}
