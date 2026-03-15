<template>
    <div class="outline-paragraph-container" :class="{'focused': isInputFocused}">
        <!-- 拖拽时的占位元素 -->
        <div 
            v-if="isDragging" 
            class="outline-paragraph-placeholder"
            :style="{ height: originalHeight + 'px', width: originalWidth + 'px' }"
        ></div>
        
        <div 
            class="outline-paragraph" 
            :class="{ 'dragging': isDragging }"
            ref="outlineParagraphRef"
            :style="dragStyle"
        >
            <div class="outline-paragraph-title">
                <div class="outline-paragraph-title-header">
                    <div class="drag-handle" @mousedown="startDrag" ref="dragHandleRef">
                        <SvgIcon icon="drag-handle"/>
                    </div>
                    <div class="outline-paragraph-title-text">
                        <OutlineInputArea 
                            v-model:value="title" 
                            :placeholder="placeholder" 
                            :textColor="'var(--uosai-color-outline-paragraph-text)'"
                            :fontSize="'1.15rem'"
                            :fontWeight="'500'"
                            @edit-complete="handleParagraphTitleEditComplete"
                            @isInputFocus="handleIsInputFocus"
                        />
                    </div>
                    <div class="outline-paragraph-title-icon">
                        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                            :show-after="1000" :offset="2" :content="store.loadTranslations['Add Section']"  v-show="isShowParsingStatus">
                            <div class="outline-paragraph-title-add-icon" @click="addOutlineItem">
                                <SvgIcon icon="add-outline" />
                            </div>
                        </el-tooltip>
                        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                            :show-after="1000" :offset="2" :content="store.loadTranslations['Delete Chapter']"  v-show="isShowParsingStatus">
                            <div class="outline-paragraph-title-delete-icon" @click="deleteParagraph">
                                <SvgIcon icon="trash" />
                            </div>
                        </el-tooltip>
                    </div>
                </div>
                <div class="outline-paragraph-title-description" v-show="description !== ''">
                    <OutlineInputArea 
                            v-model:value="description" 
                            :placeholder="'请输入'" 
                            @edit-complete="handleParagraphDescriptionEditComplete"
                        />
                </div>
            </div>
            <div class="outline-paragraph-content">
                <div 
                    v-for="(item, index) in localContent" 
                    :key="item.id || index"
                    class="outline-item-wrapper"
                    :class="{ 
                        'drag-over': dragOverIndex === index,
                        'drag-over-before': dragOverIndex === index && dragDirection === 'up',
                        'drag-over-after': dragOverIndex === index && dragDirection === 'down',
                        'dragging': dragState.isDragging && dragState.draggedIndex === index
                    }"
                >
                    <OutlineItem 
                        :title="item.title" 
                        :index="index"
                        :isNewlyAdded="item.isNewlyAdded || false"
                        @drag-start="handleDragStart"
                        @drag-move="handleDragMove"
                        @drag-end="handleDragEnd"
                        @update:title="(newTitle) => updateItemTitle(index, newTitle)"
                        @update:content="(newContent) => updateItemContent(index, newContent)"
                        @add-item="addOutlineItem"
                        @delete-item="(itemIndex) => handleDeleteItem(itemIndex)"
                    />
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed, defineProps, defineEmits } from 'vue'
import SvgIcon from "@/components/svgIcon/svgIcon.vue"
import OutlineItem from './OutlineItem.vue'
import OutlineInputArea from "./OutlineInputArea.vue"

import { Qrequest } from "@/utils";
import { useGlobalStore } from "@/store/global";
const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()

const props = defineProps({
    title: {
        type: String,
        default: ''
    },
    description: {
        type: String,
        default: ''
    },
    content: {
        type: Array,
        default: () => []
    },
    index: {
        type: Number,
        default: 0
    }
})

const title = computed(() =>{
    return props.title
})

const description = computed(() =>{
    return props.description || ''
})

// 段落标题占位符
const placeholder = computed(() => {
    return store.loadTranslations['Enter Chapter Title'] || '请输入段落标题'
})

// 处理标题编辑完成
const handleParagraphTitleEditComplete = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        emit('update:title', value)
    })
}

// 处理描述编辑完成
const handleParagraphDescriptionEditComplete = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        emit('update:description', value)
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

