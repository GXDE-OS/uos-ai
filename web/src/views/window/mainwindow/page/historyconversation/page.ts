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
        const store = useHistoryConversationStore();
        store.setIsInitialLoading(true); // 初始化时设置为加载状态
        store.updateFilterCondition({ searchKeyword: "" }); // 这里调用一次后端接口，保证初始化时消息能正常接收到结果信号
        store.setIsSearching(false); // 初始化时设置为非搜索状态
        await store.fetchHistoryConversations();
    },
    leave: async () => {
        useHistoryConversationStore().resetPageState();
    },
};
