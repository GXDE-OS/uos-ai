<template>
    <div v-if="visible" class="guide-overlay">
        <div class="guide-mask"></div>
        <div class="guide-popup" :style="popupStyle">
            <div class="guide-content">
                <span class="guide-title">{{ currentStep.title }}</span>
                <span class="guide-text">
                    <template v-if="Array.isArray(currentStep.content)">
                        <template v-for="(item, index) in currentStep.content" :key="index">
                            <span v-if="item.type === 'text'">{{ item.value }}</span>
                            <SvgIcon v-if="item.type === 'icon'" :icon="item.value" :style="{'fill': 'var(--uosai-color-flat-btn-icon)', 'width': '13.54px', 'height': '12.77px', 'margin-right': '2px', 'margin-left': '2px', 'display': 'inline', 'vertical-align': 'middle'}"/>
                        </template>
                    </template>
                    <template v-else>
                        {{ currentStep.content }}
                    </template>
                </span>
                <span v-if="currentStep.contentText" class="guide-content-text">{{ currentStep.contentText }}</span>
                <div class="guide-btn-group">
                    <span class="step-progress" v-if="guideType === 'sequential' && totalSteps > 1">{{ currentStepIndex + 1 }}/{{ totalSteps }}</span>
                    <span v-if="currentStep.useLaterText" class="close-btn" @click="closeGuide(false)" style="color: var(--uosai-guide-popup-trylater-color)">{{ currentStep.useLaterText }}</span>
                    <span class="close-btn" @click="activeBtn">{{ currentStep.activeText }}</span>
                </div>
            </div>
            <div class="guide-arrow" :style="arrayStyle"></div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed } from 'vue'
import SvgIcon from "@/components/svgIcon/svgIcon.vue"

const props = defineProps({
    steps: {
        type: Array,
        default: () => [],
        required: true
    },
    visible: Boolean,
    startStep: {
        type: Number,
        default: 0
    },
    guideType: {
        type: String,
        default: 'independent',
        validator: (value) => ['independent', 'sequential'].includes(value)
    },
    isWindowMode: Boolean,
    isDarkMode: Boolean
})

const emit = defineEmits(['close'])

const popupStyle = ref({})
const arrayStyle = ref({})

const currentStepIndex = ref(props.startStep)

// 存储克隆的目标元素
let clonedTarget = null

const totalSteps = computed(() => props.steps.length)
const currentStep = computed(() => {
    if (props.steps && props.steps.length > currentStepIndex.value) {
        return props.steps[currentStepIndex.value]
    }
    return null
})

let resizeObserver = null
let themeObserver = null

const updateClonedTargetPosition = () => {
    if (!clonedTarget || !currentStep.value || !currentStep.value.targetRef) return

    const el = currentStep.value.targetRef
    if (!el || typeof el.getBoundingClientRect !== 'function') return

    const rect = el.getBoundingClientRect()
    clonedTarget.style.left = `${rect.left}px`
    clonedTarget.style.top = `${rect.top}px`
    clonedTarget.style.width = `${rect.width}px`
    clonedTarget.style.height = `${rect.height}px`
}

