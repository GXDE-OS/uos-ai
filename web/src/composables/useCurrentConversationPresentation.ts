import { computed } from "vue";
import { useAssistantInfosStore, useConversationManagerStore } from "@/stores";
import { ConversationScene } from "@/types/conversation";
import { resolveConversationPresentation } from "@/utils/mainwindow/conversationPresentation";

// 统一为聊天相关组件提供“当前会话的展示配置”，
// 让 WelcomeView、InputArea 等视图只消费配置，不直接关心具体场景分支。
export function useCurrentConversationPresentation() {
    const assistantInfosStore = useAssistantInfosStore();
    const conversationManagerStore = useConversationManagerStore();

    const currentConversationPresentation = computed(() => {
        const currentConversationRecord = conversationManagerStore.getCurrentConversationRecord;
        // 优先使用当前会话 root 上绑定的 assistant，
        // 避免全局 currentAssistant 与当前会话暂时不同步时出现展示漂移。
        const assistant =
            (currentConversationRecord?.root?.assistant
                ? assistantInfosStore.getAssistantById(currentConversationRecord.root.assistant)
                : null) ?? assistantInfosStore.getCurrentAssistant;

        return resolveConversationPresentation({
            assistant,
            scene: conversationManagerStore.getCurrentConversationScene ?? ConversationScene.Default,
        });
    });

    return {
        currentConversationPresentation,
    };
}
