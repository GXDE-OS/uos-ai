import { AssistantID } from "@/types/assistant";
import GenericAssistant from "./GenericAssistant";
import WritingAssistant from "./WritingAssistant";
import McpAndSkillsAssistant from "./McpAndSkillsAssistant";
import TranslateAssistant from "./TranslateAssistant";

/**
 * 欢迎页个性化组件映射表
 * key: AssistantID 枚举值
 * value: 对应的个性化 Vue 组件
 */
export const WELCOME_COMPONENT_MAP: Record<AssistantID, any> = {
    [AssistantID.UOS_AI]: GenericAssistant,
    [AssistantID.UOS_AI_WRITING]: WritingAssistant,
    [AssistantID.UOS_AI_MCP_AND_SKILL]: McpAndSkillsAssistant,
    [AssistantID.UOS_AI_TRANSLATION]: TranslateAssistant,
    // 后续扩展：
    // [AssistantID.UOS_AI_KNOWLEDGE_BASE]: KnowledgeBaseAssistant,
};

/**
 * 根据助手 ID 获取对应的欢迎页个性化组件
 * @param assistantId - 助手 ID (AssistantID 枚举值)
 * @returns 对应的组件实例，没有匹配则返回 null
 */
export const getWelcomeComponent = (assistantId: string) => {
    return WELCOME_COMPONENT_MAP[assistantId as AssistantID] || null;
};
