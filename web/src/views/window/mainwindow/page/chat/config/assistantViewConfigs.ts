import { AssistantID } from "@/types/assistant";
import type { AssistantViewConfig } from "@/types/assistant-view";
import { useBackendStore } from "@/stores";

/** 构建写作助手默认提示词模板（支持国际化） */
const buildWritingDefaultPrompt = (): string => {
    const backend = useBackendStore();
    const prompt = backend.translate("writing_default_prompt");
    return `<uos-ai-prompt-template>${prompt}</uos-ai-prompt-template>`;
};

const ASSISTANT_VIEW_CONFIG_MAP: Partial<Record<AssistantID, AssistantViewConfig>> = {
    [AssistantID.UOS_AI_WRITING]: {
        chat: { showFilePrivacyDialog: true },
        input: { showDeepThink: false, showSearch: 1 },
        message: { showRetry: false },
        get defaultPrompt() {
            return buildWritingDefaultPrompt();
        },
    },
    [AssistantID.UOS_AI]: {
        input: { showSearch: 2 },
    },
    [AssistantID.UOS_AI_TRANSLATION]: {
        input: { showDeepThink: false },
    },
    [AssistantID.UOS_AI_KNOWLEDGE_BASE]: {
        input: { showDeepThink: false },
    },
};

export const getAssistantViewConfig = (assistantId?: string | null): AssistantViewConfig => {
    return ASSISTANT_VIEW_CONFIG_MAP[assistantId as AssistantID] ?? {};
};
