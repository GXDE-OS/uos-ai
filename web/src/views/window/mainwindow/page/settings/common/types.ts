import type { McpService } from "@/types/mcp-service";

/**
 * 工具管理项基础接口
 * 用于 MCP 服务和 Skills 的通用列表展示
 *
 * 注意：这是一个前端适配层接口，后端数据需要转换为此格式
 */
export interface ToolManagementItem {
    /** 唯一标识（MCP用id，Skills用name） */
    id: string;
    /** 名称（不翻译） */
    name: string;
    /** 简介/描述（不翻译） */
    description: string;
    /** 是否启用 */
    enabled: boolean;
    /** 是否为内置项 */
    isBuiltIn: boolean;
    /** 是否可编辑 */
    editable: boolean;
    /** 是否可删除 */
    removable: boolean;
}

export type ToolManagementCustomActionResolver<T> = T | ((item: ToolManagementItem) => T);

export interface ToolManagementCustomAction {
    /** 自定义按钮图标 */
    icon: ToolManagementCustomActionResolver<string>;
    /** 自定义按钮点击事件 */
    onClick: (item: ToolManagementItem, event: MouseEvent) => void;
    /** 自定义按钮 tooltip */
    tooltip?: ToolManagementCustomActionResolver<string | undefined>;
    /** 自定义按钮图标尺寸 */
    iconSize?: ToolManagementCustomActionResolver<[number, number]>;
    /** 是否显示自定义按钮 */
    visible?: ToolManagementCustomActionResolver<boolean>;
    /** 是否禁用自定义按钮 */
    disabled?: ToolManagementCustomActionResolver<boolean>;
}

/**
 * 将 MCP 服务数据转换为 ToolManagementItem 格式
 * MCP 服务结构与 ToolManagementItem 几乎一致，直接转换即可
 */
export function convertMcpServiceToToolItem(service: McpService): ToolManagementItem {
    return {
        id: service.id,
        name: service.name,
        description: service.description,
        enabled: service.enabled,
        isBuiltIn: service.isBuiltIn,
        editable: service.editable,
        removable: service.removable,
    };
}

/**
 * 将 Skills 数据转换为 ToolManagementItem 格式的辅助函数
 */
export interface SkillRawItem {
    name: string;
    description: string;
    path: string;
    source: string;
    enabled: boolean;
}

export function convertSkillToToolItem(skill: SkillRawItem): ToolManagementItem {
    const isBuiltInSkill = skill.source === "builtin";
    const isUosAiSkill = skill.source === "uos-ai";

    return {
        id: skill.name,
        name: skill.name,
        description: skill.description,
        enabled: skill.enabled,
        isBuiltIn: isBuiltInSkill,
        editable: !isBuiltInSkill && !isUosAiSkill, // 第三方 skill 支持在文件管理器中打开目录
        removable: isUosAiSkill, // 仅 uos-ai 来源的技能可删除
    };
}
