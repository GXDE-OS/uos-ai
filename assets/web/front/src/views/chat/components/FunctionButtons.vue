<template>
    <div class="function-buttons" ref="functionButtonsRef">
        <div class="buttons-container" :class="{ 'expanded': isExpanded }" ref="buttonsContainerRef">
            <div v-for="(item, index) in functionList" :key="layoutKey + '-' + index" class="button-wrapper">
                <div class="function-button" :class="{ active: activeIndex === index }" @click="handleClick(item, index)">
                    <SvgIcon :icon=item.iconName class="function-icon"/>
                    <span class="function-text">{{ item.Name }}</span>
                </div>
                <div v-if="index < functionList.length - 1 && !isLastInRow(index)" class="divider"></div>
            </div>
        </div>
        <div v-if="isShowMoreBtn" class="more-button" @click="toggleExpand">
            <SvgIcon :icon="isExpanded ? 'arrow_up' : 'arrow_down'" style="fill: var(--uosai-think-title-svg-color);"/>
        </div>
        <div class="function-separator"></div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { useGlobalStore } from "@/store/global";

const store = useGlobalStore()

const props = defineProps({
    functionList: {
        type: Array,
        required: true
    },
    isWindowMode: {
        type: Boolean,
        default: false
    },
    currentAssistant: {
        type: Object,
        required: true
    }
})

const emit = defineEmits(['selectFunction', 'clearFunction'])

const activeIndex = computed(() => {
    const found = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === props.currentAssistant.type)
    return found ? found.index : -1
})

const resetActiveIndex = () => {
    activeIndex.value =  -1
}

const isExpanded = ref(false)
const containerWidth = ref(0)
const buttonWidth = ref(96) // 默认值
const moreBtnWidth = ref(14)
const layoutKey = ref(0)

// 模板引用
const functionButtonsRef = ref(null)
const buttonsContainerRef = ref(null)

const handleClick = (item, index) => {
    const found = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === props.currentAssistant.type)
    if (found) {
        if (found.index == index){
            activeIndex.value = -1
            found.index = -1
            emit('clearFunction')
        } else {
            activeIndex.value = index
            found.index = index
            emit('selectFunction', item)
        }
    }
}

const isShowMoreBtn = computed(() => {
    // 计算每行能放多少个按钮
    const perRow = buttonWidth.value > 0 ? Math.floor((containerWidth.value - moreBtnWidth.value) / buttonWidth.value) : 1
    // 计算总共需要多少行
    const totalRows = perRow > 0 ? Math.ceil(props.functionList.length / perRow) : 1
    // 超过一行才显示"更多"按钮
    return totalRows > 1
})

const updateContainerWidth = () => {
    nextTick(() => {
        setTimeout(() => {
            if (buttonsContainerRef.value) {
                containerWidth.value = buttonsContainerRef.value.offsetWidth
            }
            const button = buttonsContainerRef.value?.querySelector('.function-button')
            if (button && button.offsetWidth > 0) {
                buttonWidth.value = button.offsetWidth
            }
            layoutKey.value++ // 触发刷新
        }, 10)
    })
}

const isLastInRow = (index) => {
    const buttonsPerRow = buttonWidth.value > 0 ? Math.floor((containerWidth.value - moreBtnWidth.value) / buttonWidth.value) : 1
    return index === props.functionList.length - 1 || (index + 1) % buttonsPerRow === 0
}

onMounted(() => {
    // 监听窗口大小变化
    window.addEventListener('resize', updateContainerWidth)
    
    // 设置button hover事件来隐藏前后divider
    nextTick(() => {
        setupDividerHoverEffect()
    })
})

let hoverEventCleanup = []

const cleanupHoverEvents = () => {
    hoverEventCleanup.forEach(cleanup => cleanup())
    hoverEventCleanup = []
    
    // 清理所有divider的内联样式
    if (buttonsContainerRef.value) {
        const dividers = buttonsContainerRef.value.querySelectorAll('.divider')
        dividers.forEach(divider => {
            divider.style.removeProperty('opacity')
        })
    }
}

const setupDividerHoverEffect = () => {
    // 清理之前的事件监听器
    cleanupHoverEvents()
    
    // 使用更长的延迟确保DOM完全渲染
    setTimeout(() => {
        // 使用模板引用确保只操作当前组件的元素
        if (!buttonsContainerRef.value) return
        
        const buttonWrappers = Array.from(buttonsContainerRef.value.querySelectorAll('.button-wrapper'))
        
        buttonWrappers.forEach((wrapper, index) => {
            const button = wrapper.querySelector('.function-button')
            if (!button) return
            
            const handleMouseEnter = () => {
                // 只需要隐藏前一个wrapper的divider，当前的由CSS处理
                if (index > 0) {
                    const prevWrapper = buttonWrappers[index - 1]
                    const prevDivider = prevWrapper.querySelector('.divider')
                    if (prevDivider) {
                        prevDivider.style.opacity = '0'
                    }
                }
            }
            
            const handleMouseLeave = () => {
                // 恢复前一个wrapper的divider - 移除内联样式让CSS重新接管
                if (index > 0) {
                    const prevWrapper = buttonWrappers[index - 1]
                    const prevDivider = prevWrapper.querySelector('.divider')
                    if (prevDivider) {
                        prevDivider.style.removeProperty('opacity')
                    }
                }
            }
            
            button.addEventListener('mouseenter', handleMouseEnter)
            button.addEventListener('mouseleave', handleMouseLeave)
            
            // 保存清理函数
            hoverEventCleanup.push(() => {
                button.removeEventListener('mouseenter', handleMouseEnter)
                button.removeEventListener('mouseleave', handleMouseLeave)
            })
        })
    }, 100) // 增加延迟时间确保DOM完全更新
}

onUnmounted(() => {
    // 移除监听器
    window.removeEventListener('resize', updateContainerWidth)
    // 清理hover事件监听器
    cleanupHoverEvents()
})

const toggleExpand = () => {
    isExpanded.value = !isExpanded.value
}

// 监听visible，显示时再定位
watch(() => props.currentAssistant, (val) => {
  if (val) {
    // 初始计算容器宽度
    nextTick(() => {
        setTimeout(() => {
            updateContainerWidth()
            setupDividerHoverEffect()
        }, 100)
    })
  }
})

watch(() => props.isWindowMode, (val) => {
    // 初始计算容器宽度
    nextTick(() => {
        setTimeout(() => {
            updateContainerWidth()
            setupDividerHoverEffect()
        }, 100) // 增加延迟确保DOM完全更新
    })
})

watch(() => props.functionList, () => {
    // 功能列表变化时重新绑定事件
    nextTick(() => {
        setTimeout(() => {
            setupDividerHoverEffect()
        }, 100)
    })
})
defineExpose({ resetActiveIndex })
</script>

<style lang="scss" scoped>
.function-buttons {
    display: flex;
    flex-direction: column;
    margin-top: 6px;
    margin-bottom: 4px;
    position: relative;

    .buttons-container {
        display: flex;
        flex-wrap: wrap;
        max-height: 37px;
        margin-left: 10px;
        overflow: hidden;
        transition: max-height 0.3s ease;
        align-items: center;
        width: calc(100% - 14px);

        &.expanded {
            max-height: 1000px;
        }
    }

    .button-wrapper {
        display: flex;
        align-items: center;
        height: 37px;
    }

    .more-button {
        position: absolute;
        right: 8px;
        top: 0;
        cursor: pointer;
        // padding: 4px;
        display: flex;
        align-items: center;
        height: 37px;
    }

    .divider {
        width: 1px;
        height: 16px;
        background-color: var(--uosai-color-shortcut-border);
        transition: opacity 0.1s ease;
        // margin: 0 4px;
    }



    .button-wrapper {
        &:hover .divider {
            opacity: 0;
        }
    }

    .function-button {
        display: flex;
        flex-direction: row;
        align-items: center;
        justify-content: flex-start;
        min-width: 96px;
        height: 32px;
        border-radius: 8px;
        cursor: pointer;
        transition: all 0.3s ease;
        user-select: none;
        color: var(--uosai-color-function-button-text-color);
        
        .function-icon {
            width: 20px;
            height: 20px;
            margin-left: 8px;
            margin-right: 4px;
            margin-bottom: 0;
        }

        .function-text {
            font-size: 12px;
            color: inherit;
            text-align: left;
            padding-right: 10px;
        }

        &:hover {
            background-color: var(--uosai-color-clear-hover-bg);
        }

        &:active {
            color: var(--activityColor);
            background-color: var(--uosai-color-clear-press-bg);
            svg {
                fill: var(--activityColor);
            }
        }

        &.active {
            color: var(--activityColor);
            svg {
                fill: var(--activityColor);
            }
        }
    }

    .function-separator {
        width: 100%;
        height: 1px;
        background-color: var(--uosai-color-function-separator);
        margin-top: 4px;
    }
}
</style> 