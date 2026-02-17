#ifndef MCPCHATAGENT_H
#define MCPCHATAGENT_H

#include "mcpagent.h"
#include "toolparser.h"

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSet>

namespace uos_ai {

class MCPChatAgent : public MCPAgent
{
    Q_OBJECT
public:
    struct MCPChatCtx : public OutputCtx {
        QString *content = nullptr;         // 当前累积的输出内容，用于存储模型生成的文本
        QString *toolResults = nullptr;     // 工具调用结果的累积内容，用于存储工具执行的结果
        ToolParser *parser = nullptr;       // 工具解析器，用于解析模型输出中的工具调用指令
    };
public:
    /**
     * 构造函数
     * @param parent 父对象指针
     */
    explicit MCPChatAgent(QObject *parent = nullptr);
    
    /**
     * 析构函数
     */
    ~MCPChatAgent() override;

    /**
     * 处理用户请求
     * 处理来自用户的请求，包括文本对话和工具调用
     * @param {QJsonObject} question - 当前请求内容，包含用户的问题和相关参数
     * @param {QJsonArray} history - 聊天消息记录，包含之前的对话历史
     * @param {QJsonObject} params - 扩展参数，包含额外的配置信息
     * @returns {QJsonObject} 智能体工作流输出的消息记录
     */
    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;

protected:
    /**
     * 发送文本链式内容
     * 将文本内容以流式方式发送给客户端
     * @param {QString} content - 要发送的文本内容
     */
    void textChainContent(const QString &content);

    /**
     * 发送工具调用内容
     * 将工具调用信息发送给客户端
     * @param {ToolUse} tool - 要发送的工具调用对象，包含工具名称和参数
     */
    void toolUseContent(const ToolUse &tool);

    /**
     * 处理模型流式输出的内容
     * 解析模型的流式输出，识别文本内容和工具调用
     * @param {OutputCtx *} ctx - 上下文
     * @returns {bool} 处理是否成功，true表示成功处理，false表示处理失败
     */
    bool handleStreamOutput(OutputCtx *ctx) override;

    /**
     * 调用工具
     * 重写LlmAgent的callTool方法，使用MCP客户端调用工具
     * @param {QString} toolName - 工具名称
     * @param {QJsonObject} params - 工具参数
     * @returns {QPair<int, QString>} 状态码和结果内容
     */
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

protected:
    QList<ToolUse> m_usedTool;              // 已使用的工具列表，用于跟踪工具调用历史
};

} // namespace uos_ai

#endif // MCPCHATAGENT_H 
