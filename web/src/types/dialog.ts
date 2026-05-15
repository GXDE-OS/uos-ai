/**
 * 通用对话框按钮视觉类型
 */
export type DialogButtonType = "default" | "primary" | "danger";

/**
 * 通用对话框按钮配置
 */
export interface DialogButton {
    /** 唯一标识，buttonClick 事件时原样返回给调用方 */
    key: string;
    /** 按钮文案 */
    text: string;
    /** 视觉样式类型，默认 default */
    type?: DialogButtonType;
    /** 是否禁用按钮 */
    disabled?: boolean;
    /** 建议操作按钮（加高亮装饰，不改变 type 色彩语义） */
    suggested?: boolean;
    /** 点击前钩子，返回 false 或 Promise<false> 则阻止触发 buttonClick 事件 */
    beforeClick?: () => boolean | Promise<boolean>;
}