const addOutlineItem = async () => {
    // 在localContent最后一个元素后新增一个元素
    const newItem = {
        title: '',
        id: `item-${Date.now()}-${localContent.value.length}`,
        isNewlyAdded: true  // 标记这是新增的项目
    }
    
    localContent.value.push(newItem)
    
    // 通知父组件内容已更新
    emit('update:content', localContent.value.map(item => {
        const { id, isNewlyAdded, ...rest } = item
        return rest
    }))
    
    // 等待DOM更新完成
    await nextTick()
    
    // 聚焦到新添加的项目的标题输入框
    focusNewItemTitleInput(localContent.value.length - 1)
}

const emit = defineEmits(['update:content', 'update:title', 'update:description', 'drag-start', 'drag-move', 'drag-end', 'delete-item'])

const outlineParagraphRef = ref(null)
const dragHandleRef = ref(null)

const isDragging = ref(false)
const dragOffsetY = ref(0) // 记录鼠标相对元素的Y偏移
const originalHeight = ref(0) // 记录原始高度
const originalWidth = ref(0) // 记录原始宽度

// 拖拽样式 - 使用fixed定位和transform实现流畅的1:1鼠标跟随
const dragStyle = computed(() => {
    if (!isDragging.value) return {}
    
    // 获取原始元素的位置信息
    const rect = outlineParagraphRef.value?.getBoundingClientRect()
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

const localContent = ref([...props.content.map((item, index) => ({
    ...item,
    id: item.id || `item-${Date.now()}-${index}`
}))])

const dragState = ref({
    isDragging: false,
    draggedIndex: -1,
    placeholderIndex: -1
})

const dragOverIndex = ref(-1)
const dragDirection = ref('') // 添加拖拽方向状态

// 自动滚动配置
const autoScrollConfig = {
    edgeThreshold: 40, // 边缘阈值（像素）
    scrollSpeed: 200, // 滚动速度（像素/秒）
    scrollInterval: 16 // 滚动间隔（毫秒）
}

let autoScrollInterval = null // 自动滚动定时器
let autoScrollDirection = 0 // 滚动方向（-1向上，1向下，0停止）

// 开始自动滚动
const startAutoScroll = (direction) => {
    if (autoScrollInterval) return
    
    autoScrollDirection = direction
    autoScrollInterval = setInterval(() => {
        const chatHistory = document.getElementById('chatHistory')
        if (chatHistory) {
            const scrollAmount = (autoScrollConfig.scrollSpeed * autoScrollConfig.scrollInterval) / 1000
            chatHistory.scrollTop += scrollAmount * autoScrollDirection
        }
    }, autoScrollConfig.scrollInterval)
}

// 停止自动滚动
const stopAutoScroll = () => {
    if (autoScrollInterval) {
        clearInterval(autoScrollInterval)
        autoScrollInterval = null
        autoScrollDirection = 0
    }
}

// 检查是否需要自动滚动
const checkAutoScroll = (currentY) => {
    const chatHistory = document.getElementById('chatHistory')
    if (!chatHistory) return
    
    const rect = chatHistory.getBoundingClientRect()
    const topEdge = rect.top + autoScrollConfig.edgeThreshold
    const bottomEdge = rect.bottom - autoScrollConfig.edgeThreshold
    
    if (currentY <= topEdge) {
        // 鼠标在顶部边缘，向上滚动
        startAutoScroll(-1)
    } else if (currentY >= bottomEdge) {
        // 鼠标在底部边缘，向下滚动
        startAutoScroll(1)
    } else {
        // 鼠标在中间区域，停止滚动
        stopAutoScroll()
    }
}

// 更新自动滚动配置（调试用）
const updateAutoScrollConfig = (config) => {
    if (config.edgeThreshold !== undefined) {
        autoScrollConfig.edgeThreshold = config.edgeThreshold
    }
    if (config.scrollSpeed !== undefined) {
        autoScrollConfig.scrollSpeed = config.scrollSpeed
    }
    if (config.scrollInterval !== undefined) {
        autoScrollConfig.scrollInterval = config.scrollInterval
    }
}

// 在组件挂载后设置全局调试接口
onMounted(() => {
    // 将组件实例暴露到全局，方便调试
    window.OutlineParagraphDebug = {
        updateAutoScrollConfig: updateAutoScrollConfig,
        getCurrentConfig: () => ({
            edgeThreshold: autoScrollConfig.edgeThreshold,
            scrollSpeed: autoScrollConfig.scrollSpeed,
            scrollInterval: autoScrollConfig.scrollInterval
        })
    }
})

// 在组件卸载时清理
onUnmounted(() => {
    stopAutoScroll()
    if (window.OutlineParagraphDebug) {
        delete window.OutlineParagraphDebug
    }
})

watch(() => props.content, (newContent) => {
    // 只在内容长度或结构发生变化时才重新创建数组
    if (newContent.length !== localContent.value.length || 
        JSON.stringify(newContent.map(item => ({ title: item.title, content: item.content }))) !== 
        JSON.stringify(localContent.value.map(item => ({ title: item.title, content: item.content })))) {
        localContent.value = [...newContent.map((item, index) => ({
            ...item,
            id: item.id || `item-${Date.now()}-${index}`,
            isNewlyAdded: false  // 从父组件传入的内容不认为是新增的
        }))]
    }
}, { deep: true })

const handleDragStart = (dragData) => {
    dragState.value = {
        isDragging: true,
        draggedIndex: dragData.index,
        placeholderIndex: -1,
        startY: dragData.startY,
        elementTop: dragData.elementTop,
        draggedHeight: dragData.originalHeight || dragData.element.offsetHeight, // 记录拖拽元素的高度
        draggedWidth: dragData.originalWidth || dragData.element.offsetWidth, // 记录拖拽元素的宽度
        mouseOffsetY: dragData.mouseOffsetY || 0 // 添加鼠标偏移量
    }
}

const handleDragMove = (dragData) => {
    if (!dragState.value.isDragging) return
    
    const currentY = dragData.currentY || dragData.event.clientY
    
    // 检查是否需要自动滚动
    checkAutoScroll(currentY)
    
    // 只获取当前段落内的outline-item-wrapper，避免跨段落干扰
    const currentParagraphContainer = outlineParagraphRef.value
    if (!currentParagraphContainer) return
    
    const itemElements = currentParagraphContainer.querySelectorAll('.outline-item-wrapper')
    const draggedHeight = dragState.value.draggedHeight
    
    let newIndex = dragState.value.draggedIndex
    
    // 获取当前拖拽元素的实际位置（考虑鼠标偏移量）
    const mouseOffsetY = dragState.value.mouseOffsetY || 0
    const draggedTop = currentY - mouseOffsetY
    const draggedBottom = draggedTop + draggedHeight
    
    // 检查是否回到原位置附近
    const currentDraggedElement = itemElements[dragState.value.draggedIndex]
    if (currentDraggedElement) {
        const currentRect = currentDraggedElement.getBoundingClientRect()
        const currentCenter = currentRect.top + currentRect.height / 2
        const draggedCenter = draggedTop + draggedHeight / 2
        
        // 如果拖拽元素的中心接近原位置的中心，则认为是回到原位置
        if (Math.abs(draggedCenter - currentCenter) < 25) {
            newIndex = dragState.value.draggedIndex
            dragDirection.value = ''
            if (newIndex !== dragState.value.placeholderIndex) {
                dragState.value.placeholderIndex = newIndex
                dragOverIndex.value = -1 // 清除拖拽指示
            }
            return
        }
    }
    
    for (let i = 0; i < itemElements.length; i++) {
        if (i === dragState.value.draggedIndex) continue
        
        const rect = itemElements[i].getBoundingClientRect()
        const itemTop = rect.top
        const itemBottom = rect.top + rect.height
        
        // 改进的边界检测：当拖拽元素的上边界或下边界越过相邻元素时触发交换
        if (i < dragState.value.draggedIndex) {
            // 向上移动：当拖拽元素的上边界越过目标元素的上边界时
            if (draggedTop < itemTop + 15) { // 添加15px的缓冲区域，提高灵敏度
                newIndex = i
                dragDirection.value = 'up'
                break
            }
        }
        else if (i > dragState.value.draggedIndex) {
            // 向下移动：当拖拽元素的下边界越过目标元素的下边界时
            if (draggedBottom > itemBottom - 15) { // 添加15px的缓冲区域，提高灵敏度
                newIndex = i
                dragDirection.value = 'down'
            }
        }
    }
    
    if (newIndex !== dragState.value.draggedIndex && newIndex !== dragState.value.placeholderIndex) {
        dragState.value.placeholderIndex = newIndex
        dragOverIndex.value = newIndex
    }
}

const handleDragEnd = (dragData) => {
    // 停止自动滚动
    stopAutoScroll()
    
    // 在拖拽结束时执行重排
    if (dragState.value.placeholderIndex !== -1 && dragState.value.placeholderIndex !== dragState.value.draggedIndex) {
        reorderItems(dragState.value.draggedIndex, dragState.value.placeholderIndex)
    }
    
    dragState.value = {
        isDragging: false,
        draggedIndex: -1,
        placeholderIndex: -1
    }
    dragOverIndex.value = -1
    dragDirection.value = '' // 重置拖拽方向
    
    // 即使回到原位置，也需要更新拖拽状态，但不需要重新排序
    // 通知父组件拖拽结束（即使位置没变也通知，用于清理状态）
    emit('update:content', localContent.value.map(item => {
        const { id, ...rest } = item
        return rest
    }))
}

const reorderItems = (fromIndex, toIndex) => {
    if (fromIndex === toIndex) return
    
    const items = [...localContent.value]
    const draggedItem = items[fromIndex]
    
    // 移除原位置的元素
    items.splice(fromIndex, 1)
    // 在新位置插入元素
    items.splice(toIndex, 0, draggedItem)
    
    localContent.value = items
}

// 更新项标题
const updateItemTitle = (index, newTitle) => {
    localContent.value[index].title = newTitle
    // 如果用户输入了内容，清除新增标记
    if (newTitle && newTitle.trim() !== '') {
        localContent.value[index].isNewlyAdded = false
    }
    // 通知父组件内容已更新
    emit('update:content', localContent.value.map(item => {
        const { id, isNewlyAdded, ...rest } = item
        return rest
    }))
}

// 更新项内容
const updateItemContent = (index, newContent) => {
    localContent.value[index].content = newContent
    // 通知父组件内容已更新
    emit('update:content', localContent.value.map(item => {
        const { id, ...rest } = item
        return rest
    }))
}

// 在指定索引位置添加新项目
const handleAddItemAtIndex = async (itemIndex) => {
    // 在当前点击的outline-item下方新增一个outline-item
    const newItem = {
        title: '',
        id: `item-${Date.now()}-${itemIndex + 1}`
    }
    
    // 在指定位置插入新元素
    localContent.value.splice(itemIndex + 1, 0, newItem)
    
    // 通知父组件内容已更新
    emit('update:content', localContent.value.map(item => {
        const { id, ...rest } = item
        return rest
    }))
    
    // 等待DOM更新完成
    await nextTick()
    
    // 聚焦到新添加的项目的标题输入框
    focusNewItemTitleInput(itemIndex + 1)
}

// 处理删除outline-item
const handleDeleteItem = (itemIndex) => {
    // 通知父组件删除指定索引的item
    emit('delete-item', itemIndex)
}

// 处理删除整个段落
const deleteParagraph = async () => {
    const ret = await Qrequest(chatQWeb.isDeleteOutlineTitle)
    // 通知父组件删除当前段落
    if (ret) {
        emit('delete-paragraph', props.index)
    }
}

const startDrag = (event) => {
    isDragging.value = true
    
    // 记录原始高度和宽度
    const rect = outlineParagraphRef.value.getBoundingClientRect()
    originalHeight.value = rect.height
    originalWidth.value = rect.width
    
    // 记录鼠标按下时的初始位置
    const startY = event.clientY
    const elementTop = rect.top
    
    // 记录鼠标相对于元素顶部的偏移量，用于精确跟随
    const mouseOffsetY = event.clientY - rect.top
    
    emit('drag-start', {
        index: props.index || 0, // 使用props.index，如果没有则默认为0
        element: outlineParagraphRef.value,
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
    const rect = outlineParagraphRef.value.getBoundingClientRect()
    const handleRect = dragHandleRef.value.getBoundingClientRect()
    const currentY = event.clientY
    
    // 使用鼠标偏移量来计算精确的跟随位置
    dragOffsetY.value = currentY - handleRect.top - (handleRect.height / 2)
    
    emit('drag-move', {
        index: props.index || 0,
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
        index: props.index || 0,
        event
    })
    
    document.removeEventListener('mousemove', handleDrag)
    document.removeEventListener('mouseup', endDrag)
}

// 聚焦到新添加项目的标题输入框
const focusNewItemTitleInput = async (itemIndex) => {
    // 等待DOM完全更新
    await nextTick()
    
    // 只在当前段落的容器内查找outline-item-wrapper
    const currentParagraphContainer = outlineParagraphRef.value
    if (!currentParagraphContainer) return
    
    // 获取当前段落内的所有outline-item-wrapper
    const outlineItemWrappers = currentParagraphContainer.querySelectorAll('.outline-item-wrapper')
    if (itemIndex >= 0 && itemIndex < outlineItemWrappers.length) {
        const targetWrapper = outlineItemWrappers[itemIndex]
        const titleInput = targetWrapper.querySelector('.input-field')
        if (titleInput) {
            titleInput.focus()
            // 添加输入事件监听器来检查空标题
            const checkEmptyTitle = () => {
                if (titleInput.value.trim() === '') {
                    // 如果标题为空，删除该项
                    handleDeleteItem(itemIndex)
                }
            }
            
            // 在失去焦点时检查
            titleInput.addEventListener('blur', checkEmptyTitle, { once: true })
            
            // 也监听输入事件，如果用户开始输入则移除blur监听器
            const removeBlurListener = () => {
                if (titleInput.value.trim() !== '') {
                    titleInput.removeEventListener('blur', checkEmptyTitle)
                    titleInput.removeEventListener('input', removeBlurListener)
                }
            }
            titleInput.addEventListener('input', removeBlurListener)
        }
    }
}

onUnmounted(() => {
    document.removeEventListener('mousemove', handleDrag)
    document.removeEventListener('mouseup', endDrag)
})

</script>

<style lang="scss" scoped>
.outline-paragraph-container {
    position: relative;
    width: 100%; // 确保容器占满父元素宽度
    box-sizing: border-box; // 确保宽度计算包含padding和border
    border: 1px solid var(--uosai-color-outline-paragraph-border);
    background-color: var(--uosai-color-outline-paragraph-bg);
    border-radius: 8px;
    margin-bottom: 6px;

    &.focused {
        border-color: var(--activityColor);
    }
}

.outline-paragraph-placeholder {
    background-color: rgba(33, 150, 243, 0);
    border: 2px dashed var(--activityColor);
    border-radius: 8px;
    margin: 2px 0;
    width: 100%; // 确保占位元素宽度与容器一致
    box-sizing: border-box; // 确保宽度计算包含padding和border
}

.outline-paragraph {
    padding: 10px;
    cursor: pointer;
    width: 100%; // 确保元素占满容器宽度
    box-sizing: border-box; // 确保宽度计算包含padding和border
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);

    // &:hover {
    //     background-color: #f0f0f0;
    // }

    &.dragging {
        opacity: 0.9;
        background-color: var(--outlineDraggingItemBg);
        border: 2px solid var(--activityColor);
        border-radius: 8px;
        box-shadow: 0 4px 12px var(--outlineDraggingItemShadow);
        transition: none; // 拖拽时禁用过渡，确保流畅
        // 移除了transform: scale(1.02)，确保元素大小完全不变
    }

    .outline-paragraph-title {
        font-size: 1rem;
        display: block;
        align-items: center;

        .outline-paragraph-title-header {
                font-size: 1rem;
                color: #000;
                display: flex;
                align-items: center;
                justify-content: space-between;

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
            }

            .outline-paragraph-title-text {
                margin-right: 10px;
                flex: 1;
            }

            .outline-paragraph-title-icon {
                display: flex;
                align-items: center;

                .outline-paragraph-title-add-icon, .outline-paragraph-title-delete-icon {
                    margin-right: 10px;
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

        .outline-paragraph-title-description {
            font-size: 0.8rem;
            margin-left: 30px;
            color: #666;
        }
    }

    .outline-paragraph-content {
        margin: 0 0 0 10px;

        .outline-item-wrapper {
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            position: relative;
            width: 100%;
            box-sizing: border-box;
            margin-bottom: 1px; // 添加默认间距，除了最后一个元素
            
            &:last-child {
                margin-bottom: 0; // 最后一个元素没有下边距
            }
            
            &.drag-over {
                position: relative;
                
                &.drag-over-before {
                    margin-top: 80px; // 为拖拽项目预留顶部空间，与一级标题保持一致
                }
                
                &.drag-over-after {
                    margin-bottom: 80px; // 为拖拽项目预留底部空间，与一级标题保持一致
                }
                
                opacity: 0.7;
                
                // 添加视觉指示器，与一级标题保持一致
                &::before {
                    content: '';
                    position: absolute;
                    left: 0;
                    right: 0;
                    height: 3px;
                    background-color: var(--activityColor);
                    z-index: 100;
                }
                
                &.drag-over-before::before {
                    top: -40px; // 与一级标题保持一致
                }
                
                &.drag-over-after::before {
                    bottom: -40px; // 与一级标题保持一致
                }
            }
            
            &.dragging {
                opacity: 1;
            }
        }
    }
}
</style>