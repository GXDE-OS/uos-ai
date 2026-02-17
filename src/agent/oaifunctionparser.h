#ifndef OAIFUNCTIONPARSER_H
#define OAIFUNCTIONPARSER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace uos_ai {

/**
 * @brief OpenAI function call解析器
 * 
 * 支持解析OpenAI API的function call回复，包括：
 * - 旧格式：function_call
 * - 新格式：tool_calls
 * 
 * 解析后返回工具名称和参数
 */
class OAIFunctionParser
{
public:
    /**
     * @brief 工具调用信息结构
     */
    struct ToolCall {
        QString id;              // 工具调用ID（tool_calls格式）
        QString name;            // 工具名称
        QJsonObject arguments;   // 工具参数
        int index;              // 索引（tool_calls格式）
        QString type;           // 类型（tool_calls格式）
        
        ToolCall() : index(-1) {}
    };

    explicit OAIFunctionParser();

    /**
     * @brief 解析OpenAI响应
     * 
     * @param response OpenAI API响应JSON对象
     * @return QList<ToolCall> 解析结果
     */
    QList<ToolCall> parseResponse(const QJsonObject &response);

    /**
     * @brief 解析单个function_call对象
     * 
     * @param functionCall function_call JSON对象
     * @return ToolCall 工具调用信息
     */
    static ToolCall parseFunctionCall(const QJsonObject &functionCall);

    /**
     * @brief 解析单个tool_call对象
     * 
     * @param toolCall tool_call JSON对象
     * @return ToolCall 工具调用信息
     */
    static ToolCall parseToolCall(const QJsonObject &toolCall);

    /**
     * @brief 解析工具参数JSON字符串
     * 
     * @param argumentsJson 参数JSON字符串
     * @return QJsonObject 解析后的参数对象
     */
    static QJsonObject parseArguments(const QString &argumentsJson);

    /**
     * @brief 检查响应是否包含工具调用
     * 
     * @param response OpenAI API响应JSON对象
     * @return bool 是否包含工具调用
     */
    static bool hasToolCalls(const QJsonObject &response);

    /**
     * @brief 获取工具调用列表
     * 
     * @param response OpenAI API响应JSON对象
     * @return QList<ToolCall> 工具调用列表
     */
    static QList<ToolCall> extractToolCalls(const QJsonObject &response);

    /**
     * @brief 将工具调用重新组装为OAI格式的聊天记录
     * 
     * @param tools 工具调用列表
     * @return QJsonArray 包含assistant和tool消息的聊天记录数组
     */
    static QJsonArray buildChatMessages(const QList<ToolCall> &tools);

    /**
     * @brief 创建OAI格式的assistant消息
     * 
     * @param tools 工具调用列表
     * @return QJsonObject assistant消息对象
     */
    static QJsonObject createAssistantMessage(const QList<ToolCall> &tools);

    /**
     * @brief 创建OAI格式的tool消息
     * 
     * @param toolCall 工具调用信息
     * @param result 工具执行结果
     * @return QJsonObject tool消息对象
     */
    static QJsonObject createToolMessage(const ToolCall &toolCall, const QString &result);

private:
    /**
     * @brief 解析流式数据中的JSON对象
     * 
     * @param data 流式数据
     * @return QJsonObject 解析的JSON对象
     */
    QJsonObject parseStreamJson(const QString &data);

    /**
     * @brief 合并多个tool_call片段
     * 
     * @param toolCalls 工具调用片段列表
     * @return QList<ToolCall> 合并后的工具调用列表
     */
    QList<ToolCall> mergeToolCallFragments(const QMap<int, QJsonObject> &toolCallFragments);
};

} // namespace uos_ai

#endif // OAIFUNCTIONPARSER_H 
