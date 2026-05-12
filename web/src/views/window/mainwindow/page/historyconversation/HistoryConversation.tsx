import { defineComponent, ref, computed, watchEffect, nextTick } from "vue";
import { ElInput } from "element-plus";
import SvgIcon from "@/components/SvgIcon";
import IconButton from "@/components/IconButton";
import FilterDropdown from "./components/FilterDropdown";
import BatchOperateBar from "./components/BatchOperateBar";
import TimeGroup from "./components/TimeGroup";
import ScrollBar from "@/components/ScrollBar";
import { useHistoryConversationStore, useNotifyStore } from "@/stores";
import { useTimeGroup } from "./composables/useTimeGroup";
import type { ConversationIndex } from "@/types/conversation";
import "@/assets/styles/window/mainwindow/page/historyconversation/HistoryConversation.css";
import { useMainWindowStore } from "@/stores/mainwindow";
import iconEmptyConversationDark from "@/assets/images/icon_empty_conversation_dark.svg";
import iconEmptyConversationLight from "@/assets/images/icon_empty_conversation_light.svg";
import { ButtonShape } from "@/types/button";
import { useBackendStore } from "@/stores";

export default defineComponent({
    name: "HistoryConversation",
    setup() {
        const historyConversationStore = useHistoryConversationStore();
        const notifyStore = useNotifyStore();
        const backendStore = useBackendStore();
        const containerRef = ref<HTMLElement | null>(null);
        const searchInputRef = ref<HTMLInputElement | null>(null);
        const showSearchInput = ref(false);
        const searchKeyword = ref("");

        const isBatchMode = computed(() => historyConversationStore.batchOperateState.isBatchMode);
        const filteredConversations = computed(() => historyConversationStore.getFilteredHistoryConversations);
        const { groupedConversations } = useTimeGroup(filteredConversations);
        const isActionsDisabled = computed(() => groupedConversations.value.length === 0);

        // 翻译文案
        const historyText = computed(() => backendStore.translate("History"));
        const searchText = computed(() => backendStore.translate("Search History"));
        const deleteText = computed(() => backendStore.translate("Delete"));
        const batchManageText = computed(() => backendStore.translate("Batch Manage"));
        const searchPlaceholderText = computed(() => backendStore.translate("Search History"));
        const noHistoryText = computed(() => backendStore.translate("No chat history yet"));

        const handleSearchButtonClick = () => {
            showSearchInput.value = !showSearchInput.value;
            if (!showSearchInput.value) {
                searchKeyword.value = "";
                historyConversationStore.updateFilterCondition({ searchKeyword: "" });
            }
        };

        const isDisabledDeleteButton = computed(() => searchKeyword.value === "");
        const handleDeleteButtonClick = () => {
            searchKeyword.value = "";
            historyConversationStore.updateFilterCondition({ searchKeyword: "" });
            // 清空后重新聚焦输入框
            nextTick(() => {
                searchInputRef.value?.focus();
            });
        };

        const handleSearchInput = (value: string) => {
            searchKeyword.value = value;
            historyConversationStore.updateFilterCondition({ searchKeyword: value });
        };

        const handleSearchInputBlur = () => {
            if (!searchKeyword.value) {
                showSearchInput.value = false;
            }
        };

        // 当显示搜索输入框时自动获取焦点
        watchEffect(() => {
            if (showSearchInput.value) {
                nextTick(() => {
                    searchInputRef.value?.focus();
                });
            }
        });

        const handleBatchModeClick = () => {
            historyConversationStore.updateBatchOperateState(!isBatchMode.value);
        };

        const handleDeleteConversation = (conversationId: string) => {
            const conversation = filteredConversations.value.find((c) => c.id === conversationId);
            if (!conversation) return;
            void notifyStore
                .showDialog({
                    title: backendStore.translate("Confirm delete this conversation"),
                    content: `${backendStore.translate("This will remove all related content from UOS AI")}`,
                    buttons: [
                        { key: "cancel", text: backendStore.translate("Cancel"), type: "default" },
                        { key: "confirm", text: backendStore.translate("Confirm Delete"), type: "danger" },
                    ],
                })
                .then((result) => {
                    if (result.key === "confirm") {
                        historyConversationStore.deleteConversations(conversationId);
                    }
                });
        };

        const handleBatchDelete = (selectedIds: string[]) => {
            if (selectedIds.length === 0) return;
            void notifyStore
                .showDialog({
                    title: backendStore.translate("Delete all records?"),
                    content: `${backendStore.translate("Once deleted, the content cannot be recovered!")}`,
                    buttons: [
                        { key: "cancel", text: backendStore.translate("Cancel"), type: "default" },
                        { key: "confirm", text: backendStore.translate("Confirm Delete"), type: "danger" },
                    ],
                })
                .then((result) => {
                    if (result.key === "confirm") {
                        historyConversationStore.deleteConversations(selectedIds);
                        historyConversationStore.updateBatchOperateState(false);
                    }
                });
        };

        // 处理滚动事件
        const handleScroll = (scrollTop: number) => {
            // 可以在这里处理滚动相关的逻辑
        };

        // 阻止 wrapper 内部点击导致输入框失焦
        const handleWrapperMouseDown = (event: MouseEvent) => {
            event.preventDefault();
        };

        return {
            containerRef,
            searchInputRef,
            showSearchInput,
            searchKeyword,
            isBatchMode,
            isActionsDisabled,
            groupedConversations,
            isDisabledDeleteButton,
            handleSearchButtonClick,
            handleDeleteButtonClick,
            handleSearchInput,
            handleSearchInputBlur,
            handleBatchModeClick,
            handleDeleteConversation,
            handleBatchDelete,
            isDarkMode: computed(() => useMainWindowStore().isDarkMode),
            historyText,
            searchText,
            batchManageText,
            searchPlaceholderText,
            noHistoryText,
            deleteText,
            handleScroll,
            handleWrapperMouseDown,
        };
    },
    render() {
        return (
            <div class="history-conversation">
                <div class="history-conversation__header-container">
                    <div class="history-conversation__header">
                        <div class="history-conversation__title">{this.historyText}</div>
                        <div class="history-conversation__actions">
                            {this.showSearchInput ? (
                                <div
                                    class="history-conversation__search-wrapper"
                                    onMousedown={this.handleWrapperMouseDown}
                                >
                                    <SvgIcon icon="search" class="history-conversation__search-icon" />
                                    <ElInput
                                        ref="searchInputRef"
                                        modelValue={this.searchKeyword}
                                        onInput={this.handleSearchInput}
                                        onBlur={this.handleSearchInputBlur}
                                        placeholder={this.searchPlaceholderText}
                                        class="history-conversation__search-input"
                                    />
                                    <IconButton
                                        icon="trash"
                                        iconSize={[16, 16]}
                                        size={[20, 20]}
                                        tooltip={this.deleteText}
                                        onClick={this.handleDeleteButtonClick}
                                        shape={ButtonShape.Rounded}
                                        disabled={this.isDisabledDeleteButton}
                                        class="history-conversation__delete-button"
                                    />
                                </div>
                            ) : (
                                <IconButton
                                    icon="search"
                                    iconSize={[16, 16]}
                                    size={[30, 30]}
                                    tooltip={this.searchText}
                                    onClick={this.isActionsDisabled ? undefined : this.handleSearchButtonClick}
                                    class="history-conversation__icon-button"
                                    shape={ButtonShape.Rounded}
                                    disabled={this.isActionsDisabled}
                                />
                            )}
                            <IconButton
                                icon="history-manage"
                                iconSize={[16, 16]}
                                size={[30, 30]}
                                tooltip={this.batchManageText}
                                onClick={this.isActionsDisabled ? undefined : this.handleBatchModeClick}
                                class="history-conversation__icon-button"
                                shape={ButtonShape.Rounded}
                                disabled={this.isActionsDisabled}
                            />
                            <FilterDropdown />
                        </div>
                    </div>
                </div>
                <ScrollBar edgeBounce momentum onScroll={this.handleScroll}>
                    <div ref="containerRef" class="history-conversation__content-container">
                        <div class="history-conversation__content">
                            {this.groupedConversations.length > 0 ? (
                                this.groupedConversations.map((group) => (
                                    <TimeGroup
                                        key={group.type}
                                        group={group}
                                        isBatchMode={this.isBatchMode}
                                        onDelete={this.handleDeleteConversation}
                                    />
                                ))
                            ) : (
                                <div class="history-conversation__empty">
                                    <img
                                        src={this.isDarkMode ? iconEmptyConversationDark : iconEmptyConversationLight}
                                    />
                                    <span>{this.noHistoryText}</span>
                                </div>
                            )}
                        </div>
                    </div>
                </ScrollBar>
                <BatchOperateBar onBatchDelete={this.handleBatchDelete} />
            </div>
        );
    },
});
