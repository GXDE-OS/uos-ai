import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import type { MainWindowWorkspacePageDefinition } from "@/utils/mainwindow/workspacePages";
import DigitalHumanView from "@/views/window/mainwindow/page/digitalhuman/DigitalHuman";
import { useDigitalHumanStore, useBackendStore, useAssistantInfosStore } from "@/stores";

export const digitalHumanWorkspacePageDefinition: MainWindowWorkspacePageDefinition = {
    id: MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN,
    component: DigitalHumanView,

    enter: async () => {
        console.log("DigitalHuman page: enter");
        const digitalHumanStore = useDigitalHumanStore();
        const backendStore = useBackendStore();
        const assistantInfos = useAssistantInfosStore();

        // 设置 backendStore 到 digitalHumanStore
        digitalHumanStore.backendStore = backendStore;

        // 如果 assistantList 已经有数据，立即执行初始化
        if (assistantInfos.assistantList.length > 0) {
            await digitalHumanStore.initDigitalHumanState();
        }
    },

    leave: async () => {
        console.log("DigitalHuman page: leave");
        const digitalHumanStore = useDigitalHumanStore();

        // 关闭数字人会话
        digitalHumanStore.closeDigitalHumanSession();
    },
};
