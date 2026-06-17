/**
 * ChatView 输入框操作能力类型定义
 */

import type { Component, InjectionKey } from "vue";
import type { MenuItem } from "@/types/menu";
import type { Params } from "@/types/message";
import type { SelectFileOptions } from "@/types/uploadfile";
import type { MainWindowWorkspaceFileDropAction } from "@/utils/mainwindow/workspacePages";

/**
 * 填充模式
 * - replace: 替换现有内容
 * - append: 追加到现有内容后面
 */
export type FillMode = "replace" | "append";

/**
 * ChatView 提供的输入框操作能力
 * 子组件可通过 inject 获取这些能力
 */
export interface ChatInputContext {
    /**
     * 填充内容到输入框
     * @param content - 要填充的文案内容
     * @param mode - 填充模式，默认替换
     */
    fillInput: (content: string, mode?: FillMode) => void;

    /**
     * 清空输入框
     */
    clearInput: () => void;

    /**
     * 聚焦输入框
     */
    focusInput: () => void;

    /**
     * 获取当前输入框内容
     */
    getInputValue: () => string;

    /**
     * 输入框是否被禁用
     */
    isInputDisabled?: () => boolean;
}

/**
 * 快速输入按钮上下文
 */
export interface QuickInputContext {
    /**
     * 显示快速输入按钮
     * @param icon - 按钮图标名
     * @param text - 按钮文本
     * @param styleClass - 附加 CSS 类名
     */
    showQuickInput: (icon: string, text: string, styleClass?: string) => void;

    /**
     * 隐藏快速输入按钮
     */
    hideQuickInput: () => void;
}

/**
 * 输入区支持的菜单动作
 */
export type ChatInputActionId = "upload-file" | "screenshot" | "reference-outline" | "local-file";

/**
 * 输入区动作执行上下文
 */
export interface ChatInputActionContext extends ChatInputContext {
    selectFile: (options?: SelectFileOptions) => Promise<void>;
    startScreenshot: () => Promise<void>;
}

/**
 * 输入区动作菜单组装上下文
 */
export interface ChatInputActionMenuContext {
    isInputDisabled?: boolean;
    isScreenshotVisible?: boolean;
}

/**
 * 输入区动作定义
 */
export interface ChatInputAction {
    id: ChatInputActionId;
    menuItem: MenuItem & {
        type: "item";
        id: ChatInputActionId;
    };
    isVisible?: (context: ChatInputActionMenuContext) => boolean;
    run?: (context: ChatInputActionContext) => void | Promise<void>;
}

/**
 * 输入区场景上下文
 */
export interface ChatInputSceneContext {
    conversationId: string;
    getSelectedMcpServiceIds: () => string[];
}

/**
 * 输入区场景配置
 */
export interface ChatInputSceneConfig {
    actions?: ChatInputAction[];
    actionExtension?: Component | null;
    resolveParams?: (context: ChatInputSceneContext) => Partial<Params>;
    resolveFileDrop?: (
        paths: string[],
    ) => MainWindowWorkspaceFileDropAction | null | undefined | Promise<MainWindowWorkspaceFileDropAction | null | undefined>;
}

/**
 * Injection Key - 用于类型安全的 provide/inject
 * 确保 provide 和 inject 的类型匹配
 */
export const CHAT_INPUT_KEY: InjectionKey<ChatInputContext> = Symbol("chatInput");

/**
 * QuickInput Injection Key
 */
export const QUICK_INPUT_KEY: InjectionKey<QuickInputContext> = Symbol("quickInput");
