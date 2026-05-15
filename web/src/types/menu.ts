/**
 * 菜单项类型
 */
export type MenuItemType = 'item' | 'separator' | 'submenu';

/**
 * 菜单项基础接口
 */
export interface MenuItem {
    type: MenuItemType;
    id?: string;
    label?: string;
    /** SvgIcon 图标名，或 data URI */
    icon?: string;
    /** 从系统主题加载的图标名（如 "application-pdf"），由 MenuItem 组件内部异步解析 */
    themeIcon?: string;
    disabled?: boolean;
    onClick?: () => void;
    children?: MenuItem[];
    /** 菜单项是否被勾选（用于 checkable 菜单） */
    checked?: boolean;
}
