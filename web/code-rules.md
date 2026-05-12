# UOS AI前端编码
## 规范与说明
### 文件结构

```
src/
├── assets/          # 资源文件
│	├── styles/      # 样式文件，按组件分文件
│	└── icons/       # 图标文件 
├── stores/          # Pinia 状态管理
├── types/           # TypeScript 类型定义
├── components/      # 基础可复用组件
├── utils/           # 工具
└── views/           # 功能页面
```

### 组件基本结构

组件优先使用`tsx`编写。

#### TSX 介绍与特性
**TSX** 是 TypeScript 版的 JSX，允许在 TypeScript 文件中直接编写类似 HTML 的模板语法。主要特性包括：

- **类型安全**：在编译时检查组件属性和事件的类型
- **组件化**：支持创建可复用的 UI 组件
- **声明式**：描述 UI 应该是什么样子，而不是如何操作 DOM
- **JavaScript 表达能力**：可以在模板中直接使用 JavaScript 表达式

#### 语法规则
1. **根节点**：每个组件必须有且只有一个根元素
2. **类名属性**：使用 `class` 而不是 `className`
3. **样式属性**：使用 `style` 对象或字符串
4. **事件处理**：使用 `onClick`、`onInput` 等驼峰命名
5. **条件渲染**：使用 JavaScript 表达式（三元运算符或逻辑与）
6. **列表渲染**：使用 `map()` 并为每个元素添加 `key` 属性

```tsx
import { defineComponent, ref } from 'vue';
import type { PropType } from 'vue';

export default defineComponent({
  name: 'ComponentName',
  
  props: {
    // Props 定义
  },
  
  emits: {
    // 事件定义
  },
  
  setup(props, { emit }) {
    // 业务逻辑
  },
  
  render() {
    // 渲染模板
  }
});
```

### Props 定义规范

- Props 是父组件向子组件传递数据的接口。
- 在 Vue 中，Props 应该是**只读**的，子组件不应直接修改 Props 的值。
- 通过类型定义确保传递数据的正确性。

#### Props 的传入方式
1. **静态传入**：直接在模板中传递固定值
```tsx
<ChildComponent title="用户列表" count={10} />
```
2. **动态传入**： 传递响应式数据
```tsx
<ChildComponent items={this.userLis} config={this.pageConfig} />
```

#### Props 定义示例
```tsx
props: {
  // 基础类型
  title: {
    type: String,
    required: true,
    default: ''
  },
  
  // 数字类型
  count: Number,
  
  // 布尔类型简写
  isActive: Boolean,
  
  // 数组类型（需要 PropType）
  items: {
    type: Array as PropType<string[]>,
    default: () => []
  },
  
  // 对象类型
  config: {
    type: Object as PropType<Record<string, any>>,
    default: () => ({})
  }
}
```

### Emits 定义规范

- Emits 是子组件向父组件通信的机制，用于触发自定义事件。
- 父组件可以监听这些事件并执行相应的处理函数。

#### 信号命名规范
- **只能使用小驼峰**：如 `submitForm`、`updateValue`、`closeModal`
- 动词开头，描述动作行为
- 明确表达事件意图

```tsx
emits: {
  // 无参数事件
  close: null,
  
  // 带参数事件（需要验证器）
  submit: (id: number, data: string) => {
    return typeof id === 'number' && typeof data === 'string';
  },

  // 小驼峰命名示例
  updateValue: (value: string) => true,
  submitForm: (formData: object) => true,
  toggleStatus: (status: boolean) => true
}
```

#### 如何响应信号

渲染模板在`tsx`的`render`函数中返回，因此不能使用`vue`的`@`语法，

```vue
// vue
<ChildComponent @submit="handleSubmit" @close="handleClose" />
```

只能使用`onXxx`语法来响应信号：

```tsx
// tsx
render() {
  return (
    <ChildComponent onSubmit={this.handleSubmit} onClose={this.handleClose} />
  );
}
```

信号名：`eventName` 

对应的响应名:`onEventName`， 即`on` + 首字母大写的信号名

### Setup 函数规范

#### Setup 函数的作用
`setup` 是 Composition API 的入口函数，在组件创建之前执行，用于：

- 定义响应式数据（`ref`、`reactive`）
- 定义计算属性（`computed`）
- 定义方法（普通函数）
- 监听属性变化（`watch`、`watchEffect`）
- 生命周期钩子（`onMounted`、`onUpdated` 等）
- 访问插槽和属性（`slots`、`attrs`）

#### 需要返回的内容
`setup` 函数必须返回一个对象，包含模板中需要使用的所有属性和方法：

```tsx
setup(props, { emit, attrs, slots }) {
  // 使用 ref 定义响应式数据
  const count = ref(0);
  const isLoading = ref(false);
  
  // 计算属性
  const doubledCount = computed(() => count.value * 2);
  
  // 方法定义使用箭头函数
  const handleClick = () => {
    emit('click', count.value);
  };
  
  // 生命周期钩子
  onMounted(() => {
    console.log('组件已挂载');
  });
  
  // 返回给模板的所有内容
  return {
    count,
    isLoading,
    doubledCount,
    handleClick
  };
}
```

### Render 函数规范

#### 如何使用 Setup 中的函数和变量
在 `render` 函数中，通过 `this` 访问 `setup` 返回的所有内容：
- 变量：`this.count`、`this.isLoading`
- 计算属性：`this.doubledCount`
- 方法：`this.handleClick`

#### 如何与变量绑定
```tsx
render() {
  return (
    <div class="component">
      {/* 条件渲染 */}
      {this.isLoading && <div>Loading...</div>}
      
      {/* 属性渲染 */}
      <span>{this.title}</span>
      
      {/* 计算属性渲染 */}
      <span>双倍值: {this.doubledCount}</span>
      
      {/* 列表渲染 */}
      <ul>
        {this.items.map(item => (
          <li key={item.id}>{item.name}</li>
        ))}
      </ul>
      
      {/* 事件绑定 - 使用setup中的方法 */}
      <button onClick={this.handleClick}>
        Click Me
      </button>
      
      {/* 动态样式绑定 */}
      <div class={{ active: this.isActive, disabled: this.isDisabled }}>
        动态类名
      </div>
      
      {/* 动态属性绑定 */}
      <input value={this.inputValue} onInput={this.handleInput} />
      
      {/* Slot 使用 */}
      {this.$slots.default?.()}
          
      {/* props 使用 */}
      {this.$props.title}
    </div>
  );
}
```

### Store 定义规范

#### Store 各部分的职责

- **职责**：存储应用程序的状态数据
- **特点**：响应式数据，状态变化会自动更新视图
- **规范**：定义清晰的初始状态，避免嵌套过深
```typescript
state: () => ({
  // 应用状态
  user: null as User | null,
  items: [] as Item[],
  loading: false,
  error: null as string | null
})
```

#### Getters
- **职责**：从 state 派生的计算值（类似于计算属性）
- **特点**：缓存结果，只有依赖变化时才重新计算
- **规范**：用于复杂的状态计算，保持纯函数特性
```typescript
getters: {
  // 派生状态
  totalItems: (state) => state.items.length,
  
  // 带参数的 getter
  getItemById: (state) => (id: number) => {
    return state.items.find(item => item.id === id);
  },
  
  // 组合其他 getter
  summary: (state): string => {
    return `共 ${state.items.length} 个项目`;
  }
}
```

#### Actions
- **职责**：包含业务逻辑，可以异步操作和修改 state
- **特点**：可以处理异步操作，通过 `this` 访问整个 store
- **规范**：一个 action 只做一件事，清晰命名，处理错误
```typescript
actions: {
  // 同步 action
  addItem(item: Item) {
    this.items.push(item);
  },
  
  // 异步 action
  async fetchItems() {
    this.loading = true;
    try {
      const response = await api.getItems();
      this.items = response.data;
    } catch (error) {
      this.error = error.message;
      throw error;
    } finally {
      this.loading = false;
    }
  },
  
  // 组合多个 actions
  async refreshData() {
    await this.reset();
    await this.fetchItems();
  }
}
```

#### 使用规范
1. **按功能模块拆分**：一个 store 对应一个业务领域
2. **禁止全局大store**：避免将所有状态放在一个 store 中
3. **组件中使用**：
```typescript
// stores/counter.ts
import { defineStore } from 'pinia';

export const useCounterStore = defineStore('counter', {
  state: () => ({
    count: 0
  }),
  
  getters: {
    doubleCount: (state) => state.count * 2,
    formattedCount: (state) => `当前计数: ${state.count}`
  },
  
  actions: {
    increment() {
      this.count++;
    },
    
    decrement() {
      if (this.count > 0) {
        this.count--;
      }
    },
    
    async incrementAsync() {
      await new Promise(resolve => setTimeout(resolve, 1000));
      this.increment();
    }
  }
});
```

### 类型定义规范

#### types/user.ts
```typescript
// 接口使用大驼峰
export interface UserInfo {
  id: number;
  name: string;
  email: string;
}

// 类型别名
export type UserStatus = 'active' | 'inactive' | 'pending';

// 联合类型
export type ButtonSize = 'small' | 'medium' | 'large';
```

### 样式文件规范

#### assets/styles/Button.css
```css
/* BEM 命名规范 */
.button {
  padding: 8px 16px;
}

.button--primary {
  background: blue;
}

.button--disabled {
  opacity: 0.5;
}
```

#### 引入样式：

所有子组件的样式在`main.css`中引入

```tsx
@import './Button.css';
```

### 命名规范

```tsx
const userName = ref('');      // 变量-小驼峰
function fetchData() {}        // 函数-小驼峰
const MAX_COUNT = 100;         // 常量-大写
interface UserProfile {}       // 接口-大驼峰
```

### 代码格式要求

```tsx
// ✅ 良好的格式
return (
  <div class="container">
    <h1 class="title">{this.title}</h1>
    <button
      class="btn"
      onClick={this.handleClick}
      disabled={this.isDisabled}
    >
      {this.buttonText}
    </button>
  </div>
);

// ❌ 避免内联函数
<button onClick={() => this.handleClick()}>❌</button>
<button onClick={this.handleClick}>✅</button>
```

### 导入顺序

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

```tsx
// 禁止使用相对路径访问上级目录
import {Chat} from "../../Chat"
```

### 组件拆分原则

```tsx
// ✅ 小型单一组件
// Button.tsx - 只负责按钮
// Input.tsx - 只负责输入
// Modal.tsx - 只负责弹窗

// ✅ 组合使用
render() {
  return (
    <div>
      <BaseInput value={this.value} />
      <BaseButton onClick={this.submit} />
    </div>
  );
}
```

### 错误处理

```tsx
setup() {
  const fetchData = async () => {
    try {
      // 业务逻辑
    } catch (error) {
      console.error('Error:', error);
      emit('error', error);
    }
  };
  
  return { fetchData };
}
```

## Vue 模板 ↔ JSX/渲染函数转换规范

###  指令转换速查

#### 1. 属性绑定
```vue
<!-- 模板 -->
<img :src="url" :alt="text" />
<div :class="{ active: isActive }" />
<input :value="modelValue" />
```
```jsx
// JSX
<img src={url} alt={text} />
<div class={{ active: isActive }} />
<input value={modelValue} />
```

#### 2. 事件绑定
```vue
<!-- 模板 -->
<button @click="handleClick">点击</button>
<button @click.stop="handleClick">停止冒泡</button>
<form @submit.prevent="handleSubmit">提交</form>
```
```jsx
// JSX
<button onClick={handleClick}>点击</button>
<button onClick={withModifiers(handleClick, ['stop'])}>停止冒泡</button>
<form onSubmit={withModifiers(handleSubmit, ['prevent'])}>提交</form>
```

#### 3. 条件渲染
```vue
<!-- 模板 -->
<div v-if="isVisible">内容</div>
<div v-else-if="isLoading">加载中</div>
<div v-else>其他</div>
```
```jsx
// JSX
{isVisible ? (
  <div>内容</div>
) : isLoading ? (
  <div>加载中</div>
) : (
  <div>其他</div>
)}
```
或使用逻辑与：
```jsx
{isVisible && <div>内容</div>}
```

#### 4. 列表渲染
```vue
<!-- 模板 -->
<ul>
  <li v-for="item in items" :key="item.id">
    {{ item.name }}
  </li>
</ul>
```
```jsx
// JSX
<ul>
  {items.map(item => (
    <li key={item.id}>{item.name}</li>
  ))}
</ul>
```

#### 5. 双向绑定
```vue
<!-- 模板 -->
<input v-model="text" />
<textarea v-model="content"></textarea>
<input v-model.number="count" type="number" />
```
```jsx
// JSX
<input 
  value={text} 
  onInput={e => text = e.target.value} 
/>
<textarea
  value={content}
  onInput={e => content = e.target.value}
/>
<input
  value={count}
  onInput={e => count = Number(e.target.value)}
  type="number"
/>
```

#### 6. 插槽
```vue
<!-- 模板 -->
<MyComponent>
  <template #header>
    <h1>标题</h1>
  </template>
  默认内容
  <template v-slot:footer>
    <p>页脚</p>
  </template>
</MyComponent>
```
```jsx
// JSX
<MyComponent>
  {{ 
    default: () => '默认内容',
    header: () => <h1>标题</h1>,
    footer: () => <p>页脚</p>
  }}
</MyComponent>
```

#### 7. 动态组件/属性
```vue
<!-- 模板 -->
<component :is="currentComponent" />
<div :[dynamicAttr]="value"></div>
<div v-bind="attrsObject"></div>
```
```jsx
// JSX
<component is={currentComponent} />
<div {...{ [dynamicAttr]: value }} />
<div {...attrsObject} />
```

---

### 工具函数

```jsx
import { withModifiers } from 'vue'

// 1. 事件修饰符
<button onClick={withModifiers(handleClick, ['stop', 'prevent'])}>

// 2. 创建 v-model 处理器
const vModelText = (refValue) => ({
  value: refValue.value,
  onInput: (e) => { refValue.value = e.target.value }
})

// 使用
<input {...vModelText(text)} />
```

---

### h() 函数对照表

```jsx
// JSX
<div class="container">
  <MyComponent prop={value} onEvent={handler}>
    {slotContent}
  </MyComponent>
</div>

// h() 等效写法
h('div', { class: 'container' }, [
  h(MyComponent, { 
    prop: value,
    onEvent: handler 
  }, slotContent)
])
```

---

### 常见坑点

#### 1. 布尔属性
```jsx
// ✅ 正确
<button disabled={isDisabled}>按钮</button>

// ❌ 错误（会渲染为 disabled=""）
<button disabled={isDisabled ? '' : false}>按钮</button>
```

#### 2. 事件处理器
```jsx
// ✅ 传递函数引用
<button onClick={handleClick}>

// ❌ 立即执行（错误！）
<button onClick={handleClick()}>

// ✅ 需要传参时
<button onClick={() => handleClick(id)}>

// ✅ 事件对象
<button onClick={(e) => handleClick(e, id)}>
```

#### 3. 样式绑定
```jsx
// 对象语法
<div style={{ color: active ? 'red' : 'black', fontSize: '14px' }}>

// 数组语法（多个样式对象）
<div style={[baseStyle, activeStyle]}>

// CSS 变量
<div style={{ '--color': themeColor }}></div>
```

### 记忆口诀
- **冒号变花括号** `:` → `{}`
- **@变on加驼峰** `@click` → `onClick`
- **v-if变三目** `v-if` → `? :`
- **v-for变map** `v-for` → `.map()`
- **v-model自己写** 双向绑定需手动实现