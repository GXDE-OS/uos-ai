import { defineComponent, computed } from "vue";
import { useHistoryConversationStore, useBackendStore } from "@/stores";
import "@/assets/styles/window/mainwindow/page/historyconversation/components/BatchOperateBar.css";
import IconButton from "@/components/IconButton";
import CheckButton from "@/components/CheckButton";
import { ButtonShape } from "@/types/button";

export default defineComponent({
    name: "BatchOperateBar",
    emits: {
        batchDelete: (selectedIds: string[]) => true,
    },
    setup(props, { emit }) {
        const historyConversationStore = useHistoryConversationStore();
        const backendStore = useBackendStore();

        const isBatchMode = computed(() => historyConversationStore.batchOperateState.isBatchMode);
        const selectedIds = computed(() => historyConversationStore.batchOperateState.selectedIds);
        const selectedCount = computed(() => historyConversationStore.getSelectedCount);
        const isAllSelected = computed(() => historyConversationStore.isAllSelected);
        const isPartiallySelected = computed(() => selectedCount.value > 0 && !isAllSelected.value);

        const selectAllText = computed(() => backendStore.translate("Select All"));
        const conversationsSelectedText = computed(() => {
            const text = backendStore.translate("%1 conversations selected");
            return text.replace("%1", selectedCount.value.toString());
        });

        const handleSelectAll = (checked: boolean) => {
            historyConversationStore.toggleSelectAll(checked);
        };

        const handleDelete = () => {
            if (selectedCount.value === 0) {
                return;
            }
            emit("batchDelete", selectedIds.value);
        };

        const handleClose = () => {
            historyConversationStore.updateBatchOperateState(false);
        };

        return {
            isBatchMode,
            selectedCount,
            isAllSelected,
            isPartiallySelected,
            selectAllText,
            conversationsSelectedText,
            handleSelectAll,
            handleDelete,
            handleClose,
            isEnableAdvancedCssFeatures: backendStore.isEnableAdvancedCssFeatures,
        };
    },
    render() {
        if (!this.isBatchMode) {
            return null;
        }

        return (
            <div class={["batch-operate-bar", this.isEnableAdvancedCssFeatures && "batch-operate-bar--advanced"]}>
                <div class="batch-operate-bar__left">
                    <CheckButton
                        checked={this.isAllSelected}
                        indeterminate={this.isPartiallySelected}
                        onChange={this.handleSelectAll}
                        class="batch-operate-bar__checkbox"
                    />
                    <span class="batch-operate-bar__label">{this.selectAllText}</span>
                    <span class="batch-operate-bar__count">{this.conversationsSelectedText}</span>
                </div>
                <div class="batch-operate-bar__right">
                    <div class="batch-operate-bar__divider"></div>
                    <IconButton
                        class="batch-operate-bar__button--delete"
                        icon="trash"
                        iconSize={[16, 16]}
                        size={[30, 30]}
                        shape={ButtonShape.Rounded}
                        onClick={this.handleDelete}
                        disabled={this.selectedCount === 0}
                        iconColor="rgba(245, 54, 0, 1)"
                    />
                    <div class="batch-operate-bar__divider"></div>
                    <IconButton
                        icon="icon_titlebar_close"
                        iconSize={[16, 16]}
                        size={[30, 30]}
                        shape={ButtonShape.Rounded}
                        onClick={this.handleClose}
                        // disabled={this.selectedCount === 0}
                    />
                </div>
            </div>
        );
    },
});
