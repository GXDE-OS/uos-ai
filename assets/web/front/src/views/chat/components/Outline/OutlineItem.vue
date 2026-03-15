<template>
    <div class="outline-item-container">
        <!-- 拖拽时的占位元素 -->
        <div 
            v-if="isDragging" 
            class="outline-item-placeholder"
            :style="{ height: originalHeight + 'px', width: originalWidth + 'px' }"
        ></div>
        
        <div 
            class="outline-item" 
            :class="{ 'dragging': isDragging , 'focused': isInputFocused}"
            ref="outlineItemRef"
            :style="dragStyle"
        >
            <div class="outline-item-title">
                <div class="drag-handle" @mousedown="startDrag" ref="dragHandleRef" :class="{ 'disabled': isDragDisabled }">
                    <SvgIcon icon="drag-handle"/>
                </div>
                <div class="outline-item-title-text">
                    <OutlineInputArea 
                        v-model:value="title" 
                        :placeholder="placeholder"  
                        :textColor="'var(--uosai-color-outline-item-text)'"
                        :fontSize="'1rem'"
                        :fontWeight="'500'"
                        @edit-complete="handleTitleEditComplete"
                        @isInputFocus="handleIsInputFocus"
                    />
                </div>
                <div style="width: 40px;display: flex;justify-content: center;align-items: center;">
                    <div class="outline-item-title-icon">
                        <div class="outline-item-title-add-icon" @click="handleAddItem" v-show="false">
                            <SvgIcon icon="add-outline" />
                        </div>
                        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                            :show-after="1000" :offset="2" :content="store.loadTranslations['Delete Chapter']"  v-show="isShowParsingStatus">
                            <div class="outline-item-title-delete-icon" @click="handleDeleteItem">
                                <SvgIcon icon="trash" />
                            </div>
                        </el-tooltip>
                    </div>
                </div>
            </div>
            <div class="outline-item-content" v-show="content !== ''">
                <OutlineInputArea 
                    v-model:value="content" 
                    :placeholder="'请输入'" 
                    @edit-complete="handleContentEditComplete"
                />
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed, defineProps, defineEmits } from 'vue'
import SvgIcon from "@/components/svgIcon/svgIcon.vue"
import OutlineInputArea from "./OutlineInputArea.vue"

import { Qrequest } from "@/utils";
import { useGlobalStore } from "@/store/global";
const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()

const props = defineProps({
    title: {
        type: String,
        default: 'this is title'
    },
    content: {
        type: String,
    },
    index: {
        type: Number,
        default: 0
    },
    isNewlyAdded: {
        type: Boolean,
        default: false
    }
})

const title = computed(() =>{
    return props.title
})

const content = computed(() =>{
    return props.content || ''
})

const placeholder = computed(() => {
    return store.loadTranslations['Enter Chapter Title'] || '请输入子标题'
})

// 计算属性：检查是否允许拖拽（只有新增的且标题为空的项才禁用拖拽）
const isDragDisabled = computed(() => {
    return props.isNewlyAdded && (!props.title || props.title.trim() === '')
})

const emit = defineEmits(['drag-start', 'drag-move', 'drag-end', 'update:title', 'update:content', 'add-item', 'delete-item'])
const outlineItemRef = ref(null)
const dragHandleRef = ref(null)

const isDragging = ref(false)
const dragOffsetY = ref(0) // 记录鼠标相对元素的Y偏移
const originalHeight = ref(0) // 记录原始高度
const originalWidth = ref(0) // 记录原始宽度

// 拖拽样式 - 使用fixed定位和transform实现流畅的1:1鼠标跟随
const dragStyle = computed(() => {
    if (!isDragging.value) return {}
    
    // 获取原始元素的位置信息
    const rect = outlineItemRef.value?.getBoundingClientRect()
    if (!rect) return {}
    
    return {
        position: 'fixed',
        top: `${rect.top}px`,
        left: `${rect.left}px`,
        width: `${originalWidth.value}px`, // 使用拖拽前的原始宽度，不进行任何调整
        zIndex: 1000,
        transform: `translateY(${dragOffsetY.value}px)`,
        pointerEvents: 'none', // 避免干扰鼠标事件
        transition: 'none' // 拖拽时禁用过渡效果，确保实时跟随
    }
})

const startDrag = (event) => {
    // 如果拖拽被禁用，则不执行拖拽操作
    if (isDragDisabled.value) {
        return
    }
    
    isDragging.value = true
    
    // 记录原始高度和宽度
    const rect = outlineItemRef.value.getBoundingClientRect()
    originalHeight.value = rect.height
    originalWidth.value = rect.width
    
    // 记录鼠标按下时的初始位置
    const startY = event.clientY
    const elementTop = rect.top
    
    // 记录鼠标相对于元素顶部的偏移量，用于精确跟随
    const mouseOffsetY = event.clientY - rect.top
    
    emit('drag-start', {
        index: props.index,
        element: outlineItemRef.value,
        event,
        startY,
        elementTop,
        mouseOffsetY, // 添加鼠标偏移量
        originalHeight: rect.height, // 添加原始高度
        originalWidth: rect.width // 使用原始宽度，不进行任何调整
    })
    
    document.addEventListener('mousemove', handleDrag)
    document.addEventListener('mouseup', endDrag)
    
    event.preventDefault()
}

