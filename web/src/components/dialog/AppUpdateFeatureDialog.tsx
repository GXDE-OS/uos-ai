import { defineComponent, computed } from "vue";
import BaseDialog from "@/components/dialog/BaseDialog";
import ScrollBar from "@/components/ScrollBar";
import {
    APP_UPDATE_CANCEL_ACTION,
    APP_UPDATE_UPGRADE_ACTION,
    getAppUpdateFeatureDialogButtons,
} from "@/utils/appUpdateReminder";
import { useAppUpdateStore, useBackendStore } from "@/stores";
import "@/assets/styles/components/AppUpdateFeatureDialog.css";

export default defineComponent({
    name: "AppUpdateFeatureDialog",

    setup() {
        const backend = useBackendStore();
        const appUpdateStore = useAppUpdateStore();

        const buttons = computed(() => getAppUpdateFeatureDialogButtons((key) => backend.translate(key)));
        const titleText = computed(() => backend.translate("New version features"));
        const currentInfo = computed(() => appUpdateStore.appUpdateFeatureDialogInfo);
        const visible = computed(() => !!currentInfo.value);
        const description = computed(() => currentInfo.value?.updateDescription || "");

        const handleButtonClick = (key: string) => {
            if (key === APP_UPDATE_UPGRADE_ACTION) {
                void appUpdateStore.upgradeFromAppUpdateFeatureDialog();
                return;
            }

            if (key === APP_UPDATE_CANCEL_ACTION) {
                appUpdateStore.closeAppUpdateFeatureDialog();
            }
        };

        return {
            visible,
            titleText,
            description,
            buttons,
            handleButtonClick,
            handleCancel: () => appUpdateStore.closeAppUpdateFeatureDialog(),
        };
    },

    render() {
        if (!this.visible) {
            return null;
        }

        return (
            <BaseDialog
                icon="icon_upgrade"
                visible={this.visible}
                title={this.titleText}
                buttons={this.buttons}
                dialogClass="app-update-feature-dialog"
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                <div class="app-update-feature-dialog__body">
                    <ScrollBar class="app-update-feature-dialog__scroll">
                        {this.description && <div class="app-update-feature-dialog__content">{this.description}</div>}
                    </ScrollBar>
                </div>
            </BaseDialog>
        );
    },
});
