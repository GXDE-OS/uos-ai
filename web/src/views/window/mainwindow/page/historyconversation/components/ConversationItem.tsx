import { defineComponent, ref, computed, type VNodeRef, onMounted, onUnmounted, type VNode } from "vue";
import SvgIcon from "@/components/SvgIcon";
import CheckButton from "@/components/CheckButton";
import Menu from "@/components/menu/Menu";
import IconButton from "@/components/IconButton";
import Tooltip from "@/components/Tooltip";
import { ButtonShape } from "@/types/button";
import type { MenuItem } from "@/types/menu";
import { formatDate } from "@/utils/date";
import "@/assets/styles/window/mainwindow/page/historyconversation/components/ConversationItem.css";
import type { ConversationIndex } from "@/types/conversation";
import {
    useHistoryConversationStore,
    useConversationManagerStore,
    useExtensionPanelStore,
    useMainWindowStore,
    useAssistantInfosStore,
    useBackendStore,
} from "@/stores";
import { getIconByType, type Assistant } from "@/types/assistant";

function highlightText(text: string, keyword: string): (string | VNode)[] {
    if (!keyword) {
        return [text];
    }
    const lowerText = text.toLowerCase();
    const lowerKeyword = keyword.toLowerCase();
    const parts: (string | VNode)[] = [];
    let lastIndex = 0;
    let index = lowerText.indexOf(lowerKeyword);

    while (index !== -1) {
        if (index > lastIndex) {
            const leading = text.slice(lastIndex, index);
            if (lastIndex === 0 && leading.length > 20) {
                parts.push("…" + leading.slice(-20));
            } else {
                parts.push(leading);
            }
        }
        parts.push(<span class="conversation-item__highlight">{text.slice(index, index + keyword.length)}</span>);
        lastIndex = index + keyword.length;
        index = lowerText.indexOf(lowerKeyword, lastIndex);
    }

    if (lastIndex < text.length) {
        parts.push(text.slice(lastIndex));
    }

    return parts;
}

