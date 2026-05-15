import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import { useAssistantInfosStore, useExtensionPanelStore } from "@/stores";
import type { MainWindowWorkspacePageDefinition } from "@/utils/mainwindow/workspacePages";
import ChatView from "@/views/window/mainwindow/page/chat/ChatView";
import { getInputAreaSceneConfig } from "@/views/window/mainwindow/page/chat/config/inputAreaScenes";

export const chatWorkspacePageDefinition: MainWindowWorkspacePageDefinition = {
    id: MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
    component: ChatView,
    acceptFileDrop: true,
    handleFileDrop: async ({ paths }) => {
        const currentAssistantId = useAssistantInfosStore().getCurrentAssistant?.id;
        const sceneConfig = getInputAreaSceneConfig(currentAssistantId);
        return (await sceneConfig.resolveFileDrop?.(paths)) ?? { type: "upload" };
    },
    enter: async () => {},
    leave: async () => {
        useExtensionPanelStore().closeExtensionPanel();
    },
};
