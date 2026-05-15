import { defineStore } from "pinia";
import type { ExtensionPanelState, OpenExtensionPanelOptions } from "@/types/extension-panel";

export const useExtensionPanelStore = defineStore("extensionPanel", {
    state: (): ExtensionPanelState => ({
        showChatArea: true,
        showExtensionPanel: false,
        extensionContent: null,
        panelFullscreen: false,
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
            }
        },

        /** Close extension panel and clear content */
        closeExtensionPanel() {
            this.showExtensionPanel = false;
            this.extensionContent = null;
            // 关闭面板时重置全屏状态，确保聊天区可见
            this.panelFullscreen = false;
            this.showChatArea = true;
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
        },

        /**
         * Set panel fullscreen state (workspace-level animated fullscreen)
         * When true: chat area animates out, extension panel fills the workspace
         * When false: chat area animates back in
         * @param value - Whether panel is in fullscreen
         */
        setPanelFullscreen(value: boolean) {
            this.panelFullscreen = value;
        },
    },
});
