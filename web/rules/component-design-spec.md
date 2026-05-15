# UOS AI 通用组件设计规范

## 目录

1. [概述](#概述)
2. [设计原则](#设计原则)
3. [组件风格化设计规范](#组件风格化设计规范)
4. [基础组件类型定义](#基础组件类型定义)
5. [基础表单组件规范](#基础表单组件规范)
6. [导航和布局组件规范](#导航和布局组件规范)
7. [反馈组件规范](#反馈组件规范)
8. [组件文档和示例规范](#组件文档和示例规范)

## 概述

本规范定义了 UOS AI 前端项目中通用组件的设计、开发和维护标准。

**编码规范请参考**：[项目编码规范](../ai-coding-standards.md)

本规范主要约定：

- 组件的 API 设计（Props、Events、Slots）
- 组件的扩展能力和通用设计
- 各类组件的具体使用示例

## 设计原则

### 1. 一致性原则

- 所有组件遵循统一的 API 设计模式
- 相似的组件具有相似的 Props 和 Events
- 统一的命名规范和代码风格

### 2. 可扩展性原则

- 组件设计考虑未来的功能扩展
- 提供灵活的插槽（Slots）机制
- 支持主题定制和样式覆盖

### 3. 易用性原则

- 组件 API 简洁明了，学习成本低
- 提供合理的默认值
- 完善的 TypeScript 类型支持

### 4. 性能优化原则

- 避免不必要的重渲染
- 合理使用计算属性和缓存
- 虚拟滚动等性能优化手段

## 基础组件类型定义

### 组件通用类型

```typescript
// src/types/component.ts

// 组件尺寸
export type ComponentSize = "small" | "medium" | "large";

// 组件类型/变体
export type ComponentType = "default" | "primary" | "success" | "warning" | "danger";

// 按钮形状
export enum ButtonShape {
    Square = "square", // 方方角
    Rounded = "rounded", // 圆角
    Circle = "circle", // 圆形
}

// 输入框类型
export type InputType = "text" | "password" | "textarea" | "number" | "email" | "tel" | "url";

// 布局方向
export type LayoutDirection = "horizontal" | "vertical";

// 对齐方式
export type AlignType = "start" | "center" | "end" | "stretch";

// 主轴对齐方式
export type JustifyType = "start" | "center" | "end" | "space-between" | "space-around";

// 放置位置
export type Placement = "top" | "bottom" | "left" | "right" | "topLeft" | "topRight" | "bottomLeft" | "bottomRight";

// 触发方式
export type TriggerType = "hover" | "click" | "contextMenu";

// 菜单模式
export type MenuMode = "vertical" | "horizontal" | "inline";

// 标签位置
export type TabPosition = "top" | "right" | "bottom" | "left";

// 进度条类型
export type ProgressType = "line" | "circle" | "dashboard";

// 进度条状态
export type ProgressStatus = "normal" | "success" | "exception" | "active";

// 通知类型
export type NoticeType = "info" | "success" | "warning" | "error";

// 主题类型
export type ThemeType = "light" | "dark";
```

### 组件 Props 基础接口

```typescript
// src/types/component.ts

// 基础组件 Props
export interface BaseComponentProps {
    // 自定义类名
    className?: string;

    // 自定义样式
    style?: CSSProperties;

    // 是否禁用
    disabled?: boolean;
}

// 表单组件基础 Props
export interface BaseFormComponentProps extends BaseComponentProps {
    // 尺寸
    size?: ComponentSize;

    // 类型/变体
    type?: ComponentType;
}

// 可点击组件 Props
export interface ClickableComponentProps extends BaseComponentProps {
    // 点击事件
    onClick?: (event: MouseEvent) => void;
}

// 可变化组件 Props
export interface ChangeableComponentProps<T = any> extends BaseComponentProps {
    // 当前值
    value?: T;

    // 默认值
    defaultValue?: T;

    // 值变化事件
    onChange?: (value: T, event?: Event) => void;
}
```

## 组件风格化设计规范

### 概述

UOS AI 组件采用统一的风格化设计，确保所有同类组件具有一致的视觉表现和交互体验。风格化设计包括：

- **三态设计**：默认态、悬停态、激活/按下态
- **禁用态**：统一的禁用状态表现
- **暗色模式**：自动适配系统暗色主题
- **尺寸规范**：统一的尺寸、间距、圆角值
- **颜色系统**：统一的颜色透明度和渐变规则

### 颜色系统

#### 透明度层级

```css
/* 背景透明度层级 */
--bg-opacity-hover: 0.05; /* 悬停态背景透明度 */
--bg-opacity-active: 0.1; /* 激活态背景透明度 */
--bg-opacity-disabled: 0.5; /* 禁用态透明度 */

/* 中性灰透明度层级 */
--gray-opacity-light: 0.1; /* 浅色透明度 */
--gray-opacity-medium: 0.4; /* 中等透明度 */
--gray-opacity-dark: 0.8; /* 深色透明度 */

/* 轨道和滑块透明度 */
--track-opacity: 0.1; /* 轨道透明度 */
--track-opacity-hover: 0.15; /* 轨道悬停透明度 */
--thumb-opacity: 0.4; /* 滑块透明度 */
--thumb-opacity-hover: 0.6; /* 滑块悬停透明度 */
--thumb-opacity-active: 0.8; /* 滑块激活透明度 */
```

#### 颜色使用规范

```css
/* 悬停态背景 - 浅色模式 */
:hover:not(.disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-hover));
}

/* 激活态背景 - 浅色模式 */
:active:not(.disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-active));
}

/* 暗色模式适配 */
@media (prefers-color-scheme: dark) {
    :hover:not(.disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-hover));
    }

    :active:not(.disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-active));
    }
}
```

### 尺寸系统

#### 圆角规范

```css
/* 圆角值 */
--radius-none: 0; /* 无圆角 */
--radius-small: 4px; /* 小圆角 */
--radius-normal: 6px; /* 圆角 */
--radius-circle: 50%; /* 圆形 */
```

#### 间距规范

```css
/* 间距值 */
--spacing-xs: 2px; /* 极小间距 */
--spacing-sm: 4px; /* 小间距 */
--spacing-md: 8px; /* 中间距 */
--spacing-lg: 16px; /* 大间距 */
--spacing-xl: 24px; /* 超大间距 */
--spacing-xxl: 32px; /* 极大间距 */
```

#### 尺寸规范

```css
/* 组件尺寸 */
--size-xs: 16px; /* 极小尺寸 */
--size-sm: 24px; /* 小尺寸 */
--size-md: 32px; /* 中尺寸 */
--size-lg: 40px; /* 大尺寸 */
--size-xl: 48px; /* 超大尺寸 */

/* 滚动条尺寸 */
--scrollbar-width: 8px; /* 默认滚动条宽度 */
--scrollbar-width-sm: 6px; /* 小滚动条宽度 */
--scrollbar-width-lg: 12px; /* 大滚动条宽度 */
--scrollbar-thumb-min: 20px; /* 滑块最小尺寸 */
```

### 交互状态规范

#### 三态设计模式

所有可交互组件应遵循以下三态设计：

```css
/* 1. 默认态 */
.component {
    background: transparent;
    border: none;
    cursor: pointer;
}

/* 2. 悬停态 */
.component:hover:not(.component-disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-hover));
}

/* 3. 激活/按下态 */
.component:active:not(.component-disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-active));
}

/* 4. 禁用态 */
.component-disabled {
    cursor: default;
    opacity: var(--bg-opacity-disabled);
    pointer-events: none;
}
```

#### 禁用态规范

```css
/* 禁用态统一样式 */
.component-disabled {
    cursor: default;
    opacity: var(--bg-opacity-disabled);
    pointer-events: none;
    user-select: none;
}

/* 禁用态子元素 */
.component-disabled * {
    pointer-events: none;
}
```

### 形状变体规范

#### 按钮形状

```css
/* 方形 - 无圆角 */
.component--square {
    border-radius: var(--radius-none);
}

/* 圆角方形 */
.component--rounded {
    border-radius: var(--radius-normal);
}

/* 圆形 */
.component--circle {
    border-radius: var(--radius-circle);
}
```

#### 尺寸变体

```css
/* 小尺寸 */
.component--small {
    width: var(--size-sm);
    height: var(--size-sm);
}

/* 中尺寸 */
.component--medium {
    width: var(--size-md);
    height: var(--size-md);
}

/* 大尺寸 */
.component--large {
    width: var(--size-lg);
    height: var(--size-lg);
}
```

### 布局规范

**特别注意!!!**：布局中禁用gap参数，因为低版本的qtwebengine无法渲染该参数。

#### Flex 居中布局

```css
/* 水平垂直居中 */
.component {
    display: flex;
    align-items: center;
    justify-content: center;
}

/* 仅水平居中 */
.component {
    display: flex;
    align-items: center;
}

/* 仅垂直居中 */
.component {
    display: flex;
    justify-content: center;
}
```

### 动画规范

#### 过渡效果

```css
/* 快速过渡 */
--transition-fast: 0.15s ease;

/* 标准过渡 */
--transition-normal: 0.2s ease;

/* 慢速过渡 */
--transition-slow: 0.3s ease;

/* 使用示例 */
.component {
    transition:
        background var(--transition-normal),
        opacity var(--transition-normal),
        transform var(--transition-normal);
}
```

#### 淡入淡出动画

```css
/* 淡入动画 */
@keyframes fadeIn {
    from {
        opacity: 0;
        transform: scale(0.95);
    }
    to {
        opacity: 1;
        transform: scale(1);
    }
}

/* 淡出动画 */
@keyframes fadeOut {
    from {
        opacity: 1;
        transform: scale(1);
    }
    to {
        opacity: 0;
        transform: scale(0.95);
    }
}

/* 使用示例 */
.component {
    animation: fadeIn var(--transition-normal);
}
```

### 暗色模式规范

#### 自动适配

```css
/* 使用 prefers-color-scheme 自动适配 */
@media (prefers-color-scheme: dark) {
    .component:hover:not(.component-disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-hover));
    }

    .component:active:not(.component-disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-active));
    }
}
```

#### 手动切换（通过类名）

```css
/* 通过父容器类名切换 */
.dark-theme .component:hover:not(.component-disabled) {
    background: rgba(255, 255, 255, var(--bg-opacity-hover));
}

.dark-theme .component:active:not(.component-disabled) {
    background: rgba(255, 255, 255, var(--bg-opacity-active));
}
```

### 无障碍支持

#### 键盘焦点

```css
/* 焦点样式 */
.component:focus {
    outline: 2px solid rgba(52, 152, 219, 0.5);
    outline-offset: 2px;
}

/* 焦点可见性 */
.component:focus-visible {
    outline: 2px solid rgba(52, 152, 219, 0.5);
    outline-offset: 2px;
}

.component:focus:not(:focus-visible) {
    outline: none;
}
```

#### 高对比度模式

```css
/* 高对比度模式支持 */
@media (prefers-contrast: high) {
    .component {
        border: 1px solid #000;
    }

    .component:hover:not(.component-disabled) {
        background: rgba(0, 0, 0, 0.3);
    }
}
```

#### 减少动画偏好

```css
/* 减少动画偏好支持 */
@media (prefers-reduced-motion: reduce) {
    .component,
    .component *,
    .component::before,
    .component::after {
        transition: none !important;
        animation: none !important;
    }
}
```

### 具体组件风格示例

#### IconButton 组件风格

```css
/* IconButton.css */
.icon-button {
    background: transparent;
    border: none;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    outline: none;
}

/* 形状变体 */
.icon-button-square {
    border-radius: var(--radius-none);
}

.icon-button-rounded {
    border-radius: var(--radius-normal);
}

.icon-button-circle {
    border-radius: var(--radius-circle);
}

/* 三态 */
.icon-button:hover:not(.icon-button-disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-hover));
}

.icon-button:active:not(.icon-button-disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-active));
}

/* 禁用态 */
.icon-button-disabled {
    cursor: default;
    opacity: var(--bg-opacity-disabled);
    pointer-events: none;
}

/* 暗色模式 */
@media (prefers-color-scheme: dark) {
    .icon-button:hover:not(.icon-button-disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-hover));
    }

    .icon-button:active:not(.icon-button-disabled) {
        background: rgba(255, 255, 255, var(--bg-opacity-active));
    }
}
```

#### ScrollBar 组件风格

```css
/* ScrollBar.css */
.scroll-bar-track {
    background-color: rgba(120, 120, 120, var(--track-opacity));
    border-radius: var(--radius-small);
    transition: background-color var(--transition-fast);
}

.scroll-bar:hover .scroll-bar-track {
    background-color: rgba(120, 120, 120, var(--track-opacity-hover));
}

.scroll-bar-thumb {
    background-color: rgba(120, 120, 120, var(--thumb-opacity));
    border-radius: var(--radius-small);
    transition: all var(--transition-fast);
    cursor: pointer;
}

.scroll-bar-thumb:hover {
    background-color: rgba(120, 120, 120, var(--thumb-opacity-hover));
}

.scroll-bar--dragging .scroll-bar-thumb {
    background-color: rgba(120, 120, 120, var(--thumb-opacity-active));
}
```

#### Splitter 组件风格

```css
/* Splitter.css */
.splitter {
    background: transparent;
    cursor: col-resize;
    display: flex;
    align-items: center;
    justify-content: center;
    border-left: 1px solid transparent;
    border-right: 1px solid transparent;
    transition: border-color var(--transition-normal);
}

.splitter:hover {
    background: rgba(0, 0, 0, 0.02);
}

.splitter:active {
    background: rgba(0, 0, 0, 0.05);
}

.splitter-handle {
    width: 4px;
    height: 40px;
    background: transparent;
    opacity: 0;
    transition: all var(--transition-normal);
}

.splitter:hover .splitter-handle {
    opacity: 1;
    background: rgba(0, 0, 0, 0.2);
}
```

## 基础表单组件规范

### Button 组件

#### API 设计

```typescript
interface ButtonProps {
    // 按钮类型
    type?: "default" | "primary" | "success" | "warning" | "danger";

    // 按钮尺寸
    size?: "small" | "medium" | "large";

    //.按钮形状
    shape?: "default" | "round" | "circle";

    // 是否禁用
    disabled?: boolean;

    // 加载状态
    loading?: boolean;

    // 幽灵按钮（透明背景）
    ghost?: boolean;

    // 块级按钮（宽度 100%）
    block?: boolean;

    // 图标
    icon?: string;

    // 图标位置
    iconPosition?: "left" | "right";

    // 自定义类名
    className?: string;
}
```

#### 使用示例

```tsx
// 基础按钮
<Button>Default Button</Button>

// 不同类型
<Button type="primary">Primary Button</Button>
<Button type="success">Success Button</Button>
<Button type="warning">Warning Button</Button>
<Button type="danger">Danger Button</Button>

// 不同尺寸
<Button size="small">Small</Button>
<Button size="medium">Medium</Button>
<Button size="large">Large</Button>

// 图标按钮
<Button icon="search">Search</Button>
<Button icon="download" iconPosition="right">Download</Button>

// 加载状态
<Button loading>Loading</Button>

// 禁用状态
<Button disabled>Disabled</Button>

// 幽灵按钮
<Button type="primary" ghost>Ghost Button</Button>

// 块级按钮
<Button block>Block Button</Button>

// 圆角按钮
<Button shape="round">Round Button</Button>

// 圆形按钮
<Button shape="circle" icon="plus" />
```

### Input 组件

#### API 设计

```typescript
interface InputProps {
    // 输入框类型
    type?: "text" | "password" | "textarea" | "number" | "email" | "tel" | "url";

    // 输入框尺寸
    size?: "small" | "medium" | "large";

    // 占位符
    placeholder?: string;

    // 默认值
    defaultValue?: string;

    // 当前值（受控模式）
    value?: string;

    // 是否禁用
    disabled?: boolean;

    // 只读状态
    readonly?: boolean;

    // 最大长度
    maxLength?: number;

    // 是否显示字数统计
    showCount?: boolean;

    // 前缀图标
    prefixIcon?: string;

    // 后缀图标
    suffixIcon?: string;

    // 前缀内容
    prefix?: string | (() => VNode);

    // 后缀内容
    suffix?: string | (() => VNode);

    // 是否显示清除按钮
    allowClear?: boolean;

    // 是否显示密码可见性切换按钮（仅 type='password' 有效）
    showPassword?: boolean;

    // 自动聚焦
    autofocus?: boolean;
}
```

#### 使用示例

```tsx
// 基础输入框
<Input placeholder="Please input" />

// 不同类型
<Input type="password" placeholder="Password" />
<Input type="textarea" rows={4} placeholder="Textarea" />
<Input type="number" placeholder="Number" />

// 不同尺寸
<Input size="small" placeholder="Small" />
<Input size="medium" placeholder="Medium" />
<Input size="large" placeholder="Large" />

// 带图标
<Input prefixIcon="user" placeholder="Username" />
<Input suffixIcon="search" placeholder="Search" />

// 前后缀
<Input prefix="$" placeholder="Price" />
<Input suffix="@qq.com" placeholder="Email" />

// 清除按钮
<Input allowClear placeholder="With clear button" />

// 密码框
<Input type="password" showPassword placeholder="Password" />

// 字数统计
<Input maxLength={100} showCount placeholder="With count" />

// 文本域
<Input type="textarea" rows={4} maxLength={200} showCount placeholder="Textarea with count" />

// 禁用状态
<Input disabled placeholder="Disabled" />

// 只读状态
<Input readonly placeholder="Readonly" />
```

### Select 组件

#### API 设计

```typescript
interface SelectOption {
    label: string;
    value: any;
    disabled?: boolean;
    icon?: string;
}

interface SelectProps {
    // 选项数据
    options?: SelectOption[];

    // 当前值（单选）
    value?: any;

    // 当前值（多选）
    values?: any[];

    // 默认值
    defaultValue?: any;

    // 占位符

    placeholder?: string;

    // 是否多选
    multiple?: boolean;

    // 是否禁用
    disabled?: boolean;

    // 是否可清空
    allowClear?: boolean;

    // 尺寸
    size?: "small" | "medium" | "large";

    // 是否可搜索
    filterable?: boolean;

    // 自定义过滤函数
    filterMethod?: (query: string, option: SelectOption) => boolean;

    // 是否远程搜索
    remote?: boolean;

    // 远程搜索函数
    remoteMethod?: (query: string) => Promise<SelectOption[]>;

    // 最大选中数量（多选）
    maxMultipleCount?: number;

    // 是否显示全选（多选）
    showSelectAll?: boolean;

    // 自定义选项渲染
    optionRender?: (option: SelectOption) => VNode;
}
```

#### 使用示例

```tsx
// 基础选择器
const options = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2' },
  { label: 'Option 3', value: '3' }
];

<Select options={options} placeholder="Select an option" />

// 默认值
<Select options={options} defaultValue="1" />

// 受控模式
<Select options={options} value={selectedValue} onChange={handleChange} />

// 多选
<Select options={options} multiple placeholder="Select multiple" />

// 禁用选项
const optionsWithDisabled = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2', disabled: true },
  { label: 'Option 3', value: '3' }
];
<Select options={optionsWithDisabled} />

// 可搜索
<Select options={options} filterable placeholder="Search options" />

// 自定义过滤
<Select
  options={options}
  filterable
  filterMethod={(query, option) =>
    option.label.toLowerCase().includes(query.toLowerCase())
  }
/>

// 远程搜索
<Select
  remote
  filterable
  remoteMethod={async (query) => {
    const results = await fetchOptions(query);
    return results;
  }}
/>

// 带图标
const optionsWithIcons = [
  { label: 'User', value: 'user', icon: 'user' },
  { label: 'Settings', value: 'settings', icon: 'settings' },
  { label: 'Logout', value: 'logout', icon: 'logout' }
];
<Select options={optionsWithIcons} />

// 自最大选择数量
<Select options={options} multiple maxMultipleCount={3} />

// 全选功能
<Select options={options} multiple showSelectAll />

// 可清空
<Select options={options} allowClear />

// 不同尺寸
<Select options={options} size="small" />
<Select options={options} size="medium" />
<Select options={options} size="large" />

// 禁用状态
<Select options={options} disabled />
```

### Checkbox 组件

#### API 设计

```typescript
interface CheckboxProps {
    // 当前值
    value?: boolean;

    // 默认值
    defaultValue?: boolean;

    // 标签文本
    label?: string;

    // 是否禁用
    disabled?: boolean;

    // 是否半选状态（仅用于全选场景）
    indeterminate?: boolean;
}

interface CheckboxGroupProps {
    // 选项数据
    options?: Array<{
        label: string;
        value: any;
        disabled?: boolean;
    }>;

    // 当前值（多选）
    value?: any[];

    // 默认值
    defaultValue?: any[];

    // 是否禁用
    disabled?: boolean;

    // 布局方向
    direction?: "horizontal" | "vertical";
}
```

#### 使用示例

```tsx
// 基础复选框
<Checkbox>Checkbox</Checkbox>

// 默认值
<Checkbox defaultValue={true}>Checked by default</Checkbox>

// 受控模式
<Checkbox value={checked} onChange={handleChange}>Controlled</Checkbox>

// 禁用状态
<Checkbox disabled>Disabled</Checkbox>
<Checkbox disabled value={true}>Disabled and checked</Checkbox>

// 半选状态
<Checkbox indeterminate>Indeterminate</Checkbox>

// 复选框组
const options = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2' },
  { label: 'Option 3', value: '3' }
];

<CheckboxGroup options={options} />

// 默认值
<CheckboxGroup options={options} defaultValue={['1', '3']} />

// 受控模式
<CheckboxGroup
  options={options}
  value={selectedValues}
  onChange={handleChange}
/>

// 禁用选项
const optionsWithDisabled = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2', disabled: true },
  { label: 'Option 3', value: '3' }
];
<CheckboxGroup options={optionsWithDisabled} />

// 禁用整个组
<CheckboxGroup options={options} disabled />

// 垂直布局
<CheckboxGroup options={options} direction="vertical" />
```

### Radio 组件

#### API 设计

```typescript
interface RadioProps {
    // 当前值
    value?: boolean;

    // 默认值
    defaultValue?: boolean;

    // 标签文本
    label?: string;

    // 是否禁用
    disabled?: boolean;
}

interface RadioGroupProps {
    // 选项数据
    options?: Array<{
        label: string;
        value: any;
        disabled?: boolean;
    }>;

    // 当前值
    value?: any;

    // 默认值
    defaultValue?: any;

    // 是否禁用
    disabled?: boolean;

    // 布局方向
    direction?: "horizontal" | "vertical";
}
```

#### 使用示例

```tsx
// 基础单选框
<Radio>Radio</Radio>

// 默认值
<Radio defaultValue={true}>Checked by default</Radio>

// 受控模式
<Radio value={checked} onChange={handleChange}>Controlled</Radio>

// 禁用状态
<Radio disabled>Disabled</Radio>
<Radio disabled value={true}>Disabled and checked</Radio>

// 单选框组
const options = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2' },
  { label: 'Option 3', value: '3' }
];

<RadioGroup options={options} />

// 默认值
<RadioGroup options={options} defaultValue="1" />

// 受控模式
<RadioGroup
  options={options}
  value={selectedValue}
  onChange={handleChange}
/>

// 禁用选项
const optionsWithDisabled = [
  { label: 'Option 1', value: '1' },
  { label: 'Option 2', value: '2', disabled: true },
  { label: 'Option 3', value: '3' }
];
<RadioGroup options={optionsWithDisabled} />

// 禁用整个组
<RadioGroup options={options} disabled />

// 垂直布局
<RadioGroup options={options} direction="vertical" />
```

### Switch 组件

#### API 设计

```typescript
interface SwitchProps {
    // 当前值
    value?: boolean;

    // 默认值
    defaultValue?: boolean;

    // 禁用状态
    disabled?: boolean;

    // 尺寸
    size?: "small" | "medium" | "large";
}
```

#### 使用示例

```tsx
// 基础开关
<Switch />

// 默认值
<Switch defaultValue={true} />

// 受控模式
<Switch value={checked} onChange={handleChange} />

// 禁用状态
<Switch disabled />

// 不同尺寸
<Switch size="small" />
<Switch size="medium" />
<Switch size="large" />
```

## 导航和布局组件规范

### Menu 组件

#### API 设计

```typescript
interface MenuItem {
    key: string;
    label: string;
    icon?: string;
    disabled?: boolean;
    children?: MenuItem[];
}

interface MenuProps {
    // 菜单数据
    items?: MenuItem[];

    // 当前选中项
    selectedKey?: string;

    // 默认选中项
    defaultSelectedKey?: string;

    // 展开的子菜单
    openKeys?: string[];

    // 默认展开的子菜单
    defaultOpenKeys?: string[];

    // 菜单模式
    mode?: "vertical" | "horizontal" | "inline";

    // 主题
    theme?: "light" | "dark";

    // 是否只保持一个子菜单展开
    accordion?: boolean;
}
```

#### 使用示例

```tsx
// 基础菜单
const items = [
  { key: '1', label: 'Menu 1' },
  { key: '2', label: 'Menu 2' },
  { key: '3', label: 'Menu 3' }
];

<Menu items={items} />

// 默认选中
<Menu items={items} defaultSelectedKey="1" />

// 受受控模式
<Menu
  items={items}
  selectedKey={selectedKey}
  onClick={handleMenuClick}
/>

// 带图标
const itemsWithIcons = [
  { key: '1', label: 'Menu 1', icon: 'home' },
  { key: '2', label: 'Menu 2', icon: 'user' },
  { key: '3', label: 'Menu 3', icon: 'settings' }
];
<Menu items={itemsWithIcons} />

// 子菜单
const itemsWithChildren = [
  {
    key: '1',
    label: 'Menu 1',
    children: [
      { key: '1-1', label: 'Sub Menu 1' },
      { key: '1-2', label: 'Sub Menu 2' }
    ]
  },
  { key: '2', label: 'Menu 2' }
];
<Menu items={itemsWithChildren} />

// 默认展开
<Menu
  items={itemsWithChildren}
  defaultOpenKeys={['1']}
/>

// 受控展开
<Menu
  items={itemsWithChildren}
  openKeys={openKeys}
  onOpenChange={handleOpenChange}
/>

// 手风琴模式
<Menu items={itemsWithChildren} accordion />

// 水平菜单
<Menu items={items} mode="horizontal" />

// 内联菜单
<Menu items={items} mode="inline" />

// 深色主题
<Menu items={items} theme="dark" />

// 禁用项
const itemsWithDisabled = [
  { key: '1', label: 'Menu 1' },
  { key: '2', label: 'Menu 2', disabled: true },
  { key: '3', label: 'Menu 3' }
];
<Menu items={itemsWithDisabled} />
```

### Tabs 组件

#### API 设计

```typescript
interface TabItem {
    key: string;
    label: string;
    icon?: string;
    disabled?: boolean;
    closable?: boolean;
}

interface TabsProps {
    // 标签页数据
    items?: TabItem[];

    // 当前激活项
    activeKey?: string;

    // 默认激活项
    defaultActiveKey?: string;

    // 标签位置
    tabPosition?: "top" | "right" | "bottom" | "left";

    // 类型
    type?: "line" | "card" | "editable-card";

    // 是否可以新增
    addable?: boolean;

    // 是否可编辑
    editable?: boolean;
}
```

#### 使用示例

```tsx
// 基础标签页
const items = [
  { key: '1', label: 'Tab 1' },
  { key: '2', label: 'Tab 2' },
  { key: '3', label: 'Tab 3' }
];

<Tabs items={items} />

// 默认激活
<Tabs items={items} defaultActiveKey="1" />

// 受控模式
<Tabs
  items={items}
  activeKey={activeKey}
  onChange={handleTabChange}
/>

// 带图标
const itemsWithIcons = [
  { key: '1', label: 'Tab 1', icon: 'home' },
  { key: '2', label: 'Tab 2', icon: 'user' },
  { key: '3', label: 'Tab 3', icon: 'settings' }
];
<Tabs items={itemsWithIcons} />

// 卡片类型
<Tabs items={items} type="card" />

// 可编辑卡片
<Tabs
  items={items}
  type="editable-card"
  editable
  onEdit={(action, key) => {
    if (action === 'add') {
      handleAdd();
    } else if (action === 'remove') {
      handleRemove(key);
    }
  }}
/>

// 可新增
<Tabs items={items} type="editable-card" addable onEdit={handleEdit} />

// 可关闭
const closableItems = [
  { key: '1', label: 'Tab 1', closable: false },
  { key: '2', label: 'Tab 2', closable: true },
  { key: '3', label: 'Tab 3', closable: true }
];
<Tabs items={closableItems} type="editable-card" editable />

// 标签位置
<Tabs items={items} tabPosition="left" />
<Tabs items={items} tabPosition="right" />
<Tabs items={items} tabPosition="bottom" />

// 禁用项
const itemsWithDisabled = [
  { key: '1', label: 'Tab 1' },
  { key: '2', label: 'Tab 2', disabled: true },
  { key: '3', label: 'Tab 3' }
];
<Tabs items={itemsWithDisabled} />
```

### Breadcrumb 组件

#### API 设计

```typescript
interface BreadcrumbItem {
    key: string;
    label: string;
    icon?: string;
    href?: string;
    dropdownItems?: Array<{
        label: string;
        key: string;
        icon?: string;
    }>;
}

interface BreadcrumbProps {
    // 面包屑数据
    items?: BreadcrumbItem[];

    // 分隔符
    separator?: string | (() => VNode);
}
```

#### 使用示例

```tsx
// 基础面包屑
const items = [
  { key: '1', label: 'Home' },
  { key: '2', label: 'Category' },
  { key: '3', label: 'Sub Category' },
  { key: '4', label: 'Current Page' }
];

<Breadcrumb items={items} />

// 带图标
const itemsWithIcons = [
  { key: '1', label: 'Home', icon: 'home' },
  { key: '2', label: 'Category', icon: 'folder' },
  { key: '3', label: 'Sub Category', icon: 'folder-open' },
  { key: '4', label: 'Current Page', icon: 'file' }
];
<Breadcrumb items={itemsWithIcons} />

// 自定义分隔符
<Breadcrumb items={items} separator=">" />
<Breadcrumb items={items} separator={() => <span style={{ color: '#999' }}>/</span>} />

// 带下拉菜单
const itemsWithDropdown = [
  { key: '1', label: 'Home' },
  {
    key: '2',
    label: 'Category',
    dropdownItems: [
      { label: 'Category 1', key: 'c1' },
      { label: 'Category 2', key: 'c2' }
    ]
  },
  { key: '3', label: 'Current Page' }
];
<Breadcrumb items={itemsWithDropdown} />
```

### Dropdown 组件

#### API 设计

```typescript
interface DropdownItem {
    key: string;
    label: string;
    icon?: string;
    disabled?: boolean;
    divided?: boolean;
    children?: DropdownItem[];
}

interface DropdownProps {
    // 下拉菜单数据
    items?: DropdownItem[];

    // 触发方式
    trigger?: "hover" | "click" | "contextMenu";

    // 放置位置
    placement?: "top" | "bottom" | "left" | "right";

    // 是否禁用
    disabled?: boolean;
}
```

#### 使用示例

```tsx
// 基础下拉菜单
const items = [
  { key: '1', label: 'Menu Item 1' },
  { key: '2', label: 'Menu Item 2' },
  { key: '3', label: 'Menu Item 3' }
];

<Dropdown items={items}>
  <Button>Dropdown</Button>
</Dropdown>

// 带图标
const itemsWithIcons = [
  { key: '1', label: 'Menu Item 1', icon: 'user' },
  { key: '2', label: 'Menu Item 2', icon: 'settings' },
  { key: '3', label: 'Menu Item 3', icon: 'logout' }
];
<Dropdown items={itemsWithIcons}>
  <Button>Dropdown with icons</Button>
</Dropdown>

// 禁用项
const itemsWithDisabled = [
  { key: '1', label: 'Menu Item 1' },
  { key: '2', label: 'Menu Item 2', disabled: true },
  { key: '3', label: 'Menu Item 3' }
];
<Dropdown items={itemsWithDisabled}>
  <Button>Dropdown with disabled</Button>
</Dropdown>

// 分割线
const itemsWithDivider = [
  { key: '1', label: 'Menu Item 1' },
  { key: '2', label: 'Menu Item 2', divided: true },
  { key: '3', label: 'Menu Item 3' }
];
<Dropdown items={itemsWithDivider}>
  <Button>Dropdown with divider</Button>
</Dropdown>

// 子菜单
const itemsWithChildren = [
  { key: '1', label: 'Menu Item 1' },
  {
    key: '2',
    label: 'Sub Menu',
    children: [
      { key: '2-1', label: 'Sub Item 1' },
      { key: '2-2', label: 'Sub Item 2' }
    ]
  },
  { key: '3', label: 'Menu Item 3' }
];
<Dropdown items={itemsWithChildren}>
  <Button>Dropdown with sub menu</Button>
</Dropdown>

// 触发方式
<Dropdown items={items} trigger="hover">
  <Button>Hover to trigger</Button>
</Dropdown>
<Dropdown items={items} trigger="click">
  <Button>Click to trigger</Button>
</Dropdown>
<Dropdown items={items} trigger="contextMenu">
  <Button>Right click to trigger</Button>
</Dropdown>

// 放置位置
<Dropdown items={items} placement="top">
  <Button>Top placement</Button>
</Dropdown>
<Dropdown items={items} placement="left">
  <Button>Left placement</Button>
</Dropdown>
<Dropdown items={items} placement="right">
  <Button>Right placement</Button>
</Dropdown>

// 禁用状态
<Dropdown items={items} disabled>
  <Button disabled>Disabled dropdown</Button>
</Dropdown>
```

### Layout 组件

#### API 设计

```typescript
interface LayoutProps {
    // 布局方向
    direction?: "horizontal" | "vertical";

    // 间距
    spacing?: "small" | "medium" | "large" | number;

    // 是否换行
    wrap?: boolean;

    // 对齐方式
    align?: "start" | "center" | "end" | "stretch";

    // 主轴对齐方式
    justify?: "start" | "center" | "end" | "space-between" | "space-around";
}

interface GridProps {
    // 列数
    columns?: number;

    // 间距
    spacing?: "small" | "medium" | "large" | number;

    // 断点配置
    breakpoints?: {
        xs?: number;
        sm?: number;
        md?: number;
        lg?: number;
        xl?: number;
    };
}
```

#### 使用示例

```tsx
// 基础布局
<Layout>
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>

// 垂直布局
<Layout direction="vertical">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>

// 间距
<Layout spacing="small">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>
<Layout spacing="medium">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>
<Layout spacing="large">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>
<Layout spacing={20}>
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>

// 换行
<Layout wrap>
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
  <div>Item 4</div>
  <div>Item 5</div>
</Layout>

// 对齐方式
<Layout align="start">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout align="center">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout align="end">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout align="stretch">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>

// 主轴对齐方式
<Layout justify="start">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout justify="center">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout justify="end">
  <div>Item 1</div>
  <div>Item 2</div>
</Layout>
<Layout justify="space-between">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>
<Layout justify="space-around">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
</Layout>

// 网格布局
<Grid columns={3} spacing="medium">
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
  <div>Item 4</div>
  <div>Item 5</div>
  <div>Item 6</div>
</Grid>

// 响应式网格
<Grid
  columns={4}
  spacing="medium"
  breakpoints={{
    xs: 1,
    sm: 2,
    md: 3,
    lg: 4,
    xl: 6
  }}
>
  <div>Item 1</div>
  <div>Item 2</div>
  <div>Item 3</div>
  <div>Item 4</div>
</Grid>
```

## 反馈组件规范

### Modal 组件

#### API 设计

```typescript
interface ModalProps {
    // 是否可见
    visible?: boolean;

    // 标题
    title?: string | (() => VNode);

    // 内容
    content?: string | (() => VNode);

    // 宽度
    width?: number | string;

    // 是否显示遮罩
    mask?: boolean;

    // 是否点击遮罩关闭
    maskClosable?: boolean;

    // 是否显示关闭按钮
    closable?: boolean;

    // 是否支持键盘 ESC 关闭
    keyboard?: boolean;

    // 是否居中显示
    centered?: boolean;

    // 底部按钮配置
    footer?: Array<{
        text: string;
        type?: "default" | "primary" | "success" | "warning" | "danger";
        onClick: () => void;
    }> | null;

    // 确认按钮文字
    okText?: string;

    // 取消按钮文字
    cancelText?: string;

    // 确认按钮类型
    okType?: "default" | "primary" | "success" | "warning" | "danger";

    // 确认按钮加载状态
    confirmLoading?: boolean;
}
```

#### 使用示例

```tsx
// 基础模态框
<Modal
  visible={visible}
  title="Basic Modal"
  content="Modal content"
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 自定义宽度
<Modal
  visible={visible}
  title="Custom Width"
  content="Modal content"
  width={800}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 无遮罩
<Modal
  visible={visible}
  title="No Mask"
  content="Modal content"
  mask={false}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 点击遮罩不关闭
<Modal
  visible={visible}
  title="Mask Not Closable"
  content="Modal content"
  maskClosable={false}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 无关闭按钮
<Modal
  visible={visible}
  title="No Closable"
  content="Modal content"
  closable={false}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 不支持 ESC 关闭
<Modal
  visible={visible}
  title="No Keyboard"
  content="Modal content"
  keyboard={false}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 居中显示
<Modal
  visible={visible}
  title="Centered Modal"
  content="Modal content"
  centered
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 自定义底部按钮
<Modal
  visible={visible}
  title="Custom Footer"
  content="Modal content"
  footer={[
    { text: 'Save', type: 'primary', onClick: handleSave },
    { text: 'Cancel', onClick: handleCancel }
  ]}
/>

// 无底部按钮
<Modal
  visible={visible}
  title="No Footer"
  content="Modal content"
  footer={null}
/>

// 自定义按钮文字
<Modal
  visible={visible}
  title="Custom Button Text"
  content="Modal content"
  okText="Save"
  cancelText="Close"
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 确认按钮类型
<Modal
  visible={visible}
  title="Custom Ok Type"
  content="Modal content"
  okType="danger"
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 确认按钮加载状态
<Modal
  visible={visible}
  title="Confirm Loading"
  content="Modal content"
  confirmLoading={loading}
  onOk={handleOk}
  onCancel={handleCancel}
/>

// 异步确认
<Modal
  visible={visible}
  title="Async Confirm"
  content="Modal content"
  onOk={async () => {
    await handleAsyncOk();
  }}
  onCancel={handleCancel}
/>

// 自定义标题和内容
<Modal
  visible={visible}
  title={() => <div style={{ color: 'red' }}>Custom Title</div>}
  content={() => <div>Custom content with <strong>HTML</strong></div>}
  onOk={handleOk}
  onCancel={handleCancel}
/>
```

### Message 组件

#### API 设计

```typescript
interface MessageProps {
    // 消息类型
    type?: "info" | "success" | "warning" | "error" | "loading";

    // 消息内容
    content: string | (() => VNode);

    // 显示时长（秒）
    duration?: number;

    // 是否显示关闭按钮
    closable?: boolean;

    // 自定义图标
    icon?: string | (() => VNode);
}

interface MessageAPI {
    // 显示消息
    show(props: MessageProps): { close: () => void };

    // 成功消息
    success(content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 错误消息
    error(content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 警告消息
    warning(content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 信息消息
    info(content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 加载消息
    loading(content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 配置
    config(options: { duration?: number; top?: number; closable?: boolean }): void;

    // 销毁所有消息
    destroy(): void;
}
```

#### 使用示例

```tsx
// 基础消息
message.show({
    type: "info",
    content: "This is a message",
});

// 成功消息
message.success("Success message");
message.success("Success message", 3);
message.success("Success message", 3, () => console.log("Message closed"));

// 错误消息
message.error("Error message");
message.error("Error message", 5);

// 警告消息
message.warning("Warning message");

// 信息消息
message.info("Info message");

// 加载消息
const loading = message.loading("Loading message");
setTimeout(() => {
    loading.close();
}, 3000);

// 自定义时长
message.show({
    type: "info",
    content: "Custom duration",
    duration: 10,
});

// 可关闭
message.show({
    type: "info",
    content: "Closable message",
    closable: true,
});

// 自定义图标
message.show({
    type: "info",
    content: "Custom icon",
    icon: "smile",
});

message.show({
    type: "info",
    content: "Custom icon component",
    icon: () => <span style={{ color: "red" }}>❤️</span>,
});

// 自定义内容
message.show({
    type: "info",
    content: () => (
        <div>
            <strong>Custom</strong> content with <em>HTML</em>
        </div>
    ),
});

// 关闭事件
message.show({
    type: "info",
    content: "Message with close event",
    onClose: () => console.log("Message closed"),
});

// 配置
message.config({
    duration: 5,
    top: 100,
    closable: true,
});

// 销毁所有消息
message.destroy();
```

### Notification 组件

#### API 设计

```typescript
interface NotificationProps {
    // 通知类型
    type?: "info" | "success" | "warning" | "error";

    // 通知标题
    title?: string | (() => VNode);

    // 通知内容
    content: string | (() => VNode);

    // 显示时长（秒）
    duration?: number;

    // 是否显示关闭按钮
    closable?: boolean;

    // 自定义图标
    icon?: string | (() => VNode);

    // 放置位置
    placement?: "topLeft" | "topRight" | "bottomLeft" | "bottomRight";
}

interface NotificationAPI {
    // 显示通知
    show(props: NotificationProps): { close: () => void };

    // 成功通知
    success(title: string, content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 错误通知
    error(title: string, content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 警告通知
    warning(title: string, content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 信息通知
    info(title: string, content: string, duration?: number, onClose?: () => void): { close: () => void };

    // 配置
    config(options: {
        duration?: number;
        top?: number;
        bottom?: number;
        placement?: "topLeft" | "topRight" | "bottomLeft" | "bottomRight";
        closable?: boolean;
    }): void;

    // 销毁所有通知
    destroy(): void;
}
```

#### 使用示例

```tsx
// 基础通知
notification.show({
  type: 'info',
  title: 'Notification Title',
  content: 'This is a notification'
});

// 成功通知
notification.success('Success', 'Success notification');
notification.success('Success', 'Success notification', 3);
notification.success('Success', 'Success notification', 3, () => console.log('Notification closed'));

// 错误通知
notification.error('Error', 'Error notification');
notification.error('Error', 'Error notification', 5);

// 警告通知
notification.warning('Warning', 'Warning notification');

// 信息通知
notification.info('Info', 'Info notification');

// 自定义时长
notification.show({
  type: 'info',
  title: 'Custom duration',
  content: 'Custom duration notification',
  duration: 10
});

// 可关闭
notification.show({
  type: 'info',
  title: 'Closable notification',
  content: 'Closable notification',
  closable: true
});

// 自定义图标
notification.show({
  type: 'info',
  title: 'Custom icon',
  content: 'Custom icon notification',
  icon: 'smile'
});

notification.show({
  type: 'info',
  title: 'Custom icon component',
  content: 'Custom icon component' notification',
  icon: () => <span style={{ color: 'red' }}>❤️</span>
});

// 自定义内容
notification.show({
  type: 'info',
  title: () => <strong>Custom Title</strong>,
  content: () => (
    <div>
      <strong>Custom</strong> content with <em>HTML</em>
    </div>
  )
});

// 放置位置
notification.show({
  type: 'info',
  title: 'Top left',
  content: 'Top left notification',
  placement: 'topLeft'
});

notification.show({
  type: 'info',
  title: 'Top right',
  content: 'Top right notification',
  placement: 'topRight'
});

notification.show({
  type: 'info',
  title: 'Bottom left',
  content: 'Bottom left notification',
  placement: 'bottomLeft'
});

notification.show({
  type: 'info',
  title: 'Bottom right',
  content: 'Bottom right notification',
  placement: 'bottomRight'
});

// 关闭事件
notification.show({
  type: 'info',
  title: 'Notification with close event',
  content: 'Notification with close event',
  onClose: () => console.log('Notification closed')
});

// 配置
notification.config({
  duration: 5,
  top: 100,
  bottom: 50,
  placement: 'topRight',
  closable: true
});

// 销毁所有通知
notification.destroy();
```

### Progress 组件

#### API 设计

```typescript
interface ProgressProps {
    // 进度百分比
    percent?: number;

    // 状态
    status?: "normal" | "success" | "exception" | "active";

    // 类型
    type?: "line" | "circle" | "dashboard";

    // 尺寸
    size?: "small" | "medium" | "large";

    // 是否显示进度文本
    showText?: boolean;

    // 自定义文本格式
    format?: (percent: number) => string;

    // 线条宽度（px）
    strokeWidth?: number;

    // 线条颜色
    strokeColor?: string | string[];

    // 轨道颜色
    trailColor?: string;

    // 圆形进度条宽度（px）
    width?: number;

    // 是否显示成功图标
    showSuccessIcon?: boolean;
}
```

#### 使用示例

```tsx
// 基础进度条
<Progress percent={50} />

// 不同状态
<Progress percent={30} status="normal" />
<Progress percent={50} status="active" />
<Progress percent={70} status="exception" />
<Progress percent={100} status="success" />

// 不同类型
<Progress percent={50} type="line" />
<Progress percent={50} type="circle" />
<Progress percent={50} type="dashboard" />

// 不同尺寸
<Progress percent={50} size="small" />
<Progress percent={50} size="medium" />
<Progress percent={50} size="large" />

// 不显示文本
<Progress percent={50} showText={false} />

// 自定义文本格式
<Progress
  percent={50}
  format={(percent) => `${percent} days`}
/>

// 自定义线条宽度
<Progress percent={50} strokeWidth={10} />

// 自定义颜色
<Progress percent={50} strokeColor="#52c41a" />
<Progress
  percent={50}
  strokeColor={['#52c41a', '#faad14']}
/>

// 自定义轨道颜色
<Progress percent={50} trailColor="#f0f0f0" />

// 圆形进度条宽度
<Progress percent={50} type="circle" width={80} />

// 显示成功图标
<Progress percent={100} showSuccessIcon />

// 动态进度
const [percent, setPercent] = useState(0);

useEffect(() => {
  const timer = setInterval(() => {
    setPercent((prev) => {
      if (prev >= 100) {
        clearInterval(timer);
        return 100;
      }
      return prev + 10;
    });
  }, 1000);

  return () => clearInterval(timer);
}, []);

<Progress percent={percent} />
```

### Spin 组件

#### API 设计

```typescript
interface SpinProps {
    // 是否加载中
    spinning?: boolean;

    // 尺寸
    size?: "small" | "medium" | "large";

    // 自定义指示符
    indicator?: (() => VNode) | VNode;

    // 提示文字
    tip?: string;

    // 延迟显示（毫秒）
    delay?: number;
}
```

#### 使用示例

```tsx
// 基础加载中
<Spin />

// 不同尺寸
<Spin size="small" />
<Spin size="medium" />
<Spin size="large" />

// 加载状态
<Spin spinning={loading} />

// 自定义指示符
<Spin indicator={<div className="custom-spinner" />} />
<Spin indicator={() => <div className="custom-spinner" />} />

// 提示文字
<Spin tip="Loading..." />

// 延迟显示
<Spin spinning={loading} delay={500} />

// 包裹内容
<Spin spinning={loading}>
  <div>Content to be loaded</div>
</Spin>
```
