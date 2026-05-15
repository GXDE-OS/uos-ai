import type { ModelNetwork } from "./model";

export interface ComboboxOption {
    value: string | number;
    label?: string;
    icon?: string;
}

export enum ComboBoxDropdownAlign {
    Left = "left",
    Right = "right",
    Center = "center",
}

export type ModelOptionGroup = "recommended" | ModelNetwork;

// 模型选择器选项扩展类型
export interface ModelOption extends ComboboxOption {
    group?: ModelOptionGroup;
    provider?: string;
}