const updatePosition = () => {
    console.log('updatePosition')

    if (!currentStep.value) return

    // 更新克隆元素的位置
    updateClonedTargetPosition()
    
    if (currentStep.value.targetRef === "none") {
        // 根据 relativePosition 计算弹窗位置
        const { arrayDirect, left = 0, right = 0, top = 0, bottom = 0 , titleBarBtnWidth = 0, arrayDisplay = '-1', position = 'none'} = currentStep.value.relativePosition

        // 获取 guide-popup 的 DOM 元素
        let popupDom = document.querySelector('.guide-popup')
        let popupWidth = 280 // 默认宽度
        let popupHeight = 120 // 默认高度
        let arrowHeight = 10 // 箭头高度
        if (popupDom) {
            popupWidth = popupDom.offsetWidth
            popupHeight = popupDom.offsetHeight
        }
        switch (position) {
            case 'none':
                {
                    // 获取 main-content 的 DOM 元素
                    let mainContentDom = document.querySelector('.main-content')
                    let mainContentWidth = 0
                    let mainContentHeight = 0
                    if (mainContentDom) {
                        mainContentWidth = mainContentDom.offsetWidth
                        mainContentHeight = mainContentDom.offsetHeight
                    }

                    const right_ = right.replace('px', '')
                    const top_ = top.replace('px', '')
                    const popupLeft = mainContentWidth - popupWidth - parseInt(right_)
                    const popupTop = parseInt(top_)
                    
                    // 根据 relativePosition 设置弹窗位置       
                    popupStyle.value = {
                        position: 'fixed',
                        left: `${popupLeft}px`,
                        top: `${popupTop}px`,
                        zIndex: 10001
                    }
                    
                    // 根据 arrayDirect 设置箭头方向
                    let arrowPosition = 0
                    let arrowTransform = ''
                    let arrowTop = ''
                    let arrowBottom = ''
                    let arrowLeft = ''
                    let arrowRight = ''

                    switch (arrayDirect) {
                        case 'left':
                            arrowPosition = popupWidth
                            arrowTransform = 'rotate(90deg)'
                            arrowLeft = '-10px'
                            arrowTop = '50%'
                            arrowBottom = 'auto'
                            arrowRight = 'auto'
                            break
                        case 'right':
                            arrowPosition = popupWidth
                            arrowTransform = 'rotate(-90deg)'
                            arrowRight = '-10px'
                            arrowTop = '50%'
                            arrowBottom = 'auto'
                            arrowLeft = 'auto'
                            break
                        case 'top':
                            if (props.isWindowMode) {
                                arrowPosition = popupWidth - parseInt(right_) -  titleBarBtnWidth * 3.5 + 10
                            }else {
                                arrowPosition = popupWidth - parseInt(right_) -  titleBarBtnWidth * 1.5 + 10
                            }
                            
                            arrowTransform = 'rotate(180deg)'
                            arrowTop = '-10px'
                            arrowLeft = '50%'
                            arrowRight = 'auto'
                            arrowBottom = 'auto'
                            break
                        case 'bottom':
                            arrowPosition = popupWidth / 2
                            arrowTransform = 'rotate(0deg)'
                            arrowBottom = '-10px'
                            arrowLeft = '50%'
                            arrowRight = 'auto'
                            arrowTop = 'auto'
                            break
                        default:
                            arrowPosition = popupWidth / 2
                            arrowTransform = 'rotate(0deg)'
                            arrowBottom = '-10px'
                            arrowLeft = '50%'
                            arrowRight = 'auto'
                            arrowTop = 'auto'
                    }
                    
                    arrayStyle.value = {
                        left: `${arrowPosition}px`,
                        right: arrowRight,
                        top: arrowTop,
                        bottom: arrowBottom,
                        transform: arrowTransform
                    }
                }
                break;
            case 'center':
                {
                    // 获取 main-content 的 DOM 元素
                    let mainContentDom = document.querySelector('.main-content')
                    let mainContentWidth = 0
                    let mainContentHeight = 0
                    if (mainContentDom) {
                        mainContentWidth = mainContentDom.offsetWidth
                        mainContentHeight = mainContentDom.offsetHeight
                    }

                    // 根据 relativePosition 设置弹窗位置       
                    popupStyle.value = {
                        position: 'fixed',
                        left: `${(mainContentWidth - popupWidth) / 2}px`,
                        top: `${(mainContentHeight - popupHeight) / 2}px`,
                        zIndex: 10001
                    }
                }
                break;
            default:
                break;
        }

        if (arrayDisplay === 'none') {
            // 隐藏箭头
            arrayStyle.value = {
                display: 'none'
            }

            console.log('隐藏箭头', arrayStyle.value)
        }

    } else {
        const el = currentStep.value.targetRef
        if (!el || typeof el.getBoundingClientRect !== 'function' || el.nodeType !== 1) {
            return
        }
        const rect = el.getBoundingClientRect()

        // 获取元素的计算样式以获取padding值
        const computedStyle = window.getComputedStyle(el)
        const paddingLeft = parseFloat(computedStyle.paddingLeft) || 0
        const paddingRight = parseFloat(computedStyle.paddingRight) || 0

        // 弹窗高度假设为120px，箭头高度10px，弹窗底部对齐targetRef顶部
        // 获取guide-popup的DOM元素
        let popupDom = document.querySelector('.guide-popup')
        let popupWidth = 280 // 默认宽度
        let popupHeight = 120 // 默认高度
        let arrowHeight = 10 // 箭头高度
        if (popupDom) {
            popupWidth = popupDom.offsetWidth
            popupHeight = popupDom.offsetHeight
        }

        // 获取窗口宽度
        const windowWidth = window.innerWidth
        const margin = 4 // 距离边缘的最小间距

        // 计算去除padding后的内容宽度
        const contentWidth = rect.width - paddingLeft - paddingRight
        let left = rect.left + (contentWidth - popupWidth) / 2

        // 检查左边界
        if (left < margin) {
            left = margin
        }

        // 检查右边界
        if (left + popupWidth > windowWidth - margin) {
            left = windowWidth - popupWidth - margin
        }

        popupStyle.value = {
            position: 'fixed',
            left: `${left}px`,
            top: `${rect.top - popupHeight - arrowHeight * 2}px`,
            zIndex: 10001
        }

        // 计算目标元素的中心点
        const targetCenter = rect.left + rect.width / 2
        // 计算弹出框的左边界
        const popupLeft = left
        // 计算箭头应该在弹出框中的位置（减去箭头宽度的一半：8px）
        const arrowPosition = targetCenter - popupLeft - 8

        // 确保箭头不会太靠近弹出框边缘
        const minArrowPosition = 10 // 距离左边至少10px
        const maxArrowPosition = popupWidth - 10 // 距离右边至少10px

        const finalArrowPosition = Math.min(Math.max(arrowPosition, minArrowPosition), maxArrowPosition)

        arrayStyle.value = {
            left: `${finalArrowPosition}px`,
            right: 'auto'
        }
    }
}

