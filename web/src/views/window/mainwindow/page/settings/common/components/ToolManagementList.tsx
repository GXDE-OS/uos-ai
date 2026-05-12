import { defineComponent, type PropType } from "vue";

import ToolManagementListItem from "@/views/window/mainwindow/page/settings/common/components/ToolManagementListItem";
import type { ToolManagementCustomAction, ToolManagementItem } from "@/views/window/mainwindow/page/settings/common/types";

export default defineComponent({
    name: "ToolManagementList",

    components: {
        ToolManagementListItem,
    },

    props: {
        items: {
            type: Array as PropType<ToolManagementItem[]>,
            required: true,
        },
        isLoading: {
            type: Boolean,
            default: false,
        },
        showEditButton: {
            type: Boolean,
            default: true,
        },
        customAction: {
            type: Object as PropType<ToolManagementCustomAction>,
            default: undefined,
        },
        loadingText: {
            type: String,
            default: "loading…",
        },
        emptyText: {
            type: String,
            default: "no data available",
        },
    },

    emits: {
        toggleItem: (_itemId: string, _enabled: boolean) => true,
        editItem: (_itemId: string) => true,
        deleteItem: (_itemId: string) => true,
    },

    setup(props, { emit }) {
        const handleToggleItem = (itemId: string, enabled: boolean) => {
            emit("toggleItem", itemId, enabled);
        };

        const handleEditItem = (itemId: string) => {
            emit("editItem", itemId);
        };

        const handleDeleteItem = (itemId: string) => {
            emit("deleteItem", itemId);
        };

        return {
            handleToggleItem,
            handleEditItem,
            handleDeleteItem,
        };
    },

    render() {
        return (
            <div class="tool-management-list">
                <div class="tool-management-list__body">
                    {this.$props.isLoading ? (
                        <div class="tool-management-list__empty">{this.$props.loadingText}</div>
                    ) : this.$props.items.length > 0 ? (
                        this.$props.items.map((item) => (
                            <div class="tool-management-list__row" key={item.id}>
                                <ToolManagementListItem
                                    item={item}
                                    customAction={this.$props.customAction}
                                    showEditButton={this.$props.showEditButton}
                                    onDeleteItem={this.handleDeleteItem}
                                    onEditItem={this.handleEditItem}
                                    onToggleItem={this.handleToggleItem}
                                />
                            </div>
                        ))
                    ) : (
                        <div class="tool-management-list__empty">{this.$props.emptyText}</div>
                    )}
                </div>
            </div>
        );
    },
});
