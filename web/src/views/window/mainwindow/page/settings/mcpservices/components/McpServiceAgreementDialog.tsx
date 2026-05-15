import { computed, defineComponent, ref, watch } from "vue";

import CheckButton from "@/components/CheckButton";
import BaseDialog from "@/components/dialog/BaseDialog";
import { useBackendStore } from "@/stores";
import type { DialogButton } from "@/types/dialog";
import "@/assets/styles/window/mainwindow/page/settings/mcpservices/McpServiceAgreementDialog.css";

export default defineComponent({
    name: "McpServiceAgreementDialog",

    props: {
        visible: {
            type: Boolean,
            required: true,
        },
    },

    emits: {
        cancel: () => true,
        confirm: () => true,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const agreed = ref(false);

        watch(
            () => props.visible,
            (visible) => {
                if (visible) {
                    agreed.value = false;
                }
            },
            {
                immediate: true,
            },
        );

        const buttons = computed<DialogButton[]>(() => [
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
                disabled: !agreed.value,
            },
        ]);

        const titleText = computed(() => backend.translate("Enabling MCP Server Features"));
        const descriptionText = computed(() =>
            backend.translate(
                "Some third-party MCP server features carry certain risks. Please use them with caution. If you enable this service, a built-in tool will detect and automatically download necessary dependencies. This download process will incur data charges. Please be aware of these risks and proceed with caution.",
            ),
        );
        const agreementText = computed(() => backend.translate("I have understood and agree to use this service"));

        const handleCancel = () => {
            emit("cancel");
        };

        const handleButtonClick = (key: string) => {
            if (key === "cancel") {
                emit("cancel");
                return;
            }

            emit("confirm");
        };

        const handleAgreementChange = (checked: boolean) => {
            agreed.value = checked;
        };

        const handleAgreementRowClick = () => {
            agreed.value = !agreed.value;
        };

        return {
            agreed,
            buttons,
            titleText,
            descriptionText,
            agreementText,
            handleCancel,
            handleButtonClick,
            handleAgreementChange,
            handleAgreementRowClick,
        };
    },

    render() {
        return (
            <BaseDialog
                visible={this.$props.visible}
                buttons={this.buttons}
                dialogClass="mcp-service-agreement-dialog"
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                <div class="mcp-service-agreement-dialog__content">
                    <h3 class="mcp-service-agreement-dialog__title">{this.titleText}</h3>
                    <p class="mcp-service-agreement-dialog__description">{this.descriptionText}</p>

                    <div class="mcp-service-agreement-dialog__agreement" onClick={this.handleAgreementRowClick}>
                        <CheckButton checked={this.agreed} onChange={this.handleAgreementChange} />
                        <span class="mcp-service-agreement-dialog__agreement-text">{this.agreementText}</span>
                    </div>
                </div>
            </BaseDialog>
        );
    },
});
