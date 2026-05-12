import { defineStore } from "pinia";
import { WindowMode } from "@/types/windowinfo";
import { useMainWindowStore } from "./mainwindow";
import { useNotifyStore } from "./notify";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";

export const useWindowChannelStore = defineStore("windowChannel", {
    state: () => ({
        windowMode: WindowMode.Null,
        // 待处理的提示信息
        pendingPrompt: null as { question: string; isSend: boolean } | null,
    }),

    getters: {},

    actions: {
        // 设置待处理的提示信息
        setPendingPrompt(question: string, isSend: boolean) {
            this.pendingPrompt = { question, isSend };
        },

        // 清除待处理的提示信息
        clearPendingPrompt() {
            this.pendingPrompt = null;
        },

        // 初始化 windowChannel 监听器
        initializeWindowChannel(windowChannel: any) {
            if (!windowChannel) {
                console.warn("Window channel is not available");
                return;
            }

            // 监听 windowModeChanged 信号
            if (windowChannel["windowModeChanged"]) {
                windowChannel["windowModeChanged"].connect((mode: number) => {
                    this.windowMode = mode as WindowMode;
                    console.log("window mode change to:", this.windowMode);
                });
            }

            // 监听 windowChangeToDigitalMode 信号，切换到数字人页面
            if (windowChannel["windowChangeToDigitalMode"]) {
                windowChannel["windowChangeToDigitalMode"].connect(() => {
                    const mainWindowStore = useMainWindowStore();
                    // 如果已经在数字人页面，不需要切换
                    if (mainWindowStore.workspacePage === MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN) {
                        console.log("windowChangeToDigitalMode signal received, already on digital human page");
                        return;
                    }
                    mainWindowStore.toggleDigitalHumanPage();
                    console.log("windowChangeToDigitalMode signal received, switching to digital human page");
                });
            }

            // 监听 windowAppendPrompt 信号
            if (windowChannel["windowAppendPrompt"]) {
                windowChannel["windowAppendPrompt"].connect((question: string, isSend: boolean) => {
                    console.log("windowAppendPrompt signal received:", question, isSend);
                    this.setPendingPrompt(question, isSend);
                });
            }

            // 监听 toastRequested 信号
            if (windowChannel["toastRequested"]) {
                windowChannel["toastRequested"].connect((type: string, message: string) => {
                    console.log("toastRequested signal received:", type, message);
                    const notifyStore = useNotifyStore();
                    notifyStore.showToast({ type: type as any, message });
                });
            }
        },
    },
});