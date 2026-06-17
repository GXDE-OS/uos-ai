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
        const isLoading = computed(
            () => historyConversationStore.getIsSearching || historyConversationStore.getIsInitialLoading,
        );
        const loadingText = computed(() =>
            historyConversationStore.getIsInitialLoading
                ? backendStore.translate("Loading history conversations...")
                : backendStore.translate("Searching history conversations..."),
        );

        // 翻译文案
        const historyText = computed(() => backendStore.translate("History"));
        const searchText = computed(() => backendStore.translate("Search History"));
        const deleteText = computed(() => backendStore.translate("Delete"));
        const batchManageText = computed(() => backendStore.translate("Batch Manage"));
        const searchPlaceholderText = computed(() => backendStore.translate("Search conversation titles or content…"));
        const noHistoryText = computed(() =>
            searchKeyword.value
                ? backendStore.translate("No historical conversations found.")
                : backendStore.translate("No chat history yet"),
        );

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
            showSearchInput.value = false;
            historyConversationStore.updateFilterCondition({ searchKeyword: "" });
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

        const handleSearchInputKeydown = (event: KeyboardEvent) => {
            if (event.key === "Escape") {
                handleDeleteButtonClick();
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

        // 阻止 wrapper 内部点击导致输入框失焦（但不阻止输入框自身的 focus）
        const handleWrapperMouseDown = (event: MouseEvent) => {
            if (!(event.target instanceof HTMLInputElement)) {
                event.preventDefault();
            }
        };

        return {
            containerRef,
            searchInputRef,
            showSearchInput,
            searchKeyword,
            isBatchMode,
            isActionsDisabled,
            isLoading,
            loadingText,
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
            handleSearchInputKeydown,
        };
    },
    render() {
        return (
            <div class={["history-conversation", { "history-conversation--loading": this.isLoading }]}>
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
                                        onKeydown={this.handleSearchInputKeydown}
                                        placeholder={this.searchPlaceholderText}
                                        class="history-conversation__search-input"
                                    />
                                    {this.searchKeyword && (
                                        <IconButton
                                            icon="icon_titlebar_close"
                                            iconSize={[16, 16]}
                                            size={[16, 16]}
                                            colorOnly={true}
                                            tooltip={this.deleteText}
                                            onClick={this.handleDeleteButtonClick}
                                            shape={ButtonShape.Circle}
                                            disabled={this.isDisabledDeleteButton}
                                            class="history-conversation__delete-button"
                                        />
                                    )}
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
                <div class="history-conversation__content-wrapper">
                    {this.isLoading ? (
                        <div class="history-conversation__loading">
                            <SvgIcon icon="loading" size={[32, 32]} class="history-conversation__loading-icon" />
                            <span>{this.loadingText}</span>
                        </div>
                    ) : this.groupedConversations.length > 0 ? (
                        <ScrollBar edgeBounce momentum onScroll={this.handleScroll}>
                            <div ref="containerRef" class="history-conversation__content-container">
                                <div class="history-conversation__content">
                                    {this.groupedConversations.map((group) => (
                                        <TimeGroup
                                            key={group.type}
                                            group={group}
                                            isBatchMode={this.isBatchMode}
                                            onDelete={this.handleDeleteConversation}
                                        />
                                    ))}
                                </div>
                            </div>
                        </ScrollBar>
                    ) : (
                        <div class="history-conversation__empty">
                            <img src={this.isDarkMode ? iconEmptyConversationDark : iconEmptyConversationLight} />
                            <span>{this.noHistoryText}</span>
                        </div>
                    )}
                </div>

                <BatchOperateBar onBatchDelete={this.handleBatchDelete} />
            </div>
        );
    },
});
