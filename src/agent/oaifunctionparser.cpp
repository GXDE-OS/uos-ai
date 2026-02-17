#include "oaifunctionparser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>
#include <QMap>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

OAIFunctionParser::OAIFunctionParser()
{
}

QList<OAIFunctionParser::ToolCall> OAIFunctionParser::parseResponse(const QJsonObject &response)
{    
    QList<ToolCall> result;
    // 检查是否包含tools字段
    if (!response.contains("tools")) {
        return result;
    }
    
    QJsonObject tools = response["tools"].toObject();
    
    // 解析function_call（旧格式）
    if (tools.contains("function_call")) {
        QJsonObject functionCall = tools["function_call"].toObject();
        ToolCall toolCall = parseFunctionCall(functionCall);
        result.append(toolCall);
        qCDebug(logAgent) << "Parsed function_call:" << toolCall.name;
    }
    
    // 解析tool_calls（新格式）
    if (tools.contains("tool_calls")) {
        QJsonArray toolCalls = tools["tool_calls"].toArray();
        for (const QJsonValue &toolCallValue : toolCalls) {
            QJsonObject toolCallObj = toolCallValue.toObject();
            ToolCall toolCall = parseToolCall(toolCallObj);
            result.append(toolCall);
            qCDebug(logAgent) << "Parsed tool_call:" << toolCall.name << "index:" << toolCall.index;
        }
    }
    
    return result;
}

OAIFunctionParser::ToolCall OAIFunctionParser::parseFunctionCall(const QJsonObject &functionCall)
{
    ToolCall toolCall;
    
    toolCall.name = functionCall["name"].toString();
    toolCall.arguments = parseArguments(functionCall["arguments"].toString());
    
    qCDebug(logAgent) << "Parsed function_call - name:" << toolCall.name 
                      << "arguments:" << QJsonDocument(toolCall.arguments).toJson();
    
    return toolCall;
}

OAIFunctionParser::ToolCall OAIFunctionParser::parseToolCall(const QJsonObject &toolCall)
{
    ToolCall result;
    
    result.id = toolCall["id"].toString();
    result.index = toolCall["index"].toInt();
    result.type = toolCall["type"].toString();
    
    if (toolCall.contains("function")) {
        QJsonObject function = toolCall["function"].toObject();
        result.name = function["name"].toString();
        result.arguments = parseArguments(function["arguments"].toString());
    }
    
    qCDebug(logAgent) << "Parsed tool_call - id:" << result.id 
                      << "name:" << result.name 
                      << "index:" << result.index
                      << "arguments:" << QJsonDocument(result.arguments).toJson();
    
    return result;
}

QJsonObject OAIFunctionParser::parseArguments(const QString &argumentsJson)
{
    QJsonDocument doc = QJsonDocument::fromJson(argumentsJson.toUtf8());
    if (doc.isNull()) {
        qCWarning(logAgent) << "Failed to parse arguments JSON:" << argumentsJson;
        return QJsonObject();
    }
    
    if (!doc.isObject()) {
        qCWarning(logAgent) << "Arguments is not a JSON object:" << argumentsJson;
        return QJsonObject();
    }
    
    return doc.object();
}

bool OAIFunctionParser::hasToolCalls(const QJsonObject &response)
{
    if (!response.contains("tools")) {
        return false;
    }

    QJsonObject tools = response["tools"].toObject();
    return tools.contains("function_call") || tools.contains("tool_calls");
}

QJsonArray OAIFunctionParser::buildChatMessages(const QList<ToolCall> &tools)
{
    QJsonArray chatMessages;
    
    if (tools.isEmpty()) {
        return chatMessages;
    }
    
    // 创建assistant消息
    QJsonObject assistantMsg = createAssistantMessage(tools);
    chatMessages.append(assistantMsg);
    
    return chatMessages;
}

QJsonObject OAIFunctionParser::createAssistantMessage(const QList<ToolCall> &tools)
{
    QJsonObject assistantMsg;
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = "";
    
    // 创建tool_calls数组
    QJsonArray toolCallsArray;
    for (const auto &toolCall : tools) {
        QJsonObject toolCallObj;
        toolCallObj["id"] = toolCall.id;
        toolCallObj["type"] = toolCall.type;
        toolCallObj["index"] = toolCall.index;
        
        QJsonObject functionObj;
        functionObj["name"] = toolCall.name;
        functionObj["arguments"] = QString::fromUtf8(QJsonDocument(toolCall.arguments).toJson(QJsonDocument::Compact));
        toolCallObj["function"] = functionObj;
        
        toolCallsArray.append(toolCallObj);
    }
    
    assistantMsg["tool_calls"] = toolCallsArray;
    
    return assistantMsg;
}

QJsonObject OAIFunctionParser::createToolMessage(const ToolCall &toolCall, const QString &result)
{
    QJsonObject toolMsg;
    toolMsg["role"] = "tool";
    toolMsg["tool_call_id"] = toolCall.id;
    toolMsg["content"] = result;
    
    return toolMsg;
}

} // namespace uos_ai 
