/**
 * 数据转换器
 * 将原始数据转换为侧边栏标准数据结构
 */

import { h } from "vue";
import SvgIcon from "@/components/SvgIcon";
import { useBackendStore } from "@/stores";
import type { Assistant } from "@/types/assistant";
import { AssistantID, getIconByType } from "@/types/assistant";
import { ConversationStatus, type ConversationIndexWithStatus } from "@/types/conversation";
import {
    ASSISTANT_LIST_APPEND_ITEMS,
    MAX_ASSISTANT_VISIBLE_COUNT,
    normalizeAssistantVisibleCount,
    sortSidebarItemsByOrder,
} from "@/utils/mainwindow/sidebarAssistantOrder";
import type { SidebarGroup, SidebarItem } from "./types";

const moveSidebarItemToIndex = (items: SidebarItem[], itemId: string, targetIndex: number): SidebarItem[] => {
    const itemIndex = items.findIndex((item) => item.id === itemId);
    if (itemIndex < 0) {
        return items;
    }

    // 仅用于生成展示态列表，不直接改变后端保存的助手排序。
    const nextItems = [...items];
    const [targetItem] = nextItems.splice(itemIndex, 1);
    nextItems.splice(Math.max(0, Math.min(targetIndex, nextItems.length)), 0, targetItem);
    return nextItems;
};

/**
 * 将助手列表转换为侧边栏分组
 */
export function convertAssistantListToSidebarGroup(
    assistants: Assistant[],
    options: {
        assistantOrder?: string[];
        assistantVisibleCount?: number;
        temporaryVisibleAssistantId?: string | null;
        selectedAssistantId?: string | null;
    } = {},
): SidebarGroup {
    const rawAssistantItems: SidebarItem[] = assistants
        .filter((assistant) => assistant.id !== AssistantID.UOS_AI)
        .map((assistant) => ({
            id: assistant.id,
            type: "assistant",
            data: assistant,
            icon: getIconByType(assistant, assistant.path?.startsWith("file://") ? "color" : "line"),
            name: assistant.name,
            right: undefined, // 后续可以根据需要添加状态图标
            selected: assistant.id === options.selectedAssistantId,
            reorderDisabled: false,
        }));
    const appendedItems: SidebarItem[] = ASSISTANT_LIST_APPEND_ITEMS.map((item) => ({
        id: item.id,
        type: item.type,
        icon: item.icon,
        name: useBackendStore().translate(item.label),
        selected: false,
        reorderDisabled: false,
        data: item.data,
    }));
    const orderedItems = sortSidebarItemsByOrder([...rawAssistantItems, ...appendedItems], options.assistantOrder ?? []);
    // 常驻数量可能来自历史配置或拖拽结果，这里统一限制到合法范围。
    const persistentVisibleCount = normalizeAssistantVisibleCount(options.assistantVisibleCount, orderedItems.length);
    const temporaryVisibleAssistantIndex = options.temporaryVisibleAssistantId
        ? orderedItems.findIndex((item) => item.id === options.temporaryVisibleAssistantId)
        : -1;
    // 选中的隐藏助手需要临时露出；如果正好在边界位，只增加展示数量即可，无需挪动。
    const shouldTemporarilyExposeAssistant = temporaryVisibleAssistantIndex >= persistentVisibleCount;
    const temporarilyInsertedAssistantId =
        temporaryVisibleAssistantIndex > persistentVisibleCount
            ? options.temporaryVisibleAssistantId
            : null;
    const items = temporarilyInsertedAssistantId
        ? moveSidebarItemToIndex(
              orderedItems,
              temporarilyInsertedAssistantId,
              persistentVisibleCount,
          ).map((item) => ({
              ...item,
              // 临时提前到常驻区后的助手只用于展示，不直接参与拖拽持久化。
              reorderDisabled: item.id === temporarilyInsertedAssistantId,
          }))
        : orderedItems;

    return {
        id: "assistant-list",
        name: useBackendStore().translate("Assistant List"),
        showHeader: false,
        items,
        defaultVisibleCount: shouldTemporarilyExposeAssistant
            ? Math.min(persistentVisibleCount + 1, items.length)
            : persistentVisibleCount,
        persistentVisibleCount,
        maxVisibleCount: MAX_ASSISTANT_VISIBLE_COUNT,
        reorderable: true,
    };
}

export function convertConversationIndexListToSidebarGroup(
    conversationIndexList: ConversationIndexWithStatus[],
    currentConversationId?: string | null,
    assistants?: Assistant[],
): SidebarGroup {
    const items: SidebarItem[] = conversationIndexList.map((conversationIndex) => {
        const assistant = assistants?.find((a) => a.id === conversationIndex.assistant);
        const icon = assistant ? getIconByType(assistant, "color") : "icon_sidebar_conversation";
        return {
            id: conversationIndex.id,
            type: "conversation",
            data: conversationIndex,
            // icon,
            name: conversationIndex.title,
            right: createConversationStatusIndicator(conversationIndex),
            selected: conversationIndex.id === currentConversationId,
            rightButtonIcon:
                conversationIndex.conversationStatus === ConversationStatus.Generating ? undefined : "more",
        };
    });

    return {
        id: "conversation-list",
        name: useBackendStore().translate("Chat History"),
        items,
        defaultVisibleCount: undefined,
        reorderable: false,
        rightButtonIcon: "icon_sidebar_history",
        rightButtonTooltip: useBackendStore().translate("Manage Chat History") || "Manage Chat History",
    };
}

/**
 * 将多个原始数据源转换为侧边栏分组列表
 */
export function convertToSidebarGroups(sources: {
    assistants?: Assistant[];
    assistantOrder?: string[];
    assistantVisibleCount?: number;
    currentConversationId?: string | null;
    conversationIndexes?: ConversationIndexWithStatus[];
    temporaryVisibleAssistantId?: string | null;
    selectedAssistantId?: string | null;
    // 以后可以添加更多数据源
    // tools?: Tool[];
}): SidebarGroup[] {
    const groups: SidebarGroup[] = [];

    if (sources.assistants && sources.assistants.length > 0) {
        groups.push(
            convertAssistantListToSidebarGroup(sources.assistants, {
                assistantOrder: sources.assistantOrder,
                assistantVisibleCount: sources.assistantVisibleCount,
                temporaryVisibleAssistantId: sources.temporaryVisibleAssistantId,
                selectedAssistantId: sources.selectedAssistantId,
            }),
        );
    }

    groups.push(
        convertConversationIndexListToSidebarGroup(
            sources.conversationIndexes || [],
            sources.currentConversationId,
            sources.assistants,
        ),
    );

    return groups;
}

function createConversationStatusIndicator(
    conversationIndex: ConversationIndexWithStatus,
): SidebarItem["right"] | undefined {
    // 生成中显示旋转 loading，未读显示红点，已读则不渲染任何右侧标识。
    if (conversationIndex.conversationStatus === ConversationStatus.Generating) {
        return () =>
            h("span", { class: "window-sidebar__conversation-status window-sidebar__conversation-status--loading" }, [
                h(SvgIcon, {
                    class: "window-sidebar__conversation-loading-icon",
                    icon: "loading",
                    size: [16, 16],
                    "aria-hidden": "true",
                }),
            ]);
    }

    if (conversationIndex.conversationStatus === ConversationStatus.Unread) {
        return () =>
            h("span", {
                class: "window-sidebar__conversation-status window-sidebar__conversation-status--unread",
            });
    }

    return undefined;
}
