import { defineStore } from "pinia";
import { useConversationManagerStore } from "@/stores/conversationmanager";
import { useBackendStore } from "@/stores/backend";
import { useAssistantInfosStore } from "@/stores/assistantinfos";
import { useHistoryConversationStore } from "@/stores/historyconversations";
import { useMainWindowStore } from "@/stores/mainwindow";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";

export const useConversationChannelStore = defineStore("conversationChannel", {
    state: () => ({}),

    getters: {},

    actions: {
        // 初始化 conversationChannel 监听器
        initializeConversationChannel(conversationChannel: any) {
            if (!conversationChannel) {
                console.warn("Conversation channel is not available");
                return;
            }

            const conversationManagerStore = useConversationManagerStore();
            const assistantInfosStore = useAssistantInfosStore();
            const historyConversationStore = useHistoryConversationStore();
            const backend = useBackendStore();
            const mainWindowStore = useMainWindowStore();

            // 监听 changeToConversation 信号
            if (conversationChannel.changeToConversation) {
                conversationChannel.changeToConversation.connect(
                    async (assistantId: string, conversationId: string) => {
                        console.log("changeToConversation signal received:", assistantId, conversationId);
                        // 先切换到对应的助手
                        const assistant = assistantInfosStore.getAssistantById(assistantId);
                        if (assistant) {
                            assistantInfosStore.setCurrentAssistant(assistant);
                        }
                        // 然后更新会话索引列表
                        await conversationManagerStore.loadConversationIndexList(backend);
                        // 最后切换到对应会话
                        conversationManagerStore.switchConversation(conversationId);
                    },
                );
            }

            // 监听 indexChanged 信号，搜索完成后重新加载索引列表
            if (conversationChannel.indexSearchChanged) {
                conversationChannel.indexSearchChanged.connect(async () => {
                    if (mainWindowStore.workspacePage !== MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION) {
                        return;
                    }
                    await conversationManagerStore.loadHistoryConversationIndexList(useBackendStore()); // 获取历史会话索引列表(搜索)
                    await historyConversationStore.fetchHistoryConversations();
                    historyConversationStore.setIsSearching(false);
                    historyConversationStore.setIsInitialLoading(false);
                });
            }
        },
    },
});
