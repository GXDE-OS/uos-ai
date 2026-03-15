<template>
    <div class="chat-input-area" :class="{ 'expanded': isExpanded, 'blur': !isFocused && isExpanded }">
        <!-- 输入区域 -->
        <div class="input-container" @click="focusInput">
            <div class="input-wrapper">                
                <!-- 文本输入框 -->
                <textarea
                    ref="inputRef"
                    v-model="inputValue"
                    class="input-field"
                    :rows="minRows"
                    :max-rows="maxRows"
                    :maxlength="maxLength"
                    @focus="onFocus"
                    @blur="onBlur"
                    @input="onInput"
                    @keydown="handleKeyDown"
                    :placeholder="placeholder"
                    :style="{ color: props.textColor, fontSize: props.fontSize, fontWeight: props.fontWeight }"
                ></textarea>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, nextTick, watch, onMounted, onUnmounted } from 'vue'
import { useGlobalStore } from "@/store/global"

const store = useGlobalStore()

// Props
const props = defineProps({
    // 初始值
    value: {
        type: String,
        default: ''
    },
    // 占位符文本
    placeholder: {
        type: String,
        default: 'Type your message here...'
    },
    // 最小行数
    minRows: {
        type: Number,
        default: 1
    },
    // 最大行数
    maxRows: {
        type: Number,
        default: 10  // 固定最大显示10行
    },
    // 是否禁用
    disabled: {
        type: Boolean,
        default: false
    },
    // 字体颜色
    textColor: {
        type: String,
        default: 'var(--uosai-text-color)'
    },
    // 字体大小
    fontSize: {
        type: String,
        default: '1rem'
    },
    // 字重
    fontWeight: {
        type: String,
        default: '500'
    },
    // 最大输入长度
    maxLength: {
        type: Number,
        default: 500
    },
})

// Emits
const emit = defineEmits(['update:value', 'send', 'blur', 'edit-complete', 'isInputFocus'])

// Refs
const inputRef = ref(null)
const inputValue = ref(props.value)
const isFocused = ref(false)
const isExpanded = ref(false)
const editTimer = ref(null) // 编辑定时器

// Methods
const focusInput = () => {
    if (inputRef.value) {
        inputRef.value.focus()
    }
}

const onFocus = () => {
    isFocused.value = true
    isExpanded.value = true
    emit('isInputFocus', true)
}

const onBlur = () => {
    isFocused.value = false
    emit('blur')
    emit('isInputFocus', false)
    
    // 清除输入定时器
    if (editTimer.value) {
        clearTimeout(editTimer.value)
        editTimer.value = null
    }

    if (!inputValue.value.trim()) {
        isExpanded.value = false
    }

    // emit('edit-complete', inputValue.value)
    
    // 延迟收起，避免按钮点击失效
    // setTimeout(() => {
        
    //     // 触发编辑完成事件
    //     emit('edit-complete', inputValue.value)
    // }, 200)
}

const onInput = () => {
    // 检查输入长度限制
    if (props.maxLength && inputValue.value.length > props.maxLength) {
        inputValue.value = inputValue.value.slice(0, props.maxLength)
    }
    
    emit('update:value', inputValue.value)
    adjustTextareaHeight()
    
    // 清除之前的定时器
    if (editTimer.value) {
        clearTimeout(editTimer.value)
    }
    
    // 实时触发编辑完成事件，但使用防抖机制避免过于频繁
    editTimer.value = setTimeout(() => {
        // 保存当前焦点状态
        const hadFocus = isFocused.value
        const currentSelectionStart = inputRef.value?.selectionStart
        const currentSelectionEnd = inputRef.value?.selectionEnd
        
        emit('edit-complete', inputValue.value)
        
        // 在下一个tick中恢复焦点和光标位置
        if (hadFocus) {
            nextTick(() => {
                if (inputRef.value) {
                    inputRef.value.focus()
                    if (currentSelectionStart !== undefined && currentSelectionEnd !== undefined) {
                        inputRef.value.setSelectionRange(currentSelectionStart, currentSelectionEnd)
                    }
                }
            })
        }
    }, 100) // 100ms防抖，平衡实时性和性能
}


const adjustTextareaHeight = () => {
    nextTick(() => {
        if (!inputRef.value) return
        
        const textarea = inputRef.value
        textarea.style.height = 'auto'
        
        const computedStyle = window.getComputedStyle(textarea)
        const lineHeight = parseInt(computedStyle.lineHeight) || 20
        const paddingTop = parseInt(computedStyle.paddingTop) || 0
        const paddingBottom = parseInt(computedStyle.paddingBottom) || 0
        const borderTop = parseInt(computedStyle.borderTopWidth) || 0
        const borderBottom = parseInt(computedStyle.borderBottomWidth) || 0
        
        const totalPadding = paddingTop + paddingBottom + borderTop + borderBottom
        const minHeight = lineHeight * props.minRows + totalPadding
        const maxHeight = lineHeight * props.maxRows + totalPadding
        
        // 计算内容高度
        const scrollHeight = textarea.scrollHeight
        
        // 自适应高度，不超过最大高度
        if (scrollHeight <= maxHeight) {
            // 内容高度小于等于最大高度时，使用实际内容高度
            textarea.style.height = Math.max(scrollHeight, minHeight) + 'px'
            textarea.style.overflowY = 'hidden'
        } else {
            // 内容高度超过最大高度时，使用最大高度并显示滚动条
            textarea.style.height = maxHeight + 'px'
            textarea.style.overflowY = 'auto'
            
            // 自动滚动到底部
            textarea.scrollTop = textarea.scrollHeight
        }
    })
}

const handleKeyDown = (event) => {
    // 处理回车键
    if (event.key === 'Enter') {
        // Enter 键：允许浏览器默认行为（换行），但阻止事件冒泡到全局监听器
        event.stopPropagation() // 阻止事件冒泡到全局监听器
    }
}


// Watch for value changes from parent
watch(() => props.value, (newValue) => {
    if (newValue !== inputValue.value) {
        inputValue.value = newValue
        nextTick(adjustTextareaHeight)
    }
})

// Lifecycle
onMounted(() => {
    adjustTextareaHeight()
})

onUnmounted(() => {
    // 清理定时器
    if (editTimer.value) {
        clearTimeout(editTimer.value)
    }
})
</script>

<style lang="scss" scoped>
.chat-input-area {
    width: 100%;
    transition: all 0.3s ease;
    
    &.expanded {        
        // 当失焦时隐藏box-shadow
        // &.blur {
        //     .input-container {
        //         box-shadow: none;
        //     }
        // }
    }
}

.input-container {
    display: flex;
    align-items: flex-end;
    border-radius: 8px;
    background-color: var(--uosai-input-bg);
    height: 100%;
    transition: all 0.2s ease;
    cursor: text;
    
    &:hover {
        border-color: var(--uosai-border-color-hover);
    }
}

.input-wrapper {
    flex: 1;
    position: relative;
    min-height: 40px;
    display: flex;
    align-items: center;
        
    .input-field {
        width: 100%;
        border: none;
        outline: none;
        background: transparent;
        resize: none;
        line-height: 1.2;
        padding: 0;
        margin-top: 10px;
        margin-bottom: 10px;
        
        font-family: var(--font-family);
        
        // 自定义滚动条样式
        &::-webkit-scrollbar {
            width: 6px;
            height: 6px;
            
            &:hover {
                width: 8px;
                height: 8px;
            }
        }
        
        &::-webkit-scrollbar-thumb {
            border-radius: 4px;
            background: var(--uosai-color-scroll-bg);
            border: 1px solid var(--uosai-color-border);
        }
        
        &::-webkit-scrollbar-track {
            background: transparent;
        }
        
        &::placeholder {
            color: var(--uosai-color-inputcontent-placeholder);
        }
    }
}

// 暗色主题适配
// .dark {
//     .chat-input-area {
//         .input-container {
//             border-color: var(--uosai-border-color-dark);
//             background-color: var(--uosai-input-bg-dark);
            
//             &:hover {
//                 border-color: var(--uosai-border-color-hover-dark);
//             }
//         }
        
//         .input-wrapper .placeholder {
//             color: var(--uosai-placeholder-color-dark);
//         }
        
//         .input-field {
//             color: var(--uosai-text-color-dark);
            
//             &::placeholder {
//                 color: var(--uosai-color-inputcontent-placeholder);
//             }
//         }
//     }
// }
</style>