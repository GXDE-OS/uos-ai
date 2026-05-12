# UOS AI 前端编码规范

## 项目架构

### 技术栈
- **框架**: Vue 3 + TypeScript + TSX（优先使用 TSX 编写组件）
- **构建**: Vite
- **状态管理**: Pinia（按功能模块拆分 store，禁止全局大 store）
- **UI 框架**: Element Plus

### 目录结构
```
src/
├── assets/
│   ├── styles/    # 样式文件，按组件分文件
│   └── icons/     # 图标文件
├── stores/        # Pinia 状态管理（按功能拆分）
├── types/         # TypeScript 类型定义
├── components/    # 基础可复用组件
├── utils/         # 工具函数
└── views/         # 功能页面
```

---

## 组件规范

### 组件基本结构
组件必须使用 TSX 编写，遵循以下结构：

```tsx
export default defineComponent({
  name: 'ComponentName',
  props: { /* 定义 */ },
  emits: { /* 定义 */ },
  setup(props, { emit }) {
    // 业务逻辑
    return { /* 返回模板需要的变量和方法 */ };
  },
  render() {
    // 渲染模板
  }
});
```

### Props 定义规范
- Props 是父组件向子组件传递数据的接口
- **Props 应该是只读的**，子组件不应直接修改 Props 的值
- 使用类型定义确保传递数据的正确性
- 支持：基础类型、数组（需要 PropType）、对象

### Emits 定义规范
- Emits 是子组件向父组件通信的机制
- **必须使用小驼峰命名**，动词开头
- 信号名 `eventName` 对应响应名 `onEventName`（on + 首字母大写）

### Setup 函数规范
- 使用 `ref`、`reactive` 定义响应式数据
- 使用 `computed` 定义计算属性
- 方法定义使用箭头函数
- 生命周期钩子（onMounted、onUpdated 等）
- **必须返回对象**，包含模板中需要使用的所有内容

### Render 函数规范
- 通过 `this` 访问 setup 返回的内容
- 通过 `this.$props` 访问 props
- 通过 `this.$slots` 访问插槽

---

## TSX 语法规范

### 基础规则
1. **根节点**：每个组件必须有且只有一个根元素
2. **类名属性**：使用 `class` 而不是 `className`
3. **事件处理**：使用 `onClick`、`onInput` 等驼峰命名（Vue 的 `@` 语法不可用）
4. **条件渲染**：使用 JavaScript 表达式（三元运算符或逻辑与）
5. **列表渲染**：使用 `map()` 并为每个元素添加 `key` 属性

### 事件绑定
```tsx
// ✅ 传递函数引用
<button onClick={this.handleClick}>

// ❌ 立即执行（错误）
<button onClick={this.handleClick()}>

// ✅ 需要传参时使用箭头函数
<button onClick={() => this.handleClick(id)}>
```

### v-model 实现
JSX 中不能使用 v-model 指令，需手动实现：
```tsx
<input
  value={text}
  onInput={e => text = e.target.value}
/>
```

### 插槽使用
```tsx
<MyComponent>
  {{
    default: () => '默认内容',
    header: () => <h1>标题</h1>
  }}
</MyComponent>
```

---

## Store (Pinia) 规范

### 核心职责
- **State**: 存储应用状态，响应式数据
- **Getters**: 从 state 派生的计算值，缓存结果
- **Actions**: 包含业务逻辑，可异步操作和修改 state

### 使用规范
1. **按功能模块拆分**：一个 store 对应一个业务领域
2. **禁止全局大 store**：避免将所有状态放在一个 store 中
3. **一个 action 只做一件事**，清晰命名，处理错误

---

## 命名规范

| 类型 | 约定 | 示例 |
|------|------|------|
| 变量 | 小驼峰 | `userName`, `isLoading` |
| 函数 | 小驼峰 | `fetchData`, `handleClick` |
| 常量 | 全大写 | `MAX_COUNT`, `API_URL` |
| 接口/类型 | 大驼峰 | `UserInfo`, `UserProfile` |
| 组件 | 大驼峰 | `BaseButton`, `ModalDialog` |
| Emits 信号 | 小驼峰 | `submitForm`, `updateValue` |

---

