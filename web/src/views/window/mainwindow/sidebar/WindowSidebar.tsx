import { defineComponent, computed, ref, onMounted, onUnmounted, nextTick, watch } from "vue";
import {
    useAssistantInfosStore,
    useModelInfosStore,
    useExtensionPanelStore,
    useConversationManagerStore,
    useMainWindowStore,
    useBackendStore,
    useMcpServicesStore,
    useNotifyStore,
    useHistoryConversationStore,
    useReportChannelStore,
} from "@/stores";
import type { Assistant } from "@/types/assistant";
import { AssistantID } from "@/types/assistant";
import type { ConversationIndex } from "@/types/conversation";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import type {
    SidebarGroup,
    SidebarItem,
    SidebarHandlers,
    SidebarGroupHeaderClickHandlers,
} from "@/views/window/mainwindow/sidebar/types";
import { createDefaultConversation, createTemporaryConversation } from "@/utils/mainwindow/conversationActions";
import { createId } from "@/utils/date";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";
import ScrollBar from "@/components/ScrollBar";
import ItemList from "@/views/window/mainwindow/sidebar/components/ItemList";
import BaseItem from "@/views/window/mainwindow/sidebar/components/BaseItem";
import GroupHeader from "@/views/window/mainwindow/sidebar/components/GroupHeader";
import { convertToSidebarGroups } from "@/views/window/mainwindow/sidebar/converters";
import {
    getAssistantSidebarItemIds,
    normalizeAssistantVisibleCount,
    sortSidebarItemIdsByOrder,
} from "@/utils/mainwindow/sidebarAssistantOrder";
import Menu from "@/components/menu/Menu";
import type { MenuItem } from "@/types/menu";
import { ReportEventType } from "@/types/report";

// 侧边栏需要直接访问 ScrollBar 暴露出来的原生滚动容器，
// 以便在外部文件拖入期间临时关闭纵向滚动，阻止底部自动滚动行为。
type ScrollBarInstance = InstanceType<typeof ScrollBar>;

const getScrollContainer = (scrollBar: ScrollBarInstance | null | undefined) => {
    const containerRef = scrollBar?.scrollContainerRef as
        | HTMLElement
        | { value: HTMLElement | null }
        | null
        | undefined;

    if (!containerRef) {
        return null;
    }

    return containerRef instanceof HTMLElement ? containerRef : containerRef.value;
};

