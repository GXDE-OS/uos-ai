import type { Assistant } from "@/types/assistant";
import { AssistantID } from "@/types/assistant";
import { getIconByType } from "@/types/assistant";
import { ConversationScene } from "@/types/conversation";
import { useBackendStore } from "@/stores/backend";

export type ConversationInputBorderStyle = "default" | "dashed";
export type ConversationIconSize = [number, number];
export type WelcomeTitleSource = "name" | "slogan";

const DEFAULT_CONVERSATION_HEADER_ICON_SIZE: ConversationIconSize = [36, 36];

// ConversationPresentation 只描述“怎么展示”，
// 不负责决定会话是否持久化、是否进入侧边栏等行为。
export interface ConversationPresentation {
    assistantId?: string;
    header: {
        name: string;
        icon: string;
        iconSize: ConversationIconSize;
        slogan: string;
    };
    welcome: {
        showContent: boolean;
        showIcon: boolean;
        titleSource: WelcomeTitleSource;
        showSlogan: boolean;
    };
    inputArea: {
        borderStyle: ConversationInputBorderStyle;
    };
}

interface ConversationPresentationOverrides {
    header?: Partial<ConversationPresentation["header"]>;
    welcome?: Partial<ConversationPresentation["welcome"]>;
    inputArea?: Partial<ConversationPresentation["inputArea"]>;
}

// assistant 维度用于承载默认展示风格差异，
// 这样 WelcomeView 等视图不需要再写死具体 assistant 的分支逻辑。
const ASSISTANT_PRESENTATION_OVERRIDES: Partial<Record<AssistantID, ConversationPresentationOverrides>> = {
    [AssistantID.UOS_AI]: {
        welcome: {
            showIcon: false,
            titleSource: "slogan",
            showSlogan: false,
        },
    },
};

// 场景文案依赖后端异步加载的翻译，必须在运行时解析，避免模块初始化时提前固化成原文。
function resolveScenePresentationOverrides(scene: ConversationScene): ConversationPresentationOverrides {
    const backendStore = useBackendStore();

    switch (scene) {
        case ConversationScene.Temporary:
            return {
                header: {
                    name: backendStore.translate("Temporary Chat"),
                    icon: "icon_temp_conversation",
                    iconSize: [24, 24],
                    slogan: backendStore.translate(
                        "Temporary chats are not saved in history. The content will be completely deleted upon leaving.",
                    ),
                },
                welcome: {
                    showContent: false,
                    showIcon: true,
                    titleSource: "name",
                    showSlogan: true,
                },
                inputArea: {
                    borderStyle: "dashed",
                },
            };
        case ConversationScene.Default:
        default:
            return {};
    }
}

export function resolveConversationPresentation(options: {
    assistant?: Assistant | null;
    scene?: ConversationScene;
}): ConversationPresentation {
    const { assistant, scene = ConversationScene.Default } = options;
    // 先基于 assistant 生成默认展示，再让场景覆盖掉差异化字段。
    const assistantOverrides = (assistant ? ASSISTANT_PRESENTATION_OVERRIDES[assistant.id] : undefined) ?? {};
    const sceneOverrides = resolveScenePresentationOverrides(scene);
    const headerOverrides = {
        ...assistantOverrides.header,
        ...sceneOverrides.header,
    };
    const welcomeOverrides = {
        ...assistantOverrides.welcome,
        ...sceneOverrides.welcome,
    };
    const inputAreaOverrides = {
        ...assistantOverrides.inputArea,
        ...sceneOverrides.inputArea,
    };

    return {
        assistantId: assistant?.id,
        header: {
            name: headerOverrides.name ?? assistant?.name ?? "UOS AI",
            icon: headerOverrides.icon ?? (assistant ? getIconByType(assistant, "color") : undefined) ?? "assistant",
            iconSize: headerOverrides.iconSize ?? DEFAULT_CONVERSATION_HEADER_ICON_SIZE,
            slogan: headerOverrides.slogan ?? assistant?.description ?? "How can I help you today?",
        },
        welcome: {
            showContent: welcomeOverrides.showContent ?? true,
            showIcon: welcomeOverrides.showIcon ?? true,
            titleSource: welcomeOverrides.titleSource ?? "name",
            showSlogan: welcomeOverrides.showSlogan ?? true,
        },
        inputArea: {
            borderStyle: inputAreaOverrides.borderStyle ?? "default",
        },
    };
}
