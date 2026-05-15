import { defineComponent, computed } from "vue";
import BaseDialog from "@/components/dialog/BaseDialog";
import type { DialogButton } from "@/types/dialog";
import { useBackendStore } from "@/stores";
import "@/assets/styles/components/FilePrivacyDialog.css";

/**
 * 文件隐私保护对话框
 *
 * 当用户使用在线模型（或开启联网搜索）并携带本地文件提交问题时，
 * 需要告知用户素材将上传至在线模型，由用户决定是否继续。
 */
export default defineComponent({
    name: "FilePrivacyDialog",

    props: {
        /** 对话框是否可见 */
        visible: {
            type: Boolean,
            required: true,
        },
        /** 当前选中的模型名称，用于提示文案 */
        modelName: {
            type: String,
            default: "",
        },
        /**
         * 触发原因是否仅为联网搜索（模型本身是本地模型，但开启了联网搜索）。
         * true  → "素材将会上传至网络进行搜索，是否继续生成？"
         * false → "素材将会上传至在线模型（...）分析，是否继续生成？"
         */
        webSearch: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        /** 取消，不发送消息 */
        cancel: () => true,
        /** 用户确认继续生成 */
        confirm: () => true,
    },

    setup(_, { emit }) {
        const backend = useBackendStore();

        const cancelText = computed(() => backend.translate("Cancel"));
        const continueText = computed(() => backend.translate("Continue Generating"));
        const privacyDescText = computed(() => backend.translate("If you don't want local materials to be uploaded, you can do the following before generating content:"));
        const privacyTip1Text = computed(() => backend.translate("1. Switch to a local model (e.g., DeepSeek-R1-1.5B) or a privately deployed model"));
        const privacyTip2Text = computed(() => backend.translate('2. Turn off "Web Search"'));

        const buttons: DialogButton[] = [
            {
                key: "cancel",
                text: cancelText.value,
                type: "default",
            },
            {
                key: "confirm",
                text: continueText.value,
                type: "primary",
                suggested: true,
            },
        ];

        const handleButtonClick = (key: string) => {
            if (key === "cancel") {
                emit("cancel");
            } else {
                emit("confirm");
            }
        };

        const handleCancel = () => {
            emit("cancel");
        };

        return { buttons, handleButtonClick, handleCancel, backend, privacyDescText, privacyTip1Text, privacyTip2Text };
    },

    render() {
        const title = this.webSearch
            ? this.backend.translate("Materials will be uploaded to the web for search. Continue generating?")
            : this.modelName
              ? this.backend.translate("Materials will be uploaded to the online model (%1) for analysis. Continue generating?").replace("%1", this.modelName)
              : this.backend.translate("Materials will be uploaded to the online model for analysis. Continue generating?");

        return (
            <BaseDialog
                visible={this.visible}
                title={title}
                buttons={this.buttons}
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
                dialogClass="file-privacy-dialog"
            >
                {/* 提示正文 */}
                <p class="file-privacy-dialog__desc">
                    {this.privacyDescText}
                </p>
                <ul class="file-privacy-dialog__list">
                    <li>{this.privacyTip1Text}</li>
                    <li>{this.privacyTip2Text}</li>
                </ul>
            </BaseDialog>
        );
    },
});