export default defineComponent({
    name: "WindowSidebar",
    components: {
        IconButton,
        ScrollBar,
        ItemList,
        BaseItem,
        GroupHeader,
    },
    setup() {
        const assistantInfos = useAssistantInfosStore();
        const modelInfosStore = useModelInfosStore();
        const extensionPanelStore = useExtensionPanelStore();
        const conversationManagerStore = useConversationManagerStore();
        const mainWindowStore = useMainWindowStore();
        const backendStore = useBackendStore();
        const mcpServicesStore = useMcpServicesStore();
        const notifyStore = useNotifyStore();
        const historyConversationStore = useHistoryConversationStore();
        const reportChannelStore = useReportChannelStore();

        // 滚动条相关状态
        const scrollTop = ref(0);
        const stickyGroups = ref<SidebarGroup[]>([]);
        const stickyStackHeight = ref(0);
        // 保存侧边栏 ScrollBar 实例，用来切换原生滚动容器的 overflow 状态。
        const sidebarScrollBarRef = ref<ScrollBarInstance | null>(null);
        // dragenter / dragleave 在子节点间移动时会重复触发，用深度计数避免过早解锁。
        const sidebarFileDragDepth = ref(0);
        // 标记当前是否正在屏蔽侧边栏文件拖拽，供后续 dragleave / drop 阶段复用。
        const isBlockingSidebarFileDrag = ref(false);

        // 菜单相关状态
        const menuTriggerRef = ref<HTMLElement | null>(null);
        const menuItems = ref<MenuItem[]>([]);
        const temporaryVisibleAssistantId = ref<string | null>(null);
        // 助手切换需要等待模型列表加载；等待期间先用 pending id 保持侧边栏选中态即时反馈。
        const pendingSelectedAssistantId = ref<string | null>(null);
        // 用版本号丢弃较早的异步切换结果，避免快速连点时旧请求覆盖新选择。
        let assistantSelectionVersion = 0;

        // 新建对话项（常驻置顶）
        const newItem = computed<SidebarItem>(() => ({
            id: "new-conversation",
            type: "new-conversation",
            name: backendStore.translate("New Chat"),
            icon: mainWindowStore.isDarkMode ? "icon_new_conversation_dark" : "icon_new_conversation",
            data: null,
        }));
        const isStickyGroupHeadersEnabled = computed(() => backendStore.isEnableAdvancedCssFeatures);

        // 原始数据
        const assistantList = computed(() => assistantInfos.getAssistantList);
        const currentConversationId = computed(() => conversationManagerStore.getCurrentConversationId);
        const currentConversationRecord = computed(() => conversationManagerStore.getCurrentConversationRecord);
        // 侧边栏直接消费带状态的会话索引，避免在渲染层再二次拼装状态。
        const conversationIndexList = computed(() => conversationManagerStore.getConversationIndexListWithStatus);
        const assistantOrder = computed(() => assistantInfos.getAssistantOrder);
        const assistantSidebarItemIds = computed(() =>
            sortSidebarItemIdsByOrder(getAssistantSidebarItemIds(assistantList.value), assistantOrder.value),
        );
        // 常驻数量来自后端配置，但渲染前仍做一次归一化，兜住异常值。
        const assistantVisibleCount = computed(() =>
            normalizeAssistantVisibleCount(assistantInfos.getAssistantVisibleCount),
        );
        const getAssistantSidebarItemIndex = (assistantId: string) =>
            assistantSidebarItemIds.value.findIndex((id) => id === assistantId);
        const selectedAssistantId = computed(() => {
            const conversationRecord = currentConversationRecord.value;
            if (!conversationRecord?.shouldShowInSidebar) {
                return null;
            }

            if (Object.keys(conversationRecord.messages).length > 0) {
                return null;
            }

            const assistantId = conversationRecord.root?.assistant;
            if (!assistantId || assistantId === AssistantID.UOS_AI) {
                return null;
            }

            return assistantId;
        });
        const effectiveSelectedAssistantId = computed(
            () => pendingSelectedAssistantId.value ?? selectedAssistantId.value,
        );
        // 判断助手是否位于折叠区，用于选中隐藏助手时把它临时露出到常驻区后方。
        const isAssistantInExpandedItems = (assistantId: string) =>
            getAssistantSidebarItemIndex(assistantId) >= assistantVisibleCount.value;
        const isAssistantTemporarilyInserted = (assistantId: string) =>
            getAssistantSidebarItemIndex(assistantId) > assistantVisibleCount.value;
        const effectiveTemporaryVisibleAssistantId = computed(() => {
            const assistantId = temporaryVisibleAssistantId.value;
            if (!assistantId || !isAssistantInExpandedItems(assistantId)) {
                return null;
            }

            return assistantId;
        });
        const effectiveTemporarilyInsertedAssistantId = computed(() => {
            const assistantId = effectiveTemporaryVisibleAssistantId.value;
            if (!assistantId || !isAssistantTemporarilyInserted(assistantId)) {
                return null;
            }

            return assistantId;
        });

        watch(
            () => ({
                assistantId: effectiveSelectedAssistantId.value,
                itemIds: assistantSidebarItemIds.value,
                visibleCount: assistantVisibleCount.value,
            }),
            ({ assistantId }) => {
                // 选中项、排序或常驻数量变化时重新判断临时露出项是否仍然有效。
                temporaryVisibleAssistantId.value =
                    assistantId && isAssistantInExpandedItems(assistantId) ? assistantId : null;
            },
            { immediate: true },
        );

        // 将原始数据转换为标准化侧边栏数据
        const groups = computed((): SidebarGroup[] => {
            if (assistantList.value.length === 0 && conversationIndexList.value.length === 0) {
                return [];
            }
            return convertToSidebarGroups({
                assistants: assistantList.value,
                assistantOrder: assistantOrder.value,
                assistantVisibleCount: assistantVisibleCount.value,
                currentConversationId: currentConversationId.value,
                conversationIndexes: conversationIndexList.value,
                temporaryVisibleAssistantId: effectiveTemporaryVisibleAssistantId.value,
                selectedAssistantId: effectiveSelectedAssistantId.value,
                shouldHighlightCurrentItem: mainWindowStore.workspacePage === MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
                groupCollapsedStates: mainWindowStore.sidebarGroupCollapsedStates,
            });
        });

        const switchToChatPageIfNeeded = async () => {
            if (mainWindowStore.workspacePage !== MAIN_WINDOW_WORKSPACE_PAGES.CHAT) {
                await mainWindowStore.openChatPage();
            }
        };

        // 定义点击处理器映射
        const handlers: SidebarHandlers = {
            "new-conversation": async () => {
                await createDefaultConversation();
            },
            "agent-store": async (item: SidebarItem) => {
                try {
                    await backendStore.requestSystem("openAppStoreTab", item.data.storeType);
                } catch (error) {
                    console.error("Failed to open agent store:", error);
                }
            },
            assistant: async (item: SidebarItem) => {
                const assistant = item.data as Assistant;
                const selectionVersion = ++assistantSelectionVersion;
                try {
                    pendingSelectedAssistantId.value = assistant.id;
                    temporaryVisibleAssistantId.value = isAssistantInExpandedItems(assistant.id) ? assistant.id : null;

                    await modelInfosStore.loadModelList(assistant.id);
                    // 会话管理器创建对话
                    conversationManagerStore.createConversation(
                        createId(), // 初始化会话ID，使用当前时间戳
                        assistant.id,
                        modelInfosStore.getCurrentModel?.id || "", // TODO: 暂时使用当前模型
                    );
                    assistantInfos.setCurrentAssistant(assistant);

                    if (selectionVersion !== assistantSelectionVersion) {
                        // 有更新的助手切换已经开始，当前异步结果直接丢弃。
                        return;
                    }
                    extensionPanelStore.closeExtensionPanel();
                    await switchToChatPageIfNeeded();
                } catch (error) {
                    console.error("Failed to set current assistant:", error);
                } finally {
                    if (selectionVersion === assistantSelectionVersion) {
                        pendingSelectedAssistantId.value = null;
                    }
                }
            },
            // 以后可以添加其他类型的处理器
            conversation: async (item: SidebarItem) => {
                const conversationIndex = item.data as ConversationIndex;
                try {
                    await switchToChatPageIfNeeded();
                    extensionPanelStore.closeExtensionPanel();
                    await conversationManagerStore.switchConversation(conversationIndex.id);
                } catch (error) {
                    console.error("Failed to set current conversation:", error);
                }
            },
            // tool: async (item: SidebarItem) => { ... },
        };

        // 处理项点击
        const handleItemClick = async (item: SidebarItem) => {
            const handler = handlers[item.type];
            if (handler) {
                await handler(item);
            } else {
                console.warn(`No handler defined for item type: ${item.type}`);
            }
        };

        // 处理排序
        const handleReorder = (payload: { groupId: string; newItems: string[]; visibleCount?: number }) => {
            if (payload.groupId === "assistant-list") {
                // 拖拽后的顺序和跨区产生的常驻数量变化都需要同步给后端持久化。
                void assistantInfos.reorderAssistants(
                    payload.newItems,
                    effectiveTemporarilyInsertedAssistantId.value,
                    payload.visibleCount,
                );
            }
        };

        const groupHeaderClickHandlers: SidebarGroupHeaderClickHandlers = {
            "conversation-list": async () => {
                historyConversationStore.setOpenedMenuId(null);
                await openHistoryConversationPage();
            },
        };

        const handleGroupHeaderClick = async (params: Record<string, any>) => {
            const handler = groupHeaderClickHandlers[params.groupId];
            if (handler) {
                await handler(params);
            } else {
                console.warn(`No handler defined for groupId: ${params.groupId}`);
            }
        };

        // 当前选中的会话项（用于菜单操作）
        const selectedMenuItem = ref<SidebarItem | null>(null);

        const sidebarMenuId = computed(() => (selectedMenuItem.value ? `sidebar:${selectedMenuItem.value.id}` : null));
        const lastOpenedMenuId = ref<string | null>(null);

        const isMenuVisible = computed(
            () => historyConversationStore.openedMenuId === sidebarMenuId.value && sidebarMenuId.value,
        );

        // 处理会话项的更多按钮点击
        const handleItemRightButtonClick = async (params: { item: SidebarItem; event: MouseEvent }) => {
            const { item, event } = params;
            if (item.type === "conversation") {
                // 如果当前已有菜单打开，先关闭它，等待下一帧后再打开新菜单
                // 这样可以确保 Menu 组件重新定位到新位置
                if (isMenuVisible.value) {
                    historyConversationStore.setOpenedMenuId(null);
                    await nextTick();
                }

                selectedMenuItem.value = item;
                menuTriggerRef.value = event.currentTarget as HTMLElement;
                menuItems.value = [
                    {
                        type: "item",
                        id: "delete",
                        label: backendStore.translate("Delete"),
                        icon: "trash",
                    },
                ];
                await nextTick();
                if (lastOpenedMenuId.value !== sidebarMenuId.value) {
                    lastOpenedMenuId.value = sidebarMenuId.value;
                    historyConversationStore.setOpenedMenuId(`sidebar:${item.id}`);
                } else {
                    lastOpenedMenuId.value = null;
                    historyConversationStore.setOpenedMenuId(null);
                }
            }
        };

        // 处理菜单项选择
        const handleMenuSelect = (menuItem: MenuItem) => {
            if (menuItem.type !== "item") {
                return;
            }

            switch (menuItem.id) {
                case "delete":
                    if (selectedMenuItem.value) {
                        console.log("Delete conversation:", selectedMenuItem.value.id, selectedMenuItem.value.name);
                        void notifyStore
                            .showDialog({
                                title: backendStore.translate("Confirm delete this conversation"),
                                content: `${backendStore.translate("This will remove all related content from UOS AI")}`,
                                buttons: [
                                    { key: "cancel", text: backendStore.translate("Cancel"), type: "default" },
                                    {
                                        key: "confirm",
                                        text: backendStore.translate("Confirm Delete"),
                                        type: "danger",
                                    },
                                ],
                            })
                            .then(async (result) => {
                                if (result.key === "confirm") {
                                    await historyConversationStore.deleteConversations(
                                        selectedMenuItem.value!.id as string,
                                    );
                                }
                            });
                    }
                    break;
                default:
                    break;
            }
        };

        const openHistoryConversationPage = async () => {
            if (mainWindowStore.workspacePage !== MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION) {
                await mainWindowStore.openHistoryConversationPage();
            }
        };

        const handleSidebarScroll = (nextScrollTop: number) => {
            scrollTop.value = nextScrollTop;
            // 滚动时隐藏菜单
            if (isMenuVisible.value) {
                historyConversationStore.setOpenedMenuId(null);
            }
        };

        const resetStickyState = () => {
            stickyGroups.value = [];
            stickyStackHeight.value = 0;
        };

        const handleStickyChange = (payload: { groups: SidebarGroup[]; stackHeight: number }) => {
            if (!isStickyGroupHeadersEnabled.value) {
                resetStickyState();
                return;
            }

            stickyGroups.value = payload.groups;
            stickyStackHeight.value = payload.stackHeight;
        };

        const handleGroupCollapse = (payload: { groupId: string; collapsed: boolean }) => {
            mainWindowStore.setSidebarGroupCollapsed(payload.groupId, payload.collapsed);
        };

        const handleMenuVisibleChange = (visible: boolean) => {
            if (visible && selectedMenuItem.value) {
                historyConversationStore.setOpenedMenuId(`sidebar:${selectedMenuItem.value.id}`);
            } else {
                historyConversationStore.setOpenedMenuId(null);
            }
        };

        const handleNewItemClick = () => {
            handleItemClick(newItem.value);
        };

        const handleCreateTemporaryConversation = async (event: MouseEvent) => {
            event.stopPropagation();
            reportChannelStore.writeReportEvent([{ type: ReportEventType.PrivateChatClickedPoint }]);
            await createTemporaryConversation();
        };

        const listShellStyle = computed(
            () =>
                ({
                    "--window-sidebar-sticky-stack-height": isStickyGroupHeadersEnabled.value
                        ? `${stickyStackHeight.value + 16}px`
                        : "0px",
                }) as Record<string, string>,
        );

        // 只拦截系统文件拖拽，不影响侧边栏内部的普通鼠标拖拽排序。
        const isSidebarFileDragEvent = (event: DragEvent) => {
            if (isBlockingSidebarFileDrag.value) {
                return true;
            }

            const types = Array.from(event.dataTransfer?.types || []);
            return types.includes("Files");
        };

        // 文件悬停在侧边栏期间关闭原生纵向滚动，避免滚动容器触发自动下滚。
        const setSidebarFileDragLock = (locked: boolean) => {
            isBlockingSidebarFileDrag.value = locked;

            const scrollContainer = getScrollContainer(sidebarScrollBarRef.value);
            if (!scrollContainer) {
                return;
            }

            scrollContainer.style.overflowY = locked ? "hidden" : "";
        };

        // 统一重置拖拽状态，保证 drop / dragend / unmount 后都能恢复正常滚动。
        const resetSidebarFileDragState = () => {
            sidebarFileDragDepth.value = 0;
            setSidebarFileDragLock(false);
        };

        // 进入侧边栏时立即接管文件拖拽事件，防止事件继续驱动滚动容器自动滚动。
        const handleSidebarFileDragEnter = (event: DragEvent) => {
            if (!isSidebarFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            sidebarFileDragDepth.value += 1;
            if (event.dataTransfer) {
                event.dataTransfer.dropEffect = "none";
            }
            setSidebarFileDragLock(true);
        };

        // 持续悬停时保持滚动锁，并显式告诉系统这里不是文件可投放目标。
        const handleSidebarFileDragOver = (event: DragEvent) => {
            if (!isSidebarFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            if (event.dataTransfer) {
                event.dataTransfer.dropEffect = "none";
            }
            setSidebarFileDragLock(true);
        };

        // 离开侧边栏后再恢复滚动；使用深度计数避免在子元素间穿梭时误解锁。
        const handleSidebarFileDragLeave = (event: DragEvent) => {
            if (!isSidebarFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            sidebarFileDragDepth.value = Math.max(0, sidebarFileDragDepth.value - 1);
            if (sidebarFileDragDepth.value === 0) {
                resetSidebarFileDragState();
            }
        };

        // 文件落到侧边栏时直接吞掉事件并清理状态，避免产生副作用。
        const handleSidebarFileDrop = (event: DragEvent) => {
            if (!isSidebarFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            resetSidebarFileDragState();
        };

        // 全局兜底：如果拖拽在窗口其他区域结束，也要把侧边栏滚动锁恢复回来。
        onMounted(() => {
            window.addEventListener("drop", resetSidebarFileDragState);
            window.addEventListener("dragend", resetSidebarFileDragState);
        });

        onUnmounted(() => {
            window.removeEventListener("drop", resetSidebarFileDragState);
            window.removeEventListener("dragend", resetSidebarFileDragState);
            resetSidebarFileDragState();
        });

        watch(isStickyGroupHeadersEnabled, (enabled) => {
            if (!enabled) {
                resetStickyState();
            }
        });

        return {
            groups,
            handleItemClick,
            handleNewItemClick,
            handleReorder,
            handleGroupHeaderClick,
            handleItemRightButtonClick,
            handleSidebarScroll,
            handleStickyChange,
            handleGroupCollapse,
            listShellStyle,
            newItem,
            handleCreateTemporaryConversation,
            sidebarScrollBarRef,
            handleSidebarFileDragEnter,
            handleSidebarFileDragOver,
            handleSidebarFileDragLeave,
            handleSidebarFileDrop,
            scrollTop,
            stickyGroups,
            stickyStackHeight,
            isStickyGroupHeadersEnabled,
            isSidebarCollapsed: computed(() => mainWindowStore.isSidebarCollapsed),
            // 菜单相关
            isMenuVisible,
            menuTriggerRef,
            menuItems,
            handleMenuVisibleChange,
            handleMenuSelect,
        };
    },
    render() {
        return (
            <div
                class={["window-sidebar", this.isSidebarCollapsed && "window-sidebar--collapsed"]}
                onDragenter={this.handleSidebarFileDragEnter}
                onDragover={this.handleSidebarFileDragOver}
                onDragleave={this.handleSidebarFileDragLeave}
                onDrop={this.handleSidebarFileDrop}
            >
                <div class="window-sidebar__content">
                    <div class="window-sidebar__new-item">
                        <BaseItem
                            class="window-sidebar__new-item-entry"
                            item={this.newItem}
                            onClick={this.handleNewItemClick}
                        />
                        <IconButton
                            class="window-sidebar__temp-conversation-button"
                            icon="icon_temp_conversation"
                            tooltip={useBackendStore().translate("Temporary Chat")}
                            iconSize={[16, 16]}
                            size={30}
                            shape={ButtonShape.Circle}
                            border={true}
                            onClick={this.handleCreateTemporaryConversation}
                        />
                    </div>
                    <div
                        class={[
                            "window-sidebar__list-shell",
                            this.isStickyGroupHeadersEnabled && "window-sidebar__list-shell--sticky-enabled",
                        ]}
                        style={this.listShellStyle}
                    >
                        {this.isStickyGroupHeadersEnabled && (
                            <div class="window-sidebar__sticky-stack">
                                {this.stickyGroups.map((group) => (
                                    <GroupHeader
                                        key={`sticky-${group.id}`}
                                        groupName={group.name}
                                        groupId={group.id}
                                        headerDomId={`stacked-group-header-${group.id}`}
                                        tooltip={group.tooltip}
                                        collapsible={group.collapsible}
                                        collapsed={group.collapsed}
                                        onClick={this.handleGroupHeaderClick}
                                        onCollapse={() =>
                                            this.handleGroupCollapse({
                                                groupId: group.id,
                                                collapsed: !group.collapsed,
                                            })
                                        }
                                    />
                                ))}
                            </div>
                        )}
                        <ScrollBar
                            ref="sidebarScrollBarRef"
                            class="window-sidebar__scroll"
                            edgeBounce
                            momentum
                            maxScrollOffset={this.isStickyGroupHeadersEnabled ? this.stickyStackHeight : 0}
                            onScroll={this.handleSidebarScroll}
                        >
                            <ItemList
                                groups={this.groups}
                                scrollTop={this.scrollTop}
                                stickyGroupHeadersEnabled={this.isStickyGroupHeadersEnabled}
                                onItem-click={this.handleItemClick}
                                onReorder={this.handleReorder}
                                onGroupHeaderClick={this.handleGroupHeaderClick}
                                onStickyChange={this.handleStickyChange}
                                onRightButtonClick={this.handleItemRightButtonClick}
                                onCollapse={this.handleGroupCollapse}
                            />
                        </ScrollBar>
                    </div>
                </div>
                <Menu
                    items={this.menuItems}
                    visible={this.isMenuVisible}
                    triggerRef={this.menuTriggerRef}
                    placement="bottom"
                    onUpdateVisible={this.handleMenuVisibleChange}
                    onSelectItem={this.handleMenuSelect}
                />
            </div>
        );
    },
});
