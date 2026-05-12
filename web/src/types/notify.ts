import type { DialogButton } from "@/types/dialog";

/** showDialog 的调用参数 */
export interface NotifyDialogOptions {
    /** 手动指定图标名，默认显示应用图标 */
    icon?: string;
    /** 对话框标题 */
    title: string;
    /** 正文文本（可选） */
    content?: string;
    /** 底部按钮列表，直接使用 DialogButton，完整支持 beforeClick / suggested */
    buttons: DialogButton[];
}

/** showDialog 的 Promise resolve 结果 */
export interface NotifyDialogResult {
    /** 用户点击的按钮 key；点击右上角关闭时固定为 "cancel" */
    key: string;
}

/** store 内部队列节点（外部无需使用） */
export interface NotifyDialogInstance {
    /** 队列唯一 ID */
    id: string;
    /** 调用参数 */
    options: NotifyDialogOptions;
    /** Promise resolve 回调 */
    resolve: (result: NotifyDialogResult) => void;
}

/** 系统通知操作项 */
export interface SystemNotifyAction {
    /** 操作 key（用于回调识别） */
    key: string;
    /** 操作文案 */
    text: string;
}

/** 系统通知参数 */
export interface SystemNotifyOptions {
    /** 通知标题 */
    title?: string;
    /** 通知内容 */
    body: string;
    /** 图标名或图标路径 */
    icon?: string;
    /** 操作列表 */
    actions?: SystemNotifyAction[];
    /** 显示时长（毫秒），-1 为系统默认 */
    timeoutMs?: number;
}

/** 系统通知 action 回调 */
export interface SystemNotificationActionPayload {
    notificationId: number;
    actionKey: string;
}

/** Toast 消息类型 */
export type ToastType = "info" | "warning" | "error" | "success" | "processing";

/** Toast 操作按钮 */
export interface ToastAction {
    /** 操作标识 */
    key: string;
    /** 按钮文案 */
    text: string;
}

/** showToast 的调用参数 */
export interface ToastOptions {
    /** 消息类型，决定默认图标，默认 "info" */
    type?: ToastType;
    /** 手动指定图标名，优先级高于 type */
    icon?: string;
    /** 通知文案（必填） */
    message: string;
    /** 操作按钮列表 */
    actions?: ToastAction[];
    /** 自动关闭时间（毫秒），默认 3000 */
    duration?: number;
    /** 是否显示关闭按钮，默认 false */
    showClose?: boolean;
}

/** showToast 的 Promise resolve 结果 */
export interface ToastResult {
    /** 用户点击的操作按钮 key；自动关闭或点击关闭按钮时为 "dismiss" */
    key: string;
}

/** Store 内部的 Toast 实例 */
export interface ToastInstance {
    /** 唯一 ID */
    id: string;
    /** 调用参数 */
    options: ToastOptions;
    /** 是否正在关闭动画中 */
    closing: boolean;
    /** 自动关闭定时器 ID */
    timerId?: ReturnType<typeof setTimeout>;
    /** Promise resolve 回调 */
    resolve: (result: ToastResult) => void;
}
