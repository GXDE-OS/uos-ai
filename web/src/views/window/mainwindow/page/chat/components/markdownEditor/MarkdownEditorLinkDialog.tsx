import { defineComponent, ref, watch } from "vue";
import type { DialogButton } from "@/types/dialog";
import BaseDialog from "@/components/dialog/BaseDialog";
import { useBackendStore } from "@/stores/backend";
import "@/assets/styles/components/MarkdownEditorLinkDialog.css";

/**
 * 链接插入对话框
 *
 * 解耦自 MarkdownEditor，独立管理表单状态。
 * 父组件只需传入可见性和预填文本，处理 confirm/cancel 事件即可。
 * 使用通用 Dialog 组件提供对话框框架。
 */
export default defineComponent({
    name: "MarkdownEditorLinkDialog",

    props: {
        /** 对话框是否可见 */
        visible: {
            type: Boolean,
            required: true,
        },
        /** 预填文本（来自编辑器选中内容） */
        initialText: {
            type: String,
            default: "",
        },
    },

    emits: {
        /** 用户确认，携带最终文本和链接 URL */
        confirm: (payload: { text: string; url: string }) =>
            typeof payload.text === "string" && typeof payload.url === "string",
        /** 用户取消 */
        cancel: () => true,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const text = ref("");
        const url = ref("");
        const error = ref("");

        // 每次对话框打开时，同步初始文本并重置表单状态
        watch(
            () => props.visible,
            (visible) => {
                if (visible) {
                    text.value = props.initialText;
                    url.value = "";
                    error.value = "";
                }
            },
        );

        const handleUrlInput = (e: Event) => {
            const val = (e.currentTarget as HTMLInputElement).value;
            url.value = val;
            // 内容变化时立即清除错误状态
            if (error.value) {
                error.value = "";
            }
        };

        const handleButtonClick = (key: string) => {
            if (key === "confirm") {
                emit("confirm", { text: text.value, url: url.value });
            } else {
                emit("cancel");
            }
        };

        const handleCancel = () => {
            emit("cancel");
        };

        // 获取按钮配置（响应式判断表单有效性）
        const getButtons = (): DialogButton[] => {
            const isFormValid = text.value.trim() !== "" && url.value.trim() !== "";
            return [
                {
                    key: "cancel",
                    text: backend.translate("Cancel"),
                    type: "default",
                },
                {
                    key: "confirm",
                    text: backend.translate("Confirm"),
                    type: "primary",
                    suggested: true,
                    disabled: !isFormValid,
                    beforeClick: () => {
                        if (!url.value.includes(".")) {
                            error.value = backend.translate("Please enter a valid link");
                            return false;
                        }
                        error.value = "";
                        return true;
                    },
                },
            ];
        };

        return { backend, text, url, error, handleUrlInput, handleButtonClick, handleCancel, getButtons };
    },

    render() {
        return (
            <BaseDialog
                visible={this.visible}
                title={this.backend.translate("Insert Link")}
                buttons={this.getButtons()}
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                {/* 表单区：Text 字段 */}
                <div class="mde-link-dialog__field">
                    <label class="mde-link-dialog__label">{this.backend.translate("Text:")}</label>
                    <input
                        type="text"
                        class="mde-link-dialog__input"
                        value={this.text}
                        onInput={(e: Event) => {
                            this.text = (e.currentTarget as HTMLInputElement).value;
                        }}
                        placeholder={this.backend.translate("Link text")}
                    />
                </div>

                {/* 表单区：Link 字段 */}
                <div class="mde-link-dialog__field">
                    <label class="mde-link-dialog__label">{this.backend.translate("Link:")}</label>
                    <input
                        type="text"
                        class={["mde-link-dialog__input", this.error && "mde-link-dialog__input--error"]}
                        value={this.url}
                        onInput={this.handleUrlInput}
                        placeholder="https://example.com"
                    />
                    {this.error && <p class="mde-link-dialog__error">{this.error}</p>}
                </div>
            </BaseDialog>
        );
    },
});
