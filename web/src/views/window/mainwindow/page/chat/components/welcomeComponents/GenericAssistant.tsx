import { defineComponent, computed } from "vue";
import SvgIcon from "@/components/SvgIcon";
import { useAssistantInfosStore } from "@/stores";
import type { Assistant } from "@/types/assistant";
import { getIconByType } from "@/types/assistant";
import { createConversation } from "@/utils/mainwindow/conversationActions";

/** 横向最多展示的智能体数量 */
const MAX_VISIBLE = 5;

/** 可用的图标颜色数组 */
const ICON_COLORS = ["#31BC52", "#449FFF", "#678CFC", "#FF9602", "#B167ED", "#06D7DB", "#F15800"];

/** 洗牌算法，返回打乱的数组 */
const shuffleArray = (array: string[]): string[] => {
    const result: string[] = [...array];
    for (let i = result.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        const temp = result[i]!;
        result[i] = result[j]!;
        result[j] = temp;
    }
    return result;
};

/**
 * 基础助手 (UOS_AI) 的欢迎页个性化组件
 * 横向展示至多 5 个智能体卡片
 */
export default defineComponent({
    name: "GenericAssistant",

    setup() {
        const store = useAssistantInfosStore();

        /** 全部助手列表 */
        const allAssistants = computed(() => store.getAssistantList);

        /** 当前助手 ID */
        const currentAssistantId = computed(() => store.getCurrentAssistant?.id);

        /** 可用颜色池（打乱顺序） */
        const shuffledColors = computed(() => shuffleArray(ICON_COLORS));

        /** 可见助手（排除当前助手，最多 MAX_VISIBLE 个）并为每个分配不重复的随机颜色 */
        const visibleAssistants = computed(() => {
            const assistants = allAssistants.value
                .filter((assistant) => assistant.id !== currentAssistantId.value)
                .slice(0, MAX_VISIBLE);

            // 为每个助手分配不重复的颜色
            const colors = shuffledColors.value;
            return assistants.map((assistant, index) => ({
                ...assistant,
                color: colors[index % colors.length],
            }));
        });

        const handleAssistantClick = async (assistant: Assistant) => {
            await createConversation({ assistantId: assistant.id });
        };

        return { visibleAssistants, handleAssistantClick };
    },

    render() {
        if (!this.visibleAssistants.length) return null;

        return (
            <div class="generic-assistant">
                <div class="generic-assistant__agent-row">
                    {this.visibleAssistants.map((assistant) => {
                        // 判断是否是三方助手
                        const isThirdPartyAssistant = assistant.path?.startsWith("file://");
                        let iconType = "";
                        if (isThirdPartyAssistant) {
                            iconType = "color";
                        } else {
                            iconType = "line";
                        }
                        const iconSrc = getIconByType(assistant, iconType) || "uos-ai";

                        return (
                            <div
                                key={assistant.id}
                                class="generic-assistant__agent-card"
                                onClick={() => this.handleAssistantClick(assistant)}
                            >
                                <div class="generic-assistant__agent-icon">
                                    {!isThirdPartyAssistant ? (
                                        <SvgIcon icon={iconSrc} size={[20, 20]} color={assistant.color} />
                                    ) : (
                                        <img src={iconSrc} class="generic-assistant__agent-icon-img" />
                                    )}
                                </div>
                                <span class="generic-assistant__agent-name">{assistant.name}</span>
                            </div>
                        );
                    })}
                </div>
            </div>
        );
    },
});
