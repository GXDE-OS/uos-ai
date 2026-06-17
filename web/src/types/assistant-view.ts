import type { InjectionKey, ComputedRef } from "vue";

/**
 * 助手视图配置
 * 控制各助手的 UI 展示行为，通过 provide/inject 向子组件传递
 */
export interface AssistantViewConfig {
    chat?: {
        /** 是否启用文件隐私提醒弹窗，默认 false */
        showFilePrivacyDialog?: boolean;
    };
    input?: {
        /** 是否显示深度思考按钮，默认 true */
        showDeepThink?: boolean;
        /** 是否显示搜索按钮，默认 false */
        showSearch?: boolean;
    };
    message?: {
        /** 是否显示重试按钮，默认 true */
        showRetry?: boolean;
    };
    /** 新会话时的默认提示词（支持模板语法） */
    defaultPrompt?: string;
}

export const ASSISTANT_VIEW_CONFIG_KEY: InjectionKey<ComputedRef<AssistantViewConfig>> = Symbol("assistantViewConfig");