## 代码格式要求

1. **避免内联函数**：直接传递函数引用
2. **布尔属性直接绑定**：`<button disabled={isDisabled}>`
3. **样式绑定使用对象语法**：`<div style={{ color: active ? 'red' : 'black' }}>`
4. **条件渲染使用三元或逻辑与**：`{isVisible && <div>内容</div>}`
5. **列表渲染使用 map 并添加 key**：`{items.map(item => <li key={item.id}>{item.name}</li>)}`

---

## 导入顺序

```typescript
// 1. Vue 核心
import { defineComponent, ref } from 'vue';

// 2. Vue 类型
import type { PropType } from 'vue';

// 3. 第三方库
import axios from 'axios';

// 4. 项目类型
import type { User } from '@/types';

// 5. 项目组件
import BaseButton from '@/components/BaseButton';

// 6. 项目工具
import { formatDate } from '@/utils';
```

**重要**: 禁止使用相对路径访问上级目录（如 `../../Chat`），应使用 `@/` 别名。

---

## 组件拆分原则

- **小型单一组件**：每个组件只负责一个功能
- **组合使用**：通过组合基础组件构建复杂功能
- 避免组件过大或职责不清

---

## 样式文件规范

采用 **BEM 命名规范**（Block-Element-Modifier），样式文件按组件分文件存放于 `assets/styles/`，所有子组件的样式在 `main.css` 中引入。

### CSS 命名约定

为了避免不同组件 CSS 文件中同名 class 被覆盖，所有组件必须遵循以下 CSS 命名约定：

#### 1. BEM 命名规范

使用 BEM（Block Element Modifier）命名风格，格式：`block__element--modifier`

```css
/* Block（组件根元素） */
.dropdown {}

/* Element（组件子元素） */
.dropdown__trigger {}
.dropdown__menu {}
.dropdown__item {}

/* Modifier（状态变体） */
.dropdown__item--disabled {}
.dropdown__item--active {}
.dropdown__item--selected {}
```

#### 2. 组件隔离

- 每个 CSS 文件只包含一个主组件的样式
- 主组件 root class 必须是唯一的，建议使用组件名 kebab-case
- 示例：`icon-button`、`scroll-bar`、`drop-down`

#### 3. 嵌套书写规范（CSS Nesting）

使用原生 CSS 嵌套（CSS Nesting）时，按组件结构进行嵌套，选择器应包含完整的组件 block 名称：

```css
.icon-button {
    display: flex;
    align-items: center;

    /* Element 层 */
    .icon-button__icon {
        width: 24px;
        height: 24px;
    }

    .icon-button__text {
        margin-left: 8px;
    }

    /* Modifier 层 */
    .icon-button--small {
        width: 24px;
        height: 24px;
    }

    .icon-button--large {
        width: 48px;
        height: 48px;
    }

    /* Hover/Active 等伪类 */
    .icon-button:hover:not(.icon-button--disabled) {
        background: rgba(0, 0, 0, 0.05);
    }

    .icon-button:active:not(.icon-button--disabled) {
        background: rgba(0, 0, 0, 0.1);
    }

    .icon-button--disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }
}
```

**注意**：在嵌套中使用其他元素选择器时，必须保留完整的 class 名称，避免使用 `&` 符号（除非明确清楚其作用）。

#### 4. 避免全局污染

- 禁止使用标签选择器（如 `div {}`、`span {}`）
- 禁止使用过于通用的 class 名（如 `.active`、`.selected`）
- 所有选择器应包含组件 block 名称

```css
/* ❌ 错误：过于通用 */
.active {
    color: blue;
}

/* ✅ 正确：组件限定 */
.dropdown__item--active {
    color: blue;
}

/* ❌ 错误：标签选择器 */
.icon-button span {
    font-size: 14px;
}

/* ✅ 正确：使用 element */
.icon-button__text {
    font-size: 14px;
}
```

#### 5. 特殊状态类命名

通用状态类加上组件前缀避免冲突：

```css
/* 通用状态 */
.dropdown--loading {}
.dropdown--error {}
.dropdown--success {}
.dropdown--has-icon {}

/* 交互状态 */
.dropdown--focused {}
.dropdown--hovered {}
.dropdown--active {}
```

#### 6. 复杂组件的子组件处理

对于包含复杂子组件的组件，子组件使用独立的 block 命名：

```css
/* 父组件 */
.dropdown { }

.dropdown__trigger { }

.dropdown__menu { }

/* 独立的子组件（如 Menu 组件复用） */
.dropdown-menu { }

.dropdown-menu__item { }

.dropdown-menu__item--active { }
```

#### 7. 公共工具类

公共工具类使用通用前缀 `u-`（utility），放在独立文件（如 `utils.css`）：

```css
.u-margin-top { }
.u-flex-center { }
.u-text-truncate { }
```

### 主题颜色变量规范

为支持亮色/暗色双主题系统，所有组件必须使用 CSS 变量定义颜色值，遵循以下规范：

#### 1. 变量定义位置

每个 CSS 文件在文件顶部定义自己的主题变量，采用模块化设计：

```css
/* [组件名] 主题变量 */
html {
    /* 亮色主题变量 */
    --[组件名]-[属性名]: [亮色值];
    --[组件名]-[属性名]-hover: [亮色悬停值];
    --[组件名]-[属性名]-active: [亮色激活值];
}

html.dark {
    /* 暗色主题变量 */
    --[组件名]-[属性名]: [暗色值];
    --[组件名]-[属性名]-hover: [暗色悬停值];
    --[组件名]-[属性名]-active: [暗色激活值];
}
```

#### 2. 命名规范

- **组件级变量**：`--[组件名]-[属性]`（如 `--button-background`）
- **通用变量**：`--uosai-color-[类别]-[属性]`（如 `--uosai-color-text-primary`）
- **状态变量**：添加 `-hover`、`-active`、`-disabled` 后缀

#### 3. 颜色值格式

- 优先使用 `rgba()` 以便控制透明度
- 对于纯色可以使用 `#hex` 或 `rgb()`
- 对于渐变使用 `linear-gradient()`

#### 4. 使用示例

```css
/* CheckButton 主题变量 */
html {
    --checkbutton-border-color: rgba(0, 0, 0, 0.18);
    --checkbutton-background: rgba(255, 255, 255, 0.96);
    --checkbutton-box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.08);
    --checkbutton-hover-border-color: rgba(0, 0, 0, 0.28);
    --checkbutton-active-border-color: rgba(0, 0, 0, 0.32);
}

html.dark {
    --checkbutton-border-color: rgba(255, 255, 255, 0.2);
    --checkbutton-background: rgba(255, 255, 255, 0.08);
    --checkbutton-box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.4);
    --checkbutton-hover-border-color: rgba(255, 255, 255, 0.3);
    --checkbutton-active-border-color: rgba(255, 255, 255, 0.36);
}

.check-button {
    border: 1px solid var(--checkbutton-border-color);
    background: var(--checkbutton-background);
    box-shadow: var(--checkbutton-box-shadow);
}

.check-button:hover:not(.check-button--checked):not(.check-button--disabled) {
    background: rgba(0, 0, 0, var(--bg-opacity-hover));
    border-color: var(--checkbutton-hover-border-color);
}
```

#### 5. 通用主题变量

在 `base.css` 中定义通用设计系统变量，所有组件均可使用：

```css
/* 基础颜色变量 */
--uosai-color-primary: rgba(31, 110, 231, 1);
--uosai-color-primary-hover: rgba(31, 110, 231, 0.8);
--uosai-color-primary-active: rgba(31, 110, 231, 0.6);

/* 文本颜色 */
--uosai-color-text-primary: rgba(0, 0, 0, 0.9);
--uosai-color-text-secondary: rgba(0, 0, 0, 0.7);
--uosai-color-text-tertiary: rgba(0, 0, 0, 0.5);
--uosai-color-text-disabled: rgba(0, 0, 0, 0.3);

/* 背景颜色 */
--uosai-color-background-primary: rgba(255, 255, 255, 1);
--uosai-color-background-secondary: rgba(245, 245, 245, 1);
--uosai-color-background-tertiary: rgba(235, 235, 235, 1);

/* 边框颜色 */
--uosai-color-border-primary: rgba(0, 0, 0, 0.1);
--uosai-color-border-secondary: rgba(0, 0, 0, 0.05);
--uosai-color-border-tertiary: rgba(0, 0, 0, 0.03);

/* 阴影 */
--uosai-shadow-small: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
--uosai-shadow-medium: 0 2px 4px 0 rgba(0, 0, 0, 0.1);
--uosai-shadow-large: 0 4px 8px 0 rgba(0, 0, 0, 0.15);

/* 交互状态 */
--uosai-color-hover-bg: rgba(0, 0, 0, 0.05);
--uosai-color-active-bg: rgba(0, 0, 0, 0.1);
--uosai-color-selected-bg: rgba(0, 0, 0, 0.08);
```

#### 6. 暗色模式适配

- 使用 `@media (prefers-color-scheme: dark)` 媒体查询
- 在暗色模式下重新定义所有颜色变量
- 保持相同的变量名，只改变值
- 不只是反转颜色，要考虑视觉层次和可读性

#### 7. 字号设置规范

所有组件的字号设置必须使用 **rem** 作为单位，以确保在不同设备和浏览器缩放级别下的可访问性。

**为什么使用 rem：**
- `rem` 是相对于根元素（html）字体大小的单位
- 支持浏览器缩放和用户自定义字体大小偏好
- 比 `px` 具有更好的响应式表现
- 比 `em` 更容易维护，不会产生嵌套累积问题

**标准字号值：**

```css
/* 标准字号scale */
--uosai-font-size-xs: 0.857rem;   /* 12px - 辅助信息 */
--uosai-font-size-sm: 0.929rem;   /* 13px - 小字号文本 */
--uosai-font-size-base: 1rem;     /* 14px - 正文（基准） */
--uosai-font-size-md: 1.143rem;   /* 16px - 中等字号 */
--uosai-font-size-lg: 1.286rem;   /* 18px - 小标题 */
--uosai-font-size-xl: 1.429rem;   /* 20px - 重要标题 */
--uosai-font-size-3xl: 1.714rem;  /* 24px - 大标题 */
--uosai-font-size-5xl: 2rem;      /* 28px - 页面标题 */

/* 行高 */
--uosai-line-height-tight: 1.25;
--uosai-line-height-normal: 1.5;
--uosai-line-height-relaxed: 1.75;
--uosai-line-height-loose: 2;
```

**使用示例：**

```css
/* 组件中使用标准字号 */
html {
    --button-font-size: var(--uosai-font-size-sm);
    --dialog-title-font-size: var(--uosai-font-size-lg);
    --dialog-content-font-size: var(--uosai-font-size-base);
}

.button {
    font-size: var(--button-font-size);
}

.dialog__title {
    font-size: var(--dialog-title-font-size);
    line-height: var(--uosai-line-height-tight);
}

.dialog__content {
    font-size: var(--dialog-content-font-size);
    line-height: var(--uosai-line-height-normal);
}
```

**使用原则：**
1. **优先使用 CSS 变量**：通过 CSS 变量引用标准字号值，便于统一调整
2. **特殊场景可自定义**：对于特殊需求，可以定义组件级字号变量
3. **配合行高使用**：字号和行高应该成对设置，确保良好的阅读体验
4. **禁用 px 单位**：除非有明确的像素级精确需求（如边框、间距），字号禁止使用 px

### 总结

**Golden Rule**: 每个 class 名都应清楚地表达"它属于哪个组件"以及"它是该组件的哪种元素或变体"。所有颜色值必须使用 CSS 变量定义，支持亮色/暗色双主题系统。

通过遵循以上约定，可以有效避免：
- 样式意外覆盖
- 组件样式冲突
- 难以追踪的样式问题
- 代码可维护性下降
- 主题切换不一致的问题

---

## 错误处理

- 使用 try-catch 捕获异步操作错误
- 通过 emit 向父组件传递错误
- 统一的错误日志记录

---

## 记忆口诀（Vue 模板 → TSX 转换）

- **冒号变花括号** `:` → `{}`
- **@变on加驼峰** `@click` → `onClick`
- **v-if变三目** `v-if` → `? :`
- **v-for变map** `v-for` → `.map()`
- **v-model自己写** 双向绑定需手动实现