const handleDrag = (event) => {
    if (!isDragging.value) return
    
    // 计算元素应该跟随鼠标移动的距离
    const rect = outlineItemRef.value.getBoundingClientRect()
    const dragHandleRect = dragHandleRef.value.getBoundingClientRect()
    const currentY = event.clientY
    
    // 使用鼠标偏移量来计算精确的跟随位置
    dragOffsetY.value = currentY - dragHandleRect.top - (dragHandleRect.height / 2)
    
    emit('drag-move', {
        index: props.index,
        event,
        offsetY: dragOffsetY.value,
        currentY // 添加当前鼠标Y坐标
    })
}

const endDrag = (event) => {
    if (!isDragging.value) return
    
    isDragging.value = false
    dragOffsetY.value = 0 // 重置偏移量，让元素回到原位
    originalHeight.value = 0 // 重置原始高度
    originalWidth.value = 0 // 重置原始宽度
    
    emit('drag-end', {
        index: props.index,
        event
    })
    
    document.removeEventListener('mousemove', handleDrag)
    document.removeEventListener('mouseup', endDrag)
}

onUnmounted(() => {
    document.removeEventListener('mousemove', handleDrag)
    document.removeEventListener('mouseup', endDrag)
})

// 处理标题编辑完成
const handleTitleEditComplete = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        emit('update:title', value)
    })
}

// 处理输入框获得焦点
const isInputFocused = ref(false)
const handleIsInputFocus = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        isInputFocused.value = value
    })
}

// 处理内容编辑完成
const handleContentEditComplete = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        emit('update:content', value)
    })
}

// 处理添加项目按钮点击
const handleAddItem = () => {
    // 通知父组件在当前项目下方添加新项目
    emit('add-item', props.index)
}

// 处理删除项目按钮点击
const handleDeleteItem = async ()  => {
    const ret = await Qrequest(chatQWeb.isDeleteOutlineTitle)
    // 通知父组件删除当前项目
    if (ret) {
        emit('delete-item', props.index)
    }
}
</script>

<style lang="scss" scoped>
.outline-item-container {
    position: relative;
    width: 100%; // 确保容器占满父元素宽度
    box-sizing: border-box; // 确保宽度计算包含padding和border
}

.outline-item-placeholder {
    background-color: rgba(33, 150, 243, 0);
    border: 2px dashed var(--activityColor);
    border-radius: 8px;
    margin: 2px 0;
    width: 100%; // 确保占位元素宽度与容器一致
    box-sizing: border-box; // 确保宽度计算包含padding和border
}

.outline-item {
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    width: 100%; // 确保元素占满容器宽度
    min-height: 40px;
    box-sizing: border-box; // 确保宽度计算包含padding和border

    &:hover {
        background-color: var(--uosai-color-outline-item-hover-bg);
        border-radius: 8px;
    }

    &.focused {
        border-radius: 8px;
        box-shadow: 0 0 0 1px var(--activityColor);
    }

    &.dragging {
        opacity: 0.9;
        background-color: var(--outlineDraggingItemBg);
        border: 2px solid var(--activityColor);
        border-radius: 8px;
        box-shadow: 0 4px 12px  var(--outlineDraggingItemShadow);
        transition: none; // 拖拽时禁用过渡，确保流畅
        // 移除了transform: scale(1.02)，确保元素大小完全不变
    }

    .outline-item-title {
        font-size: 1rem;
        display: flex;
        align-items: center;

        .drag-handle {
            cursor: grab;
            margin-right: 8px;
            padding: 4px;
            border-radius: 4px;
            transition: background-color 0.2s ease;

            svg {
                fill: var(--uosai-color-outline-draghandle);
            }
            
            // &:hover {
            //     background-color: #e0e0e0;
            // }
            
            &:active {
                cursor: grabbing;
            }
            
            // 禁用状态的样式
            &.disabled {
                cursor: not-allowed;
                opacity: 0.4;
                
                &:hover {
                    background-color: transparent;
                }
            }
        }

        .outline-item-title-text {
            flex: 1;
            max-width: calc(100% - 66px);
        }

        .outline-item-title-icon {
            display: none;
            align-items: center;

            .outline-item-title-add-icon, .outline-item-title-delete-icon {
                margin-left: 10px;
                
            }

            .outline-item-title-add-icon, .outline-item-title-delete-icon {
                margin-right: 10px;
                cursor: pointer;
                width: 20px;
                height: 20px;
                display: flex;
                align-items: center;
                justify-content: center;
                svg {
                    fill: var(--uosai-color-flat-btn-icon);
                }

                &:hover {
                    background-color: var(--uosai-color-flat-btn-bg-hover);
                    border-radius: 4px;
                }

                &:active {
                    background-color: var(--uosai-color-flat-btn-bg-hover);
                    border-radius: 4px;

                    svg {
                        fill: var(--activityColor);
                    }
                }
            }
        }

    }

    &:hover .outline-item-title-icon {
        display: flex;
    }

    .outline-item-content {
        font-size: 0.8rem;
        color: #666;
        margin-left: 30px;
    }
}
</style>