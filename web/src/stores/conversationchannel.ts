import { defineStore } from "pinia";
import { useConversationManagerStore } from "@/stores/conversationmanager";
import { useBackendStore } from "@/stores/backend";
import { useAssistantInfosStore } from "@/stores/assistantinfos";

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
            const backend = useBackendStore();

            // 监听 changeToConversation 信号
            if (conversationChannel.changeToConversation) {
                conversationChannel.changeToConversation.connect(async (assistantId: string, conversationId: string) => {
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
                });
            }
        },
    },
});