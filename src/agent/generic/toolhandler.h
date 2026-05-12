#ifndef TOOLHANDLER_H
#define TOOLHANDLER_H

#include <QJsonObject>
#include <QString>
#include "toolresult.h"

namespace uos_ai {

/**
 * @brief 工具处理器类，负责处理所有工具的调用逻辑
 */
class ToolHandler
{
public:
    /**
     * @brief 调用指定的工具
     * @param toolName 工具名称
     * @param params 工具参数
     * @return 调用结果，包含错误码、消息和工具调用详细信息
     */
    static ToolCallResult callTool(const QString &toolName, const QJsonObject &params);

private:
    /**
     * @brief 处理系统控制相关工具
     */
    static ToolCallResult handleSystemTool(const QString &toolName, const QJsonObject &params);

    /**
     * @brief 处理文件操作相关工具
     */
    static ToolCallResult handleFileTool(const QString &toolName, const QJsonObject &params);

    /**
     * @brief 处理媒体控制相关工具
     */
    static ToolCallResult handleMediaTool(const QString &toolName, const QJsonObject &params);

    /**
     * @brief 处理通信相关工具
     */
    static ToolCallResult handleCommunicationTool(const QString &toolName, const QJsonObject &params);

    /**
     * @brief 处理应用商店相关工具
     */
    static ToolCallResult handleStoreTool(const QString &toolName, const QJsonObject &params);
};

} // namespace uos_ai

#endif // TOOLHANDLER_H