const resetTargetStyle = (targetRef) => {
    // 移除克隆的目标元素
    if (clonedTarget && clonedTarget.parentNode) {
        clonedTarget.parentNode.removeChild(clonedTarget)
        clonedTarget = null
    }

    // 恢复原始元素的样式
    if (targetRef && targetRef.style) {
        targetRef.style.zIndex = ''
        targetRef.style.pointerEvents = ''
        targetRef.style.opacity = ''
    }
}

const closeGuide = (isActive) => {
    emit('close', isActive)
    if (currentStep.value) {
        resetTargetStyle(currentStep.value.targetRef)
    }
}

// 递归复制元素及其子元素的计算样式
const copyComputedStyles = (sourceElement, targetElement) => {
    if (!sourceElement || !targetElement) return

    const computedStyle = window.getComputedStyle(sourceElement)

    // 复制所有计算样式
    for (let i = 0; i < computedStyle.length; i++) {
        const property = computedStyle[i]
        const value = computedStyle.getPropertyValue(property)
        try {
            targetElement.style.setProperty(property, value)
        } catch (e) {
            // 某些属性可能无法设置,忽略错误
        }
    }

    // 递归处理子元素
    const sourceChildren = sourceElement.children
    const targetChildren = targetElement.children
    for (let i = 0; i < sourceChildren.length && i < targetChildren.length; i++) {
        copyComputedStyles(sourceChildren[i], targetChildren[i])
    }
}

