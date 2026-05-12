import { ConversationScene } from "@/types/conversation";

// 会话场景行为只定义运行规则，不掺杂 UI 文案和样式。
export interface ConversationSceneBehavior {
    persistHistory: boolean;
    showInSidebar: boolean;
}

const DEFAULT_CONVERSATION_SCENE_BEHAVIOR: ConversationSceneBehavior = {
    persistHistory: true,
    showInSidebar: true,
};

// 新增场景时，只需要在这里补充行为差异。
const CONVERSATION_SCENE_BEHAVIOR_MAP: Record<ConversationScene, ConversationSceneBehavior> = {
    [ConversationScene.Default]: DEFAULT_CONVERSATION_SCENE_BEHAVIOR,
    [ConversationScene.Temporary]: {
        persistHistory: false,
        showInSidebar: false,
    },
};

export function getConversationSceneBehavior(scene: ConversationScene = ConversationScene.Default): ConversationSceneBehavior {
    return CONVERSATION_SCENE_BEHAVIOR_MAP[scene] ?? DEFAULT_CONVERSATION_SCENE_BEHAVIOR;
}
