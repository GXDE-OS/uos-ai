#ifndef TOOLUSE_H
#define TOOLUSE_H

#include <QJsonObject>
#include <QObject>

namespace uos_ai {
class LlmAgent;
struct ToolUse
{
    enum Staus { Calling = 0, Completed, Failed, Canceled };
    QString name;
    QString params;
    QString result;
    QString content;
    Staus status = Calling;
    QString index;

   QJsonObject toJson() const;

   /**
    * 发送工具调用内容
    * 将工具调用信息发送给客户端
    * @param {LlmAgent} agent - 要发送信息的智能体对象
    * @param {ToolUse} tool - 要发送的工具调用对象，包含工具名称和参数
    */
   static void toolUseContent(LlmAgent *agent, const ToolUse &tool);

   static void actionUseContent(LlmAgent *agent, const ToolUse &tool);
};


}

#endif // TOOLUSE_H
