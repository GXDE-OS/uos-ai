import type { ConversationIndex } from "./conversation";

// 批量管理状态类型
export interface BatchOperateState {
    isBatchMode: boolean; // 是否开启批量模式
    selectedIds: string[]; // 选中的会话ID
}

// 时间分组类型
export type TimeGroupType =
    | "today"
    | "yesterday"
    | "past7"
    | "past30"
    | "month1"
    | "month2"
    | "month3"
    | "month4"
    | "month5"
    | "month6"
    | "month7"
    | "month8"
    | "month9"
    | "month10"
    | "month11"
    | "year"
    | "earlier";

export interface GroupedConversations {
    type: TimeGroupType;
    label: string; // 分组展示文案（如2026年、3月）
    list: ConversationIndex[];
}

// 智能体筛选标签类型
export interface AssistantFilterTag {
    id: string;
    name: string;
    isSelected: boolean;
}

// 历史会话操作菜单
export enum HistoryConversationOptionMenu {
    Delete = "delete",
}
