import { useHistoryConversationStore } from "@/stores";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import type { MainWindowWorkspacePageDefinition } from "@/utils/mainwindow/workspacePages";
import HistoryConversation from "@/views/window/mainwindow/page/historyconversation/HistoryConversation";

export const historyConversationWorkspacePageDefinition: MainWindowWorkspacePageDefinition = {
    id: MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION,
    component: HistoryConversation,
    backButton: {
        text: "Back",
        fallbackPage: MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
    },
    enter: async () => {
        await useHistoryConversationStore().fetchHistoryConversations();
    },
    leave: async () => {
        useHistoryConversationStore().resetPageState();
    },
};
