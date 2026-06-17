import { defineStore } from "pinia";
import { nextTick } from "vue";
import type { AppUpdateInfo } from "@/types/appUpdate";
import type { ToastResult } from "@/types/notify";
import { useBackendStore } from "@/stores/backend";
import { useNotifyStore } from "@/stores/notify";
import {
    APP_UPDATE_REMINDER_VIEW_FEATURES_ACTION,
    APP_UPDATE_UPGRADE_ACTION,
    getAppUpdateStorePackageId,
} from "@/utils/appUpdateReminder";

let _appUpdateReminderInitialized = false;
const _notifiedAppUpdateVersions = new Set<string>();

function getAppUpdateVersionKey(info?: AppUpdateInfo | null): string {
    return info?.availableVersion?.trim() || "";
}

export const useAppUpdateStore = defineStore("appUpdate", {
    state: () => ({
        /** 应用更新特性弹窗内容 */
        appUpdateFeatureDialogInfo: null as AppUpdateInfo | null,
    }),

    actions: {
        /**
         * 初始化应用更新提醒监听。
         *
         * 主窗口挂载后主动触发一次版本检查，并监听后端实时回传。
         */
        initializeAppUpdateReminderChannel(systemChannel: any): void {
            if (_appUpdateReminderInitialized || !systemChannel) {
                return;
            }

            if (systemChannel.appUpdateAvailable?.connect) {
                systemChannel.appUpdateAvailable.connect((info: AppUpdateInfo) => {
                    this.showAppUpdateReminderToast(info);
                });
            }

            _appUpdateReminderInitialized = true;
            void this.requestAppUpdateCheck();
        },

        /**
         * 由前端主动触发更新检查。
         */
        async requestAppUpdateCheck(): Promise<void> {
            const backend = useBackendStore();
            if (!backend.systemChannel) {
                return;
            }

            try {
                await backend.requestSystem("checkAppUpdate");
            } catch (error) {
                console.warn("[AppUpdateStore] Failed to request app update check:", error);
            }
        },

        /**
         * 展示新版本 toast，并在 toast 状态完成渲染周期后通知后端写入数据库。
         */
        showAppUpdateReminderToast(info?: AppUpdateInfo | null): void {
            const versionKey = getAppUpdateVersionKey(info);
            if (!versionKey || _notifiedAppUpdateVersions.has(versionKey)) {
                return;
            }

            const backend = useBackendStore();
            const notifyStore = useNotifyStore();
            const message = backend.translate("New version available");
            const viewFeaturesText = backend.translate("View new features");
            const upgradeNowText = backend.translate("Upgrade now");

            let toast: ReturnType<typeof notifyStore.showToast>;
            try {
                toast = notifyStore.showToast({
                    type: "info",
                    icon: "icon_upgrade",
                    message,
                    duration: 0,
                    showClose: true,
                    actions: [
                        {
                            key: APP_UPDATE_REMINDER_VIEW_FEATURES_ACTION,
                            text: viewFeaturesText,
                            type: "default",
                        },
                        {
                            key: APP_UPDATE_UPGRADE_ACTION,
                            text: upgradeNowText,
                            type: "primary",
                        },
                    ],
                });
            } catch (error) {
                console.warn("[AppUpdateStore] Failed to show app update reminder toast:", error);
                return;
            }

            _notifiedAppUpdateVersions.add(versionKey);
            void this.handleAppUpdateToastAction(toast.promise, info);
            void this.markAppUpdateReminderConsumedAfterToastRendered(toast.id, versionKey);
        },

        async handleAppUpdateToastAction(
            toastPromise: Promise<ToastResult>,
            info?: AppUpdateInfo | null,
        ): Promise<void> {
            try {
                const result = await toastPromise;
                if (result.key === APP_UPDATE_REMINDER_VIEW_FEATURES_ACTION) {
                    this.showAppUpdateFeatureDialog(info);
                    return;
                }

                if (result.key === APP_UPDATE_UPGRADE_ACTION) {
                    await this.openAppStoreForAppUpdate(info);
                }
            } catch (error) {
                console.warn("[AppUpdateStore] Failed to handle app update reminder toast action:", error);
            }
        },

        async markAppUpdateReminderConsumedAfterToastRendered(toastId: string, versionKey: string): Promise<void> {
            try {
                await nextTick();

                const notifyStore = useNotifyStore();
                if (!notifyStore.toasts.some((toast) => toast.id === toastId)) {
                    _notifiedAppUpdateVersions.delete(versionKey);
                    return;
                }

                const backend = useBackendStore();
                await backend.requestSystem("markAppUpdateReminderConsumed", versionKey);
            } catch (error) {
                console.warn("[AppUpdateStore] Failed to mark app update reminder consumed:", error);
            }
        },

        /**
         * 展示应用更新特性弹窗。
         */
        showAppUpdateFeatureDialog(info?: AppUpdateInfo | null): void {
            if (!info) {
                return;
            }

            this.appUpdateFeatureDialogInfo = { ...info };
        },

        /**
         * 关闭应用更新特性弹窗。
         */
        closeAppUpdateFeatureDialog(): void {
            this.appUpdateFeatureDialogInfo = null;
        },

        /**
         * 打开商店的 UOS AI 页面。
         */
        async openAppStoreForAppUpdate(info?: AppUpdateInfo | null): Promise<void> {
            const backend = useBackendStore();
            await backend.requestSystem("openAppStore", getAppUpdateStorePackageId(info)).catch((error) => {
                console.warn("[AppUpdateStore] Failed to open app store for app update:", error);
            });
        },

        /**
         * 从更新特性弹窗执行“立刻升级”。
         */
        async upgradeFromAppUpdateFeatureDialog(): Promise<void> {
            const info = this.appUpdateFeatureDialogInfo;
            this.closeAppUpdateFeatureDialog();
            await this.openAppStoreForAppUpdate(info);
        },
    },
});
