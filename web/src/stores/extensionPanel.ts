import { defineStore } from "pinia";
import type { ExtensionPanelState, OpenExtensionPanelOptions } from "@/types/extension-panel";
import { useBackendStore } from "@/stores/backend";
import { useMainWindowStore } from "@/stores/mainwindow";

export const useExtensionPanelStore = defineStore("extensionPanel", {
    state: (): ExtensionPanelState => ({
        showChatArea: true,
        showExtensionPanel: false,
        extensionContent: null,
        panelFullscreen: false,
        isChatAreaAutoHiddenByResize: false,
        isPanelFullscreenAutoByResize: false,
    }),

    getters: {
        isPanelFullscreen: (state) => state.panelFullscreen,
    },

    actions: {
        /**
         * Open extension panel with specified content
         * @param content - Component or VNode to render in panel
         */
        openExtensionPanel(content: ExtensionPanelState["extensionContent"]) {
            this.extensionContent = content;
            this.showExtensionPanel = true;
            this.ensureWindowWidthForDualColumn();
        },

        /**
         * Open extension panel with options (supports autoFullscreen, etc.)
         * @param options - Configuration options for opening panel
         */
        openExtensionPanelWithOptions(options: OpenExtensionPanelOptions) {
            this.extensionContent = options.content;
            this.showExtensionPanel = true;

            // Auto-enable fullscreen mode if requested
            if (options.autoFullscreen) {
                this.panelFullscreen = true;
                this.isPanelFullscreenAutoByResize = false;
                this.isChatAreaAutoHiddenByResize = false;
                return;
            }

            this.ensureWindowWidthForDualColumn();
        },

        /** Close extension panel and clear content */
        closeExtensionPanel() {
            this.showExtensionPanel = false;
            this.extensionContent = null;
            // 关闭面板时重置全屏状态，确保聊天区可见
            this.panelFullscreen = false;
            this.showChatArea = true;
            this.isChatAreaAutoHiddenByResize = false;
            this.isPanelFullscreenAutoByResize = false;
        },

        /** Toggle extension panel visibility */
        toggleExtensionPanel() {
            this.showExtensionPanel = !this.showExtensionPanel;
        },

        /**
         * Set chat area visibility
         * @param visible - Whether to show chat area
         */
        setChatAreaVisible(visible: boolean) {
            this.showChatArea = visible;

            // 手动恢复聊天区时，清理 resize 自动隐藏标记并退出自动全屏。
            if (visible) {
                this.isChatAreaAutoHiddenByResize = false;
                if (this.isPanelFullscreenAutoByResize) {
                    this.panelFullscreen = false;
                }
                this.isPanelFullscreenAutoByResize = false;
                this.ensureWindowWidthForDualColumn();
            }
        },

        /**
         * Set panel fullscreen state (workspace-level animated fullscreen)
         * When true: chat area animates out, extension panel fills the workspace
         * When false: chat area animates back in
         * @param value - Whether panel is in fullscreen
         */
        setPanelFullscreen(value: boolean) {
            this.panelFullscreen = value;

            // 手动退出全屏时，恢复聊天区并清理自动状态。
            if (!value) {
                this.showChatArea = true;
                this.isChatAreaAutoHiddenByResize = false;
                this.isPanelFullscreenAutoByResize = false;
                this.ensureWindowWidthForDualColumn();
                return;
            }

            this.isPanelFullscreenAutoByResize = false;
            this.isChatAreaAutoHiddenByResize = false;
        },

        // 仅供窗口 resize 逻辑调用：自动进入/退出扩展区全屏。
        applyResizeFullscreenMode(value: boolean) {
            if (!this.showExtensionPanel) {
                return;
            }

            if (value) {
                // 已处于手动全屏时，不应被 resize 逻辑覆盖为“自动全屏”。
                if (this.panelFullscreen && !this.isPanelFullscreenAutoByResize) {
                    return;
                }

                this.showChatArea = false;
                this.panelFullscreen = true;
                this.isChatAreaAutoHiddenByResize = true;
                this.isPanelFullscreenAutoByResize = true;
                return;
            }

            if (!this.isChatAreaAutoHiddenByResize) {
                return;
            }

            this.showChatArea = true;
            if (this.isPanelFullscreenAutoByResize) {
                this.panelFullscreen = false;
            }
            this.isChatAreaAutoHiddenByResize = false;
            this.isPanelFullscreenAutoByResize = false;
        },

        ensureWindowWidthForDualColumn() {
            if (!this.showExtensionPanel || !this.showChatArea || this.panelFullscreen) {
                return;
            }

            const backend = useBackendStore();
            const mainWindowStore = useMainWindowStore();
            const sidebarRequiredWidth = !mainWindowStore.isSidebarCollapsed
                ? mainWindowStore.normalizeSidebarWidth(mainWindowStore.sidebarWidth)
                : 0;
            const requiredWidth = 990 + sidebarRequiredWidth;

            void backend.requestWindow("ensureMinimumWidth", requiredWidth);
        },
    },
});