export default defineComponent({
    name: "ConversationItem",
    props: {
        conversation: {
            type: Object as () => ConversationIndex,
            required: true,
        },
        isBatchMode: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        delete: (conversationId: string) => true,
    },
    setup(props, { emit }) {
        const historyConversationStore = useHistoryConversationStore();
        const conversationManagerStore = useConversationManagerStore();
        const extensionPanelStore = useExtensionPanelStore();
        const mainWindowStore = useMainWindowStore();
        const assistantInfosStore = useAssistantInfosStore();
        const backendStore = useBackendStore();
        const isHovered = ref(false);
        const optionMenuTriggerRef = ref<HTMLElement | null>(null);

        const searchKeyword = computed(() => historyConversationStore.filterCondition.searchKeyword);

        const menuId = `history:${props.conversation.id}`;
        const isMenuVisible = computed(() => historyConversationStore.openedMenuId === menuId);

        const assistantInfos = computed(() => assistantInfosStore.getAssistantList);

        const currentAssistant = computed(() =>
            assistantInfos.value.find((assistant) => assistant.id === props.conversation.assistant),
        );

        const assistantIcon = computed(() => {
            let iconType = "line";
            if (currentAssistant.value?.path?.startsWith("file://")) {
                iconType = "color";
            }
            return getIconByType(currentAssistant.value as Assistant, iconType);
        });

        const isSelected = computed(() =>
            historyConversationStore.batchOperateState.selectedIds.includes(props.conversation.id),
        );

        const handleMouseEnter = () => {
            isHovered.value = true;
        };

        const handleMouseLeave = () => {
            isHovered.value = false;
        };

        const handleOptionButtonClick = (e: MouseEvent) => {
            e.stopPropagation();
            const newVisible = !isMenuVisible.value;
            historyConversationStore.setOpenedMenuId(newVisible ? menuId : null);
        };

        const handleWrapperClick = () => {
            if (props.isBatchMode) {
                handleCheckboxChange();
            } else {
                handleSwitchCurrentConversation();
                historyConversationStore.setOpenedMenuId(null); // 切换后关闭菜单
            }
        };

        const handleCheckboxChange = () => {
            historyConversationStore.toggleConversationSelection(props.conversation.id);
        };

        const handleSwitchCurrentConversation = () => {
            void mainWindowStore.openChatPage();
            extensionPanelStore.closeExtensionPanel();
            try {
                conversationManagerStore.switchConversation(props.conversation.id);
            } catch (error) {
                console.error("Failed to set current conversation:", error);
            }
        };

        const handleDelete = () => {
            emit("delete", props.conversation.id);
        };

        const handleNoDragMouseDown = (event: MouseEvent) => {
            event.stopPropagation();
        };

        const handleOptionMenuSelect = (item: MenuItem) => {
            if (item.type !== "item") {
                return;
            }

            switch (item.id) {
                case "delete":
                    handleDelete();
                    break;
                default:
                    break;
            }
        };

        const setOptionMenuTriggerRef: VNodeRef = (element) => {
            optionMenuTriggerRef.value = element as HTMLElement | null;
        };

        const optionMenuItems = computed<MenuItem[]>(() => [
            {
                type: "item",
                id: "delete",
                label: backendStore.translate("Delete"),
                icon: "trash",
            },
        ]);

        // 鼠标滚轮事件
        const handleWheel = (e: WheelEvent) => {
            historyConversationStore.setOpenedMenuId(null);
        };

        const handleMenuVisibleUpdate = (visible: boolean) => {
            historyConversationStore.setOpenedMenuId(visible ? menuId : null);
        };

        onMounted(() => {
            const contentContainer = document.querySelector(".history-conversation__content");
            contentContainer?.addEventListener("wheel", handleWheel as EventListener);
        });

        onUnmounted(() => {
            const contentContainer = document.querySelector(".history-conversation__content");
            contentContainer?.removeEventListener("wheel", handleWheel as EventListener);
        });

        return {
            isHovered,
            isMenuVisible,
            optionMenuTriggerRef,
            optionMenuItems,
            isSelected,
            assistantIcon,
            formatDate,
            searchKeyword,
            highlightText,
            handleMouseEnter,
            handleMouseLeave,
            handleOptionButtonClick,
            handleCheckboxChange,
            handleWrapperClick,
            handleNoDragMouseDown,
            handleOptionMenuSelect,
            setOptionMenuTriggerRef,
            handleMenuVisibleUpdate,
        };
    },
    render() {
        return (
            <>
                <div class="conversation-item__wrapper" onClick={this.handleWrapperClick}>
                    {this.isBatchMode && (
                        <CheckButton
                            checked={this.isSelected}
                            onChange={this.handleCheckboxChange}
                            class="conversation-item__checkbox"
                        />
                    )}
                    <div
                        class="conversation-item"
                        onMouseenter={this.handleMouseEnter}
                        onMouseleave={this.handleMouseLeave}
                    >
                        <div class="conversation-item__header">
                            {this.assistantIcon && this.assistantIcon !== "" ? (
                                this.assistantIcon.startsWith("file://") ? (
                                    <div class="conversation-item__icon">
                                        <img src={this.assistantIcon} alt="" />
                                    </div>
                                ) : (
                                    <div class="conversation-item__icon">
                                        <SvgIcon icon={this.assistantIcon} size={[16, 16]} />
                                    </div>
                                )
                            ) : (
                                <div class="conversation-item__icon">
                                    <SvgIcon icon="lost-assistant-historyitem" size={[24, 24]} />
                                </div>
                            )}
                            <Tooltip content={this.conversation.title} showAfter={1000} placement="top-start">
                                <div class="conversation-item__title">
                                    {this.highlightText(this.conversation.title, this.searchKeyword)}
                                </div>
                            </Tooltip>
                            <div class="conversation-item__right">
                                {this.isHovered && !this.isBatchMode ? (
                                    <div ref={this.setOptionMenuTriggerRef}>
                                        <IconButton
                                            icon="more"
                                            iconSize={[16, 16]}
                                            size={[30, 30]}
                                            shape={ButtonShape.Circle}
                                            colorOnly={true}
                                            onClick={this.handleOptionButtonClick}
                                        />
                                    </div>
                                ) : (
                                    <span class="conversation-item__time">
                                        {this.formatDate(this.conversation.updated_at)}
                                    </span>
                                )}
                            </div>
                        </div>
                        {this.conversation.introduction &&
                            !(
                                this.conversation.introduction.trim() !== "" &&
                                this.conversation.introduction === "Null"
                            ) && (
                                <div class="conversation-item__description">
                                    {this.highlightText(this.conversation.introduction, this.searchKeyword)}
                                </div>
                            )}
                    </div>
                </div>
                <Menu
                    items={this.optionMenuItems}
                    visible={this.isMenuVisible}
                    triggerRef={this.optionMenuTriggerRef}
                    placement="bottom"
                    onUpdateVisible={this.handleMenuVisibleUpdate}
                    onSelectItem={this.handleOptionMenuSelect}
                />
            </>
        );
    },
});
