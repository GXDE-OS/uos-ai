export interface Assistant {
    id: AssistantID;
    name: string;
    description?: string;
    icon?: Record<string, string>; // icon type as key (e.g., "line", "color"), icon name as value
    gradient_colors?: string[]; // 流光动效渐变颜色数组，默认为空
    path?: string;
    place_holder?: string;
    envExists?: boolean; // 环境是否存在，默认为 true
}

// Get icon by type
export function getIconByType(assistant: Assistant, type: string): string | undefined {
    if (!assistant) {
        return undefined;
    }

    if (!assistant.icon) {
        return undefined;
    }

    if (assistant.path?.startsWith("file://")) {
        // 三方助手，直接返回路径
        return assistant.path + assistant.icon[type] + "-110.svg";
    }

    // Try to get the specified type
    if (assistant.icon[type]) {
        return assistant.icon[type];
    }

    // Fallback to first available icon
    const firstIcon = Object.values(assistant.icon)[0];
    return firstIcon;
}

export enum AssistantID {
    UOS_AI = "uos-ai-generic", //uos-ai-generic
    UOS_AI_KNOWLEDGE_BASE = "uos-ai-knowledge-base", //uos-ai-knowledge-base
    UOS_AI_TRANSLATION = "uos-ai-translation", //uos-ai-translation
    UOS_AI_WRITING = "uos-ai-writing", //uos-ai-writing
    UOS_AI_MCP_AND_SKILL = "uos-ai-claw", //uos-ai-claw
}