// 刷新克隆元素的样式(用于主题变化时更新)
const refreshClonedTargetStyles = () => {
    if (!clonedTarget || !currentStep.value || !currentStep.value.targetRef) return

    const el = currentStep.value.targetRef
    if (!el || typeof el.getBoundingClientRect !== 'function') return

    // 重新复制样式
    copyComputedStyles(el, clonedTarget)

    // 恢复必要的定位样式
    const rect = el.getBoundingClientRect()
    clonedTarget.style.position = 'fixed'
    clonedTarget.style.left = `${rect.left}px`
    clonedTarget.style.top = `${rect.top}px`
    clonedTarget.style.width = `${rect.width}px`
    clonedTarget.style.height = `${rect.height}px`
    clonedTarget.style.pointerEvents = currentStep.value.isAllowClickOnTarget ? 'pointer' : 'none'
    clonedTarget.style.zIndex = currentStep.value.isAllowClickOnTarget ? '10001' : '10000'
    clonedTarget.style.margin = '0'
    clonedTarget.style.opacity = '1'
}

const highlightTarget = () => {
    if (!currentStep.value || !currentStep.value.targetRef) return

    const el = currentStep.value.targetRef
    if (!el || typeof el.getBoundingClientRect !== 'function' || el.nodeType !== 1) {
        return
    }

    // 移除之前的克隆元素
    if (clonedTarget && clonedTarget.parentNode) {
        clonedTarget.parentNode.removeChild(clonedTarget)
        clonedTarget = null
    }

    // 隐藏原始元素,避免与克隆元素产生视觉叠加
    el.style.opacity = '0'

    // 获取目标元素的位置
    const rect = el.getBoundingClientRect()

    // 克隆目标元素
    clonedTarget = el.cloneNode(true)

    // 递归复制所有计算样式(包括子元素和SVG)
    copyComputedStyles(el, clonedTarget)

    // 确保克隆元素的基本定位样式不被覆盖
    clonedTarget.style.position = 'fixed'
    clonedTarget.style.left = `${rect.left}px`
    clonedTarget.style.top = `${rect.top}px`
    clonedTarget.style.width = `${rect.width}px`
    clonedTarget.style.height = `${rect.height}px`
    clonedTarget.style.pointerEvents = currentStep.value.isAllowClickOnTarget ? 'pointer' : 'none'
    clonedTarget.style.zIndex = currentStep.value.isAllowClickOnTarget ? '10001' : '10000'
    clonedTarget.style.margin = '0'
    clonedTarget.style.opacity = '1'

    // 添加一个标识class
    clonedTarget.classList.add('guide-cloned-target')

    // 将克隆元素添加到body,确保不受父级disabled影响
    document.body.appendChild(clonedTarget)

    // 如果允许点击,需要将点击事件代理到原始元素
    if (currentStep.value.isAllowClickOnTarget) {
        clonedTarget.addEventListener('click', (e) => {
            e.stopPropagation()
            el.click()
        })
    }
}

const activeBtn = () => {
    if (currentStep.value && currentStep.value.onActiveClick) {
        currentStep.value.onActiveClick()
    }

    if (currentStepIndex.value < totalSteps.value - 1) {
        resetTargetStyle(currentStep.value.targetRef)
        currentStepIndex.value++
    } else {
        closeGuide(true)
    }
}

onMounted(() => {
    window.addEventListener('resize', updatePosition)
    window.addEventListener('scroll', updatePosition, true)

    // 使用 MutationObserver 监听主题变化
    // 监听 HTML 元素的 class 或 data-theme 属性变化
    themeObserver = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            // 检查是否是 class 或相关属性的变化
            if (mutation.type === 'attributes' &&
                (mutation.attributeName === 'class' ||
                 mutation.attributeName === 'data-theme' ||
                 mutation.attributeName === 'style')) {
                // 主题变化时刷新克隆元素样式
                if (props.visible && clonedTarget) {
                    nextTick(() => {
                        refreshClonedTargetStyles()
                    })
                }
                break
            }
        }
    })

    // 监听 document.documentElement (html 标签) 的属性变化
    themeObserver.observe(document.documentElement, {
        attributes: true,
        attributeFilter: ['class', 'data-theme', 'style']
    })

    // 也监听 body 元素的变化
    themeObserver.observe(document.body, {
        attributes: true,
        attributeFilter: ['class', 'data-theme', 'style']
    })
})

