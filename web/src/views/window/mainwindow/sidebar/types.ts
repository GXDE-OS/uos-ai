import type { VNode } from "vue";

/**
 * 侧边栏项 - 标准化数据结构
 */
export interface SidebarItem {
    /** 唯一标识 */
    id: string;
    /** SVG 图标名称（不包含 #icon- 前缀）或 file:// 图标路径 */
    icon?: string;
    /** 名称 */
    name: string;
    /** 右侧内容：可以是字符串或渲染函数（放置按钮、状态标志、示意图标等） */
    right?: string | (() => VNode);
    /** 是否选中 */
    selected?: boolean;
    /** 是否禁止排序：用于保护临时展示项，避免把展示态直接持久化 */
    reorderDisabled?: boolean;
    /** 数据标签，用于点击处理器分发 */
    type: string;
    /** 携带的原始数据 */
    data?: any;
    /** 右侧按钮图标 */
    rightButtonIcon?: string;
    /** 右侧按钮提示 */
    rightButtonTooltip?: string;
    /** 是否显示右侧按钮 */
    showRightButton?: boolean;
}

/**
 * 侧边栏分组 - 标准化数据结构
 */
export interface SidebarGroup {
    /** 分组唯一标识 */
    id: string;
    /** 分组名称 */
    name: string;
    /** 分组头部提示 */
    tooltip?: string;
    /** 是否显示分组头部，默认显示 */
    showHeader?: boolean;
    /** 分组内的项列表 */
    items: SidebarItem[];
    /** 默认显示数量（可选，默认全显示） */
    defaultVisibleCount?: number;
    /** 可持久化的常驻显示数量；未设置时与 defaultVisibleCount 一致 */
    persistentVisibleCount?: number;
    /** 常驻显示数量上限 */
    maxVisibleCount?: number;
    /** 是否允许组内排序 */
    reorderable?: boolean;
    /** 是否显示右侧按钮 */
    rightButtonIcon?: string;
    /** 右侧按钮提示 */
    rightButtonTooltip?: string;
    /** 是否可折叠，默认 false */
    collapsible?: boolean;
    /** 是否已折叠 */
    collapsed?: boolean;
}

/**
 * 侧边栏点击处理器映射
 */
export type SidebarHandlers = Partial<{
    [type: string]: (item: SidebarItem) => void | Promise<void>;
}>;

/**
 * 侧边栏header的点击处理器映射
 */
export type SidebarGroupHeaderClickHandlers = Partial<{
    [type: string]: (params: Record<string, any>) => void | Promise<void>;
}>;
