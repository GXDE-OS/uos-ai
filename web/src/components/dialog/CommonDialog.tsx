import { defineComponent, computed, type PropType } from "vue";
import type { DialogButton } from "@/types/dialog";
import BaseDialog from "@/components/dialog/BaseDialog";
import "@/assets/styles/components/CommonDialog.css";

/**
 * 通用消息对话框
 *
 * 在 BaseDialog 基础上封装了标准 icon+title+content 布局。
 * - 默认显示应用图标 "uos-ai-assistant"
 * - icon 可手动指定其他图标
 * - content 渲染为正文段落；需要自定义内容时使用默认插槽
 */
export default defineComponent({
    name: "CommonDialog",

    props: {
        /** 对话框是否可见 */
        visible: {
            type: Boolean,
            required: true,
        },
        /** 图标名，默认显示应用图标 */
        icon: {
            type: String,
            default: "UosAiAssistant",
        },
        /** 对话框标题 */
        title: {
            type: String,
            required: true,
        },
        /** 正文内容文本（可选），复杂内容可使用默认插槽 */
        content: {
            type: String,
            default: "",
        },
        /** 底部按钮列表，完整支持 DialogButton 的所有字段 */
        buttons: {
            type: Array as PropType<DialogButton[]>,
            default: () => [],
        },
    },

    emits: {
        /** 点击右上角关闭按钮时触发 */
        cancel: () => true,
        /** 底部按钮点击后触发，携带按钮 key */
        buttonClick: (key: string) => typeof key === "string",
    },

    setup(props, { emit }) {
        return {
            resolvedIcon: computed(() => props.icon),
            handleCancel: () => emit("cancel"),
            handleButtonClick: (key: string) => emit("buttonClick", key),
        };
    },

    render() {
        return (
            <BaseDialog
                visible={this.visible}
                icon={this.resolvedIcon}
                title={this.title}
                buttons={this.buttons}
                dialogClass="common-dialog"
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                {/* 正文文本（可选） */}
                {this.content && <p class="common-dialog__content">{this.content}</p>}
                {/* 自定义 body 插槽 */}
                {this.$slots.default?.()}
            </BaseDialog>
        );
    },
});
