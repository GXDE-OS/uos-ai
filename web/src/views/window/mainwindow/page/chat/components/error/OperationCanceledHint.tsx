import { defineComponent, computed } from "vue";
import { useBackendStore } from "@/stores/backend";

/**
 * 操作取消提示组件
 * 当操作被取消时显示的提示信息
 */
export default defineComponent({
    name: "OperationCanceledHint",

    emits: {
        // 点击重新编辑事件
        edit: () => {
            return true;
        },
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const text = computed(() => backendStore.translate("You stopped this answer, "));
        const editText = computed(() => backendStore.translate("please re-edit your question"));

        // 处理重新编辑点击
        const handleEditClick = (event: MouseEvent) => {
            event.stopPropagation();
            emit("edit");
        };

        return { handleEditClick, text, editText };
    },

    render() {
        return (
            <div class="operation-canceled-hint">
                <span class="operation-canceled-hint__text">{this.text}</span>
                <span class="operation-canceled-hint__link" onClick={this.handleEditClick}>
                    {this.editText}
                </span>
            </div>
        );
    },
});