onUnmounted(() => {
    window.removeEventListener('resize', updatePosition)
    window.removeEventListener('scroll', updatePosition, true)

    if (resizeObserver) {
        resizeObserver.disconnect()
        resizeObserver = null
    }

    if (themeObserver) {
        themeObserver.disconnect()
        themeObserver = null
    }
})

watch(() => props.visible, (val) => {
    if (val) {
        currentStepIndex.value = props.startStep
        nextTick(() => {
            if (currentStep.value) {
                updatePosition()
                highlightTarget()

                const popupDom = document.querySelector('.guide-popup')
                if (popupDom && !resizeObserver) {
                    resizeObserver = new ResizeObserver(() => {
                        updatePosition()
                    })
                    resizeObserver.observe(popupDom)
                }
            }
        })
    } else {
        if (resizeObserver) {
            resizeObserver.disconnect()
            resizeObserver = null
        }
    }
})

watch(currentStepIndex, (newIndex, oldIndex) => {
    if (props.visible) {
        const oldStep = props.steps[oldIndex]
        if (oldStep) {
            resetTargetStyle(oldStep.targetRef)
        }
        
        nextTick(() => {
            if (currentStep.value) {
                updatePosition()
                highlightTarget()
            }
        })
    }
}, { immediate: true })
</script>

<style lang="scss" scoped>
.guide-overlay {
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    z-index: 10000;
    pointer-events: auto;
}
.guide-mask {
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background-color: rgba(0, 0, 0, 0); // var(--uosai-history-list-mask-bg); 
    z-index: 10000;
}
.guide-popup {
    width: 280px;
    box-sizing: border-box;
    background-color: var(--uosai-guide-popup-bg);
    border-radius: 10px;
    border: 1px solid rgba(0, 0, 0, 0.1);
    box-shadow: 0 4px 12px rgba(0,0,0,0.18);
    padding-top: 20px;
    padding-left: 20px;
    padding-right: 20px;
    padding-bottom: 16px;
    position: fixed;
    z-index: 10001;
    display: flex;
    flex-direction: column;
    align-items: center;
}
.guide-content {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    color: var(--uosai-guide-popup-font-color);
    width: 100%;
}

.guide-title {
    font-size: 1.214rem; /* 17px */
    font-weight: bold;
}

.guide-text {
    font-size: 0.857rem; /* 12px */
    font-weight: 400;
    margin-top: 6px;
    margin-bottom: 10px;
}

.guide-content-text {
    font-size: 0.857rem; /* 12px */
    font-weight: 400;
    margin-bottom: 15px;
    color: var(--uosai-guide-popup-contentText-color);
}

.guide-btn-group {
    display: flex;
    justify-content: flex-end;
    align-items: center;
    width: 100%;
}

.step-progress {
    font-size: 0.857rem; /* 12px */
    color: var(--uosai-guide-popup-contentText-color);
    margin-right: auto;
}

.close-btn {
    margin-top: 0;
    margin-left: 16px;
    color: var(--activityColor);
    border-radius: 4px;
    cursor: pointer;
}
.guide-arrow {
    width: 0;
    height: 0;
    border-left: 8px solid transparent;
    border-right: 8px solid transparent;
    border-top: 10px solid rgba(0, 0, 0, 0.05);
    border-bottom: 0;
    margin: 0;
    position: absolute;
    bottom: 0;
    transform: translateY(100%);
    z-index: 10002;

    &::after {
        content: '';
        position: absolute;
        width: 0;
        height: 0;
        border-left: 8px solid transparent;
        border-right: 8px solid transparent;
        border-top: 10px solid var(--uosai-guide-popup-bg);
        left: -8px;
        top: -11px;
    }
}

// 克隆的目标元素样式
:deep(.guide-cloned-target) {
    // 确保克隆元素具有完整的视觉效果
    box-sizing: border-box;

    // 添加高亮效果,使其更加醒目
    outline: 2px solid var(--activityColor, #0081FF);
    outline-offset: 2px;

    // 可以添加一些过渡效果
    transition: outline 0.3s ease;

    // 禁用所有子元素的交互(除非特别允许)
    * {
        pointer-events: none;
    }
}
</style> 