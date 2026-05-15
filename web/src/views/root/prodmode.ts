import Qt from "./QWebChannel";
import { useRouter } from "vue-router";
import { useBackendStore } from "@/stores";
import { useReportChannelStore } from "@/stores/reportchannel";
import { updateActiveColor, updateFont, updateThemeColor } from "@/utils/themeAppearance";

var initialize = async function () {
    const router = useRouter();
    const backendStore = useBackendStore();
    const reportChannelStore = useReportChannelStore();
    new Qt(qt.webChannelTransport, async function (channel) {
        router.replace({ name: "App" });
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
        backendStore.setReportChannel(channel.objects.reportObj);
        reportChannelStore.initializeReportChannel(channel.objects.reportObj);

        // Initialize system theme active color
        const systemObj = channel.objects.systemObj;
        if (systemObj) {
            const activeColor = systemObj.activeColor;
            if (activeColor) {
                updateActiveColor(activeColor);
            }
            systemObj.activeColorChanged.connect((color: string) => {
                updateActiveColor(color);
            });

            // // 初始化系统主题字体
            const fontInfo = await systemObj.fontInfo;
            updateFont(fontInfo);

            // // 查询当前深浅主题
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
};

export default initialize;
