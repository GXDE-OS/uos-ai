import { defineComponent, ref, onMounted, computed } from "vue";
import {
    useBackendStore,
    useAssistantInfosStore,
    useModelInfosStore,
    useSessionChannelStore,
    useUploadFilesStore,
    useConversationManagerStore,
    useNotifyStore,
    useTaskChannelStore,
    useConversationChannelStore,
    useFileChannelStore,
    useWindowChannelStore,
    useMainWindowStore,
    useNetworkStore,
} from "@/stores";
import { createId } from "@/utils/date";
import { AssistantID } from "@/types/assistant";
import { WindowMode } from "@/types/windowinfo";

import MainWindow from "@/views/window/mainwindow/MainWindow";
import MiniWindow from "@/views/window/minwindow/MiniWindow";
import SideWindow from "@/views/window/mainwindow/SideWindow";

export default defineComponent({
    name: "RootWindow",
    components: {
        MainWindow,
        MiniWindow,
        SideWindow,
    },
    setup() {
        const backend = useBackendStore();
        const assistantInfosStore = useAssistantInfosStore();
        const modelInfosStore = useModelInfosStore();
        const sessionChannelStore = useSessionChannelStore();
        const uploadFilesStore = useUploadFilesStore();
        const conversationManagerStore = useConversationManagerStore();
        const notifyStore = useNotifyStore();
        const taskChannelStore = useTaskChannelStore();
        const conversationChannelStore = useConversationChannelStore();
        const fileChannelStore = useFileChannelStore();
        const windowChannelStore = useWindowChannelStore();
        const mainWindowStore = useMainWindowStore();
        const networkStore = useNetworkStore();

        // 初始化各个 channel 监听器
        windowChannelStore.initializeWindowChannel(backend.windowChannel);
        fileChannelStore.initializeFileChannel(backend.fileChannel);
        conversationChannelStore.initializeConversationChannel(backend.conversationChannel);
        taskChannelStore.initializeTaskChannel(backend.taskChannel);
        assistantInfosStore.initializeEnvChannel(backend.serviceConfigChannel);

        // 监听后端助手列表变化，保证动态增删助手后前端状态能及时刷新。
        assistantInfosStore.initializeConnections();
        modelInfosStore.initializeConnections();

        onMounted(async () => {
            //初始化翻译
            await backend.loadTranslations();

            // 初始化是否启用高级 CSS 功能
            await backend.loadAdvancedCssFeaturesStatus();

            // 初始化窗口模式。主窗口渲染前先恢复侧边栏状态，避免首次进入时布局闪动。
            const windowMode = (await backend.requestWindow("windowMode")) as WindowMode;
            if (windowMode === WindowMode.Main) {
                await mainWindowStore.loadPersistedSidebarState();
            }
            windowChannelStore.windowMode = windowMode;

            // 先加载保存的顺序配置，再加载助手列表，避免首次渲染出现顺序闪动。
            await assistantInfosStore.loadAssistantOrder(backend);
            await assistantInfosStore.loadAssistantVisibleCount(backend);
            await assistantInfosStore.loadAssistantList(backend);

            // 使用当前助手ID加载模型列表
            const currentAssistant = assistantInfosStore.getCurrentAssistant;
            const assistantId = currentAssistant?.id || AssistantID.UOS_AI;
            await modelInfosStore.loadModelList(assistantId);

            await conversationManagerStore.loadConversationIndexList(backend);

            console.log("assistant list:", assistantInfosStore.assistantList);
            console.log("model list:", modelInfosStore.modelList);

            // 初始化会话通道
            sessionChannelStore.initializeSessionChannel(backend.sessionChannel);
            notifyStore.initializeSystemNotificationChannel(backend.systemChannel);
            conversationManagerStore.initializeAiReplyNotificationHandlers();

            // 会话管理器创建对话
            await conversationManagerStore.createConversation(
                createId(), // 初始化会话ID，使用当前时间戳
                assistantId,
                modelInfosStore.getCurrentModel?.id || "", // TODO: 暂时使用当前模型
            );

            // 初始化网络状态
            await networkStore.initNetworkStatus(backend.systemChannel);

            console.log("init window mode:", windowChannelStore.windowMode);

            // 通知后端窗口初始化完成
            taskChannelStore.notifyWindowCreated(backend.taskChannel);
        });

        return {
            windowMode: computed(() => windowChannelStore.windowMode),
        };
    },
    render() {
        return (
            <div class="root-window">
                {this.windowMode === WindowMode.Main ? <MainWindow /> : null}
                {this.windowMode === WindowMode.Mini ? <MiniWindow /> : null}
                {this.windowMode === WindowMode.Side ? <SideWindow /> : null}
            </div>
        );
    },
});
