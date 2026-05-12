import type { Assistant } from "@/types/assistant";
import { AssistantID } from "@/types/assistant";

export const DEFAULT_ASSISTANT_VISIBLE_COUNT = 4;
// 目前产品固定最多展示 4 个常驻入口；拖拽只能减少/恢复，不能扩展上限。
export const MAX_ASSISTANT_VISIBLE_COUNT = DEFAULT_ASSISTANT_VISIBLE_COUNT;

export const ASSISTANT_LIST_APPEND_ITEMS = [
    {
        id: "agent-store",
        type: "agent-store",
        icon: "icon_sidebar_agent_store",
        label: "Agent Store",
        data: {
            storeType: "topAiAgentApp",
        },
    },
] as const;

type SidebarOrderableItem = {
    id: string;
};

export const normalizeAssistantVisibleCount = (
    visibleCount: unknown,
    itemCount = Number.POSITIVE_INFINITY,
): number => {
    // 后端配置、旧版本数据或拖拽结果都通过这里收口，保证渲染层只拿到整数范围。
    const normalizedCount =
        typeof visibleCount === "number" && Number.isFinite(visibleCount)
            ? Math.trunc(visibleCount)
            : DEFAULT_ASSISTANT_VISIBLE_COUNT;

    return Math.max(0, Math.min(normalizedCount, MAX_ASSISTANT_VISIBLE_COUNT, itemCount));
};

const buildOrderMap = (order: string[]) => {
    const orderMap = new Map<string, number>();

    for (const [index, id] of order.entries()) {
        if (!orderMap.has(id)) {
            orderMap.set(id, index);
        }
    }

    return orderMap;
};

export const getAssistantSidebarAssistantIds = (assistants: Assistant[]): string[] =>
    assistants.filter((assistant) => assistant.id !== AssistantID.UOS_AI).map((assistant) => assistant.id);

export const getAssistantSidebarItemIds = (assistants: Assistant[]): string[] => [
    ...getAssistantSidebarAssistantIds(assistants),
    ...ASSISTANT_LIST_APPEND_ITEMS.map((item) => item.id),
];

export const sortSidebarItemsByOrder = <T extends SidebarOrderableItem>(items: T[], order: string[]): T[] => {
    const orderMap = buildOrderMap(order);
    const originIndexMap = new Map(items.map((item, index) => [item.id, index]));

    return [...items].sort((a, b) => {
        const aIndex = orderMap.get(a.id);
        const bIndex = orderMap.get(b.id);

        if (aIndex === undefined && bIndex === undefined) {
            return (originIndexMap.get(a.id) ?? 0) - (originIndexMap.get(b.id) ?? 0);
        }

        if (aIndex === undefined) {
            return 1;
        }

        if (bIndex === undefined) {
            return -1;
        }

        return aIndex - bIndex;
    });
};

export const sortSidebarItemIdsByOrder = (itemIds: string[], order: string[]): string[] =>
    sortSidebarItemsByOrder(itemIds.map((id) => ({ id })), order).map((item) => item.id);
