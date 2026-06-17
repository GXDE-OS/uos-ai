import Qt from "./QWebChannel";
import WebSocketTransport from "./WebSocketTransport";
import { useRouter } from "vue-router";
import { useBackendStore } from "@/stores";
import { hexToRgb } from "@/utils/hexToRgb";
import { updateActiveColor, updateFont, updateThemeColor } from "@/utils/themeAppearance";

var initialize = async function () {
    const router = useRouter();
    const backendStore = useBackendStore();

    // 使用当前访问的主机名，支持跨网访问
    // 如果 hostname 为空（如 file:// 协议），则回退到 localhost
    const wsHost = window.location.hostname || 'localhost'
    const wsPort = 8081
    const wsUrl = `ws://${wsHost}:${wsPort}`
    console.log(`WebSocket connecting to: ${wsUrl}`)
    const wsTransport = new WebSocketTransport(wsUrl);
    wsTransport.connect().then(() => {
        new Qt(wsTransport, async function (channel: any) {
            backendStore.setSessionChannel(channel.objects.sessObj);
            backendStore.setWindowChannel(channel.objects.windowObj);
            backendStore.setAssistantChannel(channel.objects.assistObj);
            backendStore.setSystemChannel(channel.objects.systemObj);
            backendStore.setServiceConfigChannel(channel.objects.serviceConfigObj);
            backendStore.setConversationChannel(channel.objects.conversationObj);
            backendStore.setFileChannel(channel.objects.fileObj);
            backendStore.setAudioChannel(channel.objects.audioObj);
            backendStore.setTaskChannel(channel.objects.taskObj);
            backendStore.setSkillsMgr(channel.objects.skillsMgr);
            router.replace({ name: "App" });

            // Initialize system theme active color
            const systemObj = channel.objects.systemObj;
            if (systemObj) {
                // 初始化系统主题活动色颜色
                const activeColor = systemObj.activeColor;
                if (activeColor) {
                    updateActiveColor(activeColor);
                }
                systemObj.activeColorChanged.connect((color: string) => {
                    updateActiveColor(color);
                });

                // 初始化系统主题字体
                const fontInfo = await systemObj.fontInfo;
                updateFont(fontInfo);

                // 查询当前深浅主题
                const themeColor = await systemObj.themeColor;
                updateThemeColor(themeColor);
                systemObj.themeColorChanged.connect((themeColor: number) => {
                    updateThemeColor(themeColor);
                });

                systemObj.themeIconChanged.connect(() => {
                    backendStore.bumpThemeIconVersion();
                });
            }

            const windowObj = channel.objects.windowObj;
            if (windowObj) {
                windowObj.windowFontChanged.connect((fontInfo: string) => {
                    updateFont(fontInfo);
                });
            }
        });
    });
};

export default initialize;
