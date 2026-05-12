import {
    useAssistantInfosStore,
    useConversationManagerStore,
    useExtensionPanelStore,
    useMainWindowStore,
    useModelInfosStore,
} from "@/stores";
import { AssistantID } from "@/types/assistant";
import { ConversationScene } from "@/types/conversation";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import { createId } from "@/utils/date";

type CreateConversationOptions = {
    assistantId: string;
    scene?: ConversationScene;
};

export async function createConversation({ assistantId, scene = ConversationScene.Default }: CreateConversationOptions) {
    // 标题栏和侧边栏都复用这条入口，避免“新建对话”行为在多个地方漂移。
    const assistantInfosStore = useAssistantInfosStore();
    const conversationManagerStore = useConversationManagerStore();
    const extensionPanelStore = useExtensionPanelStore();
    const mainWindowStore = useMainWindowStore();
    const modelInfosStore = useModelInfosStore();

    const targetAssistant = assistantInfosStore.assistantList.find((assistant) => assistant.id === assistantId);
    if (!targetAssistant) {
        console.warn(`[MainWindow] Assistant ${assistantId} not found, failed to create conversation.`);
        return false;
    }

    assistantInfosStore.setCurrentAssistant(targetAssistant);
    await modelInfosStore.loadModelList(targetAssistant.id);

    const conversationId = createId();
    const currentModelId = modelInfosStore.getCurrentModel?.id || "";

    // 使用当前助手下的默认模型创建空白会话，并立即切换到该会话。
    conversationManagerStore.createConversation(conversationId, targetAssistant.id, currentModelId, scene);

    extensionPanelStore.closeExtensionPanel();

    if (mainWindowStore.workspacePage !== MAIN_WINDOW_WORKSPACE_PAGES.CHAT) {
        await mainWindowStore.openChatPage();
    }

    return true;
}

export async function createDefaultConversation() {
    return createConversation({
        assistantId: AssistantID.UOS_AI,
    });
}

export async function createTemporaryConversation() {
    return createConversation({
        assistantId: AssistantID.UOS_AI,
        scene: ConversationScene.Temporary,
    });
}
