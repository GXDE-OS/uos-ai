import { SessionEvent } from "@/types/message";
import { useConversationManagerStore } from "@/stores/conversationmanager";
import { defineStore } from "pinia";

export const useSessionChannelStore = defineStore("sessionChannel", {
    state: () => ({}),

    getters: {},

    actions: {
        // 初始化 sessionChannel 监听器
        initializeSessionChannel(sessionChannel: any) {
            if (!sessionChannel) {
                console.warn("Session channel is not available");
                return;
            }

            const conversationManagerStore = useConversationManagerStore();

            // 监听 sessionEvent
            sessionChannel.sessionEvent.connect((event: SessionEvent, sessionId: string, message: string) => {
                console.info("Session event:", event, sessionId, message);
                this.handleSessionEvent(event, sessionId, message, conversationManagerStore);
            });
        },

        // 处理会话事件，调用conversationManagerStore中的方法
        handleSessionEvent(event: SessionEvent, sessionId: string, message: string, conversationManagerStore: any) {
            // 处理会话开始事件
            if (event === SessionEvent.SeStarted) {
                conversationManagerStore.handleSessionStarted(sessionId);
                return;
            }

            // 处理会话消息事件
            if (event === SessionEvent.SeMessage) {
                conversationManagerStore.handleSessionMessage(sessionId, message);
                return;
            }

            // 处理会话完成事件
            if (event === SessionEvent.SeFinished) {
                conversationManagerStore.handleSessionFinished(sessionId, message);
                return;
            }

            // 处理会话错误事件
            if (event === SessionEvent.SeError) {
                conversationManagerStore.handleSessionError(sessionId, message);
                return;
            }
        },
    },
});
