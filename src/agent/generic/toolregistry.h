#ifndef TOOLREGISTRY_H
#define TOOLREGISTRY_H

#include "model/modeltool.h"
#include <QList>

namespace uos_ai {

/**
 * @brief 工具注册表类，负责管理所有工具的定义
 */
class ToolRegistry
{
public:
    /**
     * @brief 获取所有工具定义
     * @return 工具列表
     */
    static QList<ModelTool> getAllTools();

    /**
     * @brief 注册系统控制相关工具
     * @param tools 工具列表
     */
    static void registerSystemTools(QList<ModelTool> &tools);

    /**
     * @brief 注册文件操作相关工具
     * @param tools 工具列表
     */
    static void registerFileTools(QList<ModelTool> &tools);

    /**
     * @brief 注册媒体控制相关工具
     * @param tools 工具列表
     */
    static void registerMediaTools(QList<ModelTool> &tools);

    /**
     * @brief 注册邮件和日程相关工具
     * @param tools 工具列表
     */
    static void registerCommunicationTools(QList<ModelTool> &tools);

    /**
     * @brief 注册应用商店相关工具
     * @param tools 工具列表
     */
    static void registerStoreTools(QList<ModelTool> &tools);
};

} // namespace uos_ai

#endif // TOOLREGISTRY_H
