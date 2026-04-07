<template>
    <div v-if="visible" class="guide-overlay">
        <div class="guide-mask"></div>
        <div class="guide-popup" :style="{ width: `${currentPanelWidth}px`, ...popupStyle }">
            <div class="guide-content">
                <span class="guide-title">{{ currentStep.title }}</span>
                <span class="guide-text">
                    <template v-if="Array.isArray(currentStep.content)">
                        <template v-for="(item, index) in currentStep.content" :key="index">
                            <span v-if="item.type === 'text'">{{ item.value }}</span>
                            <SvgIcon
                                v-if="item.type === 'icon'"
                                :icon="item.value"
                                :style="{
                                    fill: 'var(--uosai-color-flat-btn-icon)',
                                    width: '13.54px',
                                    height: '12.77px',
                                    'margin-right': '2px',
                                    'margin-left': '2px',
                                    display: 'inline',
                                    'vertical-align': 'middle',
                                }"
                            />
                        </template>
                    </template>
                    <template v-else>
                        {{ currentStep.content }}
                    </template>
                </span>
                <span v-if="currentStep.contentText" class="guide-content-text">{{ currentStep.contentText }}</span>
                <div class="guide-btn-group">
                    <span class="step-progress" v-if="guideType === 'sequential' && totalSteps > 1"
                        >{{ currentStepIndex + 1 }}/{{ totalSteps }}</span
                    >
                    <span
                        v-if="currentStep.useLaterText"
                        class="close-btn"
                        @click="closeGuide(false)"
                        style="color: var(--uosai-guide-popup-trylater-color)"
                        >{{ currentStep.useLaterText }}</span
                    >
                    <span class="close-btn" @click="activeBtn">{{ currentStep.activeText }}</span>
                </div>
            </div>
            <div class="guide-arrow" :style="arrowStyle"></div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed } from "vue";
import SvgIcon from "@/components/svgIcon/svgIcon.vue";

const props = defineProps({
    steps: {
        type: Array,
        default: () => [],
        required: true,
    },
    visible: Boolean,
    startStep: {
        type: Number,
        default: 0,
    },
    guideType: {
        type: String,
        default: "independent",
        validator: (value) => ["independent", "sequential"].includes(value),
    },
    isWindowMode: Boolean,
    isDarkMode: Boolean,
    // New props for multi-target support
    targets: {
        type: Array,
        default: () => [],
    },
    primaryTarget: {
        type: Object, // DOM Element
        default: null,
    },
});

const emit = defineEmits(["close"]);

const popupStyle = ref({});
const arrowStyle = ref({});

const currentStepIndex = ref(props.startStep);

// 存储克隆的目标元素数组
let clonedTargets = [];
// 存储目标元素的原始 display 值
const originalDisplayValues = new Map();

const totalSteps = computed(() => props.steps.length);
const currentStep = computed(() => {
    if (props.steps && props.steps.length > currentStepIndex.value) {
        return props.steps[currentStepIndex.value];
    }
    return null;
});

const currentPanelWidth = computed(() => {
    return currentStep.value?.width || 280;
});

const getActiveTargets = computed(() => {
    // 新的targets有值时使用targets，否则向下兼容使用targetRef
    if (props.steps && props.steps[currentStepIndex.value]) {
        const step = props.steps[currentStepIndex.value];
        if (step.targets && step.targets.length > 0) {
            return step.targets;
        }
        // 旧的targetRef兼容
        if (step.targetRef && step.targetRef !== "none") {
            return [step.targetRef];
        }
    }
    return [];
});

const getPrimaryTarget = computed(() => {
    // 新的primaryTarget有值时使用，否则取第一个目标
    if (props.steps && props.steps[currentStepIndex.value]) {
        const step = props.steps[currentStepIndex.value];
        if (step.primaryTarget) {
            return step.primaryTarget;
        }
        const activeTargets = getActiveTargets.value;
        if (activeTargets.length > 0) {
            return activeTargets[0];
        }
    }
    return null;
});

let resizeObserver = null;
let themeObserver = null;
let highlightRetryTimeout = null;
let updatePositionRetryTimeout = null;
let updatePositionRetryCount = 0;
const MAX_UPDATE_POSITION_RETRIES = 5;

// 检查元素样式是否有效（尺寸和位置都已计算完成）
const isTargetStyleValid = (target) => {
    if (!target || typeof target.getBoundingClientRect !== "function") {
        return false;
    }

    const rect = target.getBoundingClientRect();

    // 检查是否有有效的尺寸
    if (rect.width === 0 && rect.height === 0) {
        return false;
    }

    // 检查位置是否有效（不是全为0或负数）
    if (rect.left === 0 && rect.top === 0 && rect.right === 0 && rect.bottom === 0) {
        return false;
    }

    // 检查计算样式是否有效
    try {
        const computedStyle = window.getComputedStyle(target);
        // 检查关键样式属性是否有值
        if (!computedStyle || computedStyle.length === 0) {
            return false;
        }
    } catch (e) {
        return false;
    }

    return true;
};

const updateClonedTargetPosition = () => {
    const targets = getActiveTargets.value;
    let checkRet = true;
    targets.forEach((target, index) => {
        if (!clonedTargets[index] || !target) {
            checkRet = false;
            return;
        }

        // 检查元素样式是否有效
        if (!isTargetStyleValid(target)) {
            // 保存原始 display 值（如果还没保存过）
            if (!originalDisplayValues.has(target)) {
                originalDisplayValues.set(target, target.style.display);
            }

            // 尝试恢复原始 display 值，如果原始值是空或 none，则尝试 flex
            const originalDisplay = originalDisplayValues.get(target);
            if (originalDisplay && originalDisplay !== "none") {
                target.style.display = originalDisplay;
            } else {
                target.style.display = "flex";
            }

            // 样式无效，需要重试
            if (updatePositionRetryCount < MAX_UPDATE_POSITION_RETRIES) {
                if (updatePositionRetryTimeout) {
                    clearTimeout(updatePositionRetryTimeout);
                }
                updatePositionRetryCount++;
                updatePositionRetryTimeout = setTimeout(() => {
                    updatePosition();
                }, 50);
            } else {
                // 多次重试无效，关闭新手引导
                console.warn("GuideOverlay: Target styles invalid after max retries, closing guide");
                closeGuide(false);
            }
            checkRet = false;
            return;
        }

        const rect = target.getBoundingClientRect();
        clonedTargets[index].style.left = `${rect.left}px`;
        clonedTargets[index].style.top = `${rect.top}px`;
        clonedTargets[index].style.width = `${rect.width}px`;
        clonedTargets[index].style.height = `${rect.height}px`;
    });

    return checkRet;
};

const updatePosition = () => {
    if (!getActiveTargets.value || getActiveTargets.value.length === 0) return;

    // 更新所有克隆元素的位置
    if (!updateClonedTargetPosition()) return;
    // 重置重试计数
    updatePositionRetryCount = 0;

    const primary = getPrimaryTarget.value;
    if (!primary) return;

    // 获取guide-popup的DOM元素和尺寸
    const popupDom = document.querySelector(".guide-popup");
    const popupWidth = popupDom?.offsetWidth || 280;
    const popupHeight = popupDom?.offsetHeight || 120;
    const arrowHeight = 10;

    if (currentStep.value?.relativePosition && currentStep.value.relativePosition.position !== undefined) {
        const {
            arrowDirect,
            left = 0,
            right = 0,
            top = 0,
            bottom = 0,
            titleBarBtnWidth = 0,
            arrowDisplay = "-1",
            position = "none",
            offset = 10,
        } = currentStep.value.relativePosition;

        // 当有 primary target 且配置了 arrowDirect 时，基于目标元素定位
        if (primary && arrowDirect && ["left", "right", "top", "bottom"].includes(arrowDirect)) {
            const rect = primary.getBoundingClientRect();
            const spacing = parseInt(String(offset)) || 10;

            let popupLeft, popupTop;
            let arrowLeft = "50%",
                arrowRight = "auto",
                arrowTop = "50%",
                arrowBottom = "auto";
            let arrowTransform = "rotate(0deg)";

            switch (arrowDirect) {
                case "left":
                    // 箭头指向左边，浮窗在目标右侧
                    popupLeft = rect.right + spacing;
                    popupTop = rect.top + (rect.height - popupHeight) / 2;
                    arrowTransform = "rotate(90deg)";
                    arrowRight = "0px";
                    break;
                case "right":
                    // 箭头指向右边，浮窗在目标左侧
                    popupLeft = rect.left - popupWidth - spacing;
                    popupTop = rect.top + (rect.height - popupHeight) / 2;
                    arrowTransform = "rotate(-90deg)";
                    arrowLeft = "calc(100% - 2px)";
                    break;
                case "top":
                    // 箭头指向上边，浮窗在目标下方
                    popupLeft = rect.left + (rect.width - popupWidth) / 2;
                    popupTop = rect.bottom + spacing;
                    arrowTransform = "rotate(180deg)";
                    arrowTop = "-10px";
                    break;
                case "bottom":
                    // 箭头指向下边，浮窗在目标上方
                    popupLeft = rect.left + (rect.width - popupWidth) / 2;
                    popupTop = rect.top - popupHeight - spacing;
                    arrowTransform = "rotate(0deg)";
                    arrowBottom = "100%";
                    break;
            }

            popupStyle.value = {
                position: "fixed",
                left: `${popupLeft}px`,
                top: `${popupTop}px`,
                zIndex: 10001,
            };

            if (arrowDisplay === "none") {
                arrowStyle.value = { display: "none" };
            } else {
                arrowStyle.value = {
                    left: arrowLeft,
                    right: arrowRight,
                    top: arrowTop,
                    bottom: arrowBottom,
                    transform: arrowTransform,
                };
            }
            return;
        }

        // 处理 'none' 或 'center' 位置
        const mainContentDom = document.querySelector(".main-content");
        const mainContentWidth = mainContentDom?.offsetWidth || 0;
        const mainContentHeight = mainContentDom?.offsetHeight || 0;

        switch (position) {
            case "none": {
                const right_ = parseInt(right.replace("px", "")) || 0;
                const top_ = parseInt(top.replace("px", "")) || 0;
                const popupLeft = mainContentWidth - popupWidth - right_;
                const popupTop = top_;

                popupStyle.value = {
                    position: "fixed",
                    left: `${popupLeft}px`,
                    top: `${popupTop}px`,
                    zIndex: 10001,
                };

                if (arrowDisplay === "none") {
                    arrowStyle.value = { display: "none" };
                    return;
                }

                let arrowLeft = "50%",
                    arrowRight = "auto",
                    arrowTop = "auto",
                    arrowBottom = "-10px";
                let arrowTransform = "rotate(0deg)";

                switch (arrowDirect) {
                    case "left":
                        arrowLeft = `-10px`;
                        arrowTop = "50%";
                        arrowBottom = "auto";
                        arrowTransform = "rotate(-90deg)";
                        break;
                    case "right":
                        arrowRight = `-10px`;
                        arrowTop = "50%";
                        arrowBottom = "auto";
                        arrowTransform = "rotate(90deg)";
                        break;
                    case "top":
                        arrowTop = `-10px`;
                        arrowBottom = "auto";
                        arrowTransform = "rotate(0deg)";
                        break;
                    case "bottom":
                        arrowTop = "auto";
                        arrowBottom = "-10px";
                        arrowTransform = "rotate(180deg)";
                        break;
                    default:
                        break;
                }

                arrowStyle.value = {
                    left: arrowLeft,
                    right: arrowRight,
                    top: arrowTop,
                    bottom: arrowBottom,
                    transform: arrowTransform,
                };
                break;
            }
            case "center":
                popupStyle.value = {
                    position: "fixed",
                    left: `${(mainContentWidth - popupWidth) / 2}px`,
                    top: `${(mainContentHeight - popupHeight) / 2}px`,
                    zIndex: 10001,
                };
                break;
            default:
                break;
        }

        if (arrowDisplay === "none") {
            arrowStyle.value = { display: "none" };
        }
        return;
    }

    // 默认情况：相对目标元素定位在上方
    const rect = primary.getBoundingClientRect();
    const computedStyle = window.getComputedStyle(primary);
    const paddingLeft = parseFloat(computedStyle.paddingLeft) || 0;
    const paddingRight = parseFloat(computedStyle.paddingRight) || 0;
    const windowWidth = window.innerWidth;
    const margin = 4;

    const contentWidth = rect.width - paddingLeft - paddingRight;
    let left = rect.left + (contentWidth - popupWidth) / 2;

    // 边界检查
    left = Math.max(margin, Math.min(left, windowWidth - popupWidth - margin));

    popupStyle.value = {
        position: "fixed",
        left: `${left}px`,
        top: `${rect.top - popupHeight - arrowHeight * 2}px`,
        zIndex: 10001,
    };

    // 计算箭头位置
    const targetCenter = rect.left + rect.width / 2;
    const arrowPosition = targetCenter - left - 8;
    const finalArrowPosition = Math.min(Math.max(arrowPosition, 10), popupWidth - 10);

    arrowStyle.value = {
        left: `${finalArrowPosition}px`,
        right: "auto",
    };
};

const resetTargetStyle = (specificTarget = null) => {
    // 恢复特定目标或所有目标的样式
    if (specificTarget) {
        specificTarget.style.opacity = "";
        specificTarget.style.zIndex = "";
        specificTarget.style.pointerEvents = "";
        // 恢复原始 display 值
        if (originalDisplayValues.has(specificTarget)) {
            const originalDisplay = originalDisplayValues.get(specificTarget);
            specificTarget.style.display = originalDisplay;
            originalDisplayValues.delete(specificTarget);
        }
    } else {
        const activeTargets = getActiveTargets.value;
        activeTargets.forEach((target) => {
            if (target && target.style) {
                target.style.opacity = "";
                target.style.zIndex = "";
                target.style.pointerEvents = "";
                // 恢复原始 display 值
                if (originalDisplayValues.has(target)) {
                    const originalDisplay = originalDisplayValues.get(target);
                    target.style.display = originalDisplay;
                    originalDisplayValues.delete(target);
                }
            }
        });
    }

    // 移除所有克隆的目标元素
    clonedTargets.forEach((cloned) => {
        if (cloned && cloned.parentNode) {
            cloned.parentNode.removeChild(cloned);
        }
    });
    clonedTargets = [];
};

const closeGuide = (isActive) => {
    emit("close", isActive);
    resetTargetStyle();
};

// 递归复制元素及其子元素的计算样式
const copyComputedStyles = (sourceElement, targetElement) => {
    if (!sourceElement || !targetElement) return;

    const computedStyle = window.getComputedStyle(sourceElement);

    // 复制所有计算样式
    for (let i = 0; i < computedStyle.length; i++) {
        const property = computedStyle[i];
        const value = computedStyle.getPropertyValue(property);
        try {
            targetElement.style.setProperty(property, value);
        } catch (e) {
            // 某些属性可能无法设置,忽略错误
        }
    }

    // 递归处理子元素
    const sourceChildren = sourceElement.children;
    const targetChildren = targetElement.children;
    for (let i = 0; i < sourceChildren.length && i < targetChildren.length; i++) {
        copyComputedStyles(sourceChildren[i], targetChildren[i]);
    }
};

// 刷新克隆元素的样式(用于主题变化时更新)
const refreshClonedTargetStyles = () => {
    if (!clonedTargets.length || !getActiveTargets.value || getActiveTargets.value.length === 0) return;

    const targets = getActiveTargets.value;
    targets.forEach((target, index) => {
        if (!clonedTargets[index] || !target) return;

        // 重新复制样式
        copyComputedStyles(target, clonedTargets[index]);

        // 恢复必要的定位样式
        const rect = target.getBoundingClientRect();
        clonedTargets[index].style.position = "fixed";
        clonedTargets[index].style.left = `${rect.left}px`;
        clonedTargets[index].style.top = `${rect.top}px`;
        clonedTargets[index].style.width = `${rect.width}px`;
        clonedTargets[index].style.height = `${rect.height}px`;
        clonedTargets[index].style.pointerEvents = currentStep.value?.isAllowClickOnTarget ? "auto" : "none";
        clonedTargets[index].style.zIndex = currentStep.value?.isAllowClickOnTarget ? "10001" : "10000";
        clonedTargets[index].style.margin = "0";
        clonedTargets[index].style.opacity = "1";
        clonedTargets[index].style.boxSizing = "border-box";

        // 显式重置可能影响定位的属性
        clonedTargets[index].style.transform = "none";
        clonedTargets[index].style.transition = "none";
        clonedTargets[index].style.animation = "none";
    });
};

const highlightTargets = (retryCount = 0, maxRetries = 10) => {
    const targets = getActiveTargets.value;
    if (!targets || targets.length === 0) return;

    // 清除之前的克隆元素
    resetTargetStyle();

    // 过滤出有效的目标元素
    const validTargets = targets.filter((target) => {
        return target && target.style && typeof target.getBoundingClientRect === "function" && target.nodeType === 1;
    });

    if (validTargets.length === 0) return;

    // 检查所有目标元素样式是否有效
    const allStylesValid = validTargets.every((target) => isTargetStyleValid(target));

    if (!allStylesValid) {
        // 样式未就绪，重试
        if (retryCount < maxRetries) {
            if (highlightRetryTimeout) {
                clearTimeout(highlightRetryTimeout);
            }
            highlightRetryTimeout = setTimeout(() => {
                highlightTargets(retryCount + 1, maxRetries);
            }, 50);
        } else {
            // 超时后强制执行
            console.warn("GuideOverlay: Target styles not ready after max retries, proceeding anyway");
        }
        return;
    }

    // 样式就绪，执行高亮逻辑
    validTargets.forEach((target) => {
        // 隐藏原始元素
        target.style.opacity = "0";

        // 获取目标元素的位置
        const rect = target.getBoundingClientRect();

        // 克隆目标元素
        const cloned = target.cloneNode(true);

        // 递归复制所有计算样式
        copyComputedStyles(target, cloned);

        // 将克隆元素添加到body（在添加后再设置样式，确保样式在DOM中生效）
        document.body.appendChild(cloned);

        // 添加标识class
        cloned.classList.add("guide-cloned-target");

        // 设置克隆元素的基本定位样式（必须在添加到DOM后设置，避免在添加前就设置了某些影响定位的样式）
        cloned.style.position = "fixed";
        cloned.style.left = `${rect.left}px`;
        cloned.style.top = `${rect.top}px`;
        cloned.style.width = `${rect.width}px`;
        cloned.style.height = `${rect.height}px`;
        cloned.style.pointerEvents = currentStep.value?.isAllowClickOnTarget ? "auto" : "none";
        cloned.style.zIndex = currentStep.value?.isAllowClickOnTarget ? "10001" : "10000";
        cloned.style.margin = "0";
        cloned.style.opacity = "1";
        cloned.style.boxSizing = "border-box";

        // 显式重置可能影响定位的属性
        cloned.style.transform = "none";
        cloned.style.transition = "none";
        cloned.style.animation = "none";

        // 如果允许点击，需要将点击事件代理到原始元素
        if (currentStep.value?.isAllowClickOnTarget) {
            cloned.addEventListener("click", (e) => {
                e.stopPropagation();
                target.click();
            });
        }

        clonedTargets.push(cloned);
    });

    // 更新位置
    updatePosition();
};

const activeBtn = async () => {
    if (currentStep.value && currentStep.value.onActiveClick) {
        await currentStep.value.onActiveClick();
    }

    if (currentStepIndex.value < totalSteps.value - 1) {
        resetTargetStyle();
        currentStepIndex.value++;
    } else {
        closeGuide(true);
    }
};

onMounted(() => {
    window.addEventListener("resize", updatePosition);
    window.addEventListener("scroll", updatePosition, true);

    // 使用 MutationObserver 监听主题变化
    // 监听 HTML 元素的 class 或 data-theme 属性变化
    themeObserver = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            // 检查是否是 class 或相关属性的变化
            if (
                mutation.type === "attributes" &&
                (mutation.attributeName === "class" ||
                    mutation.attributeName === "data-theme" ||
                    mutation.attributeName === "style")
            ) {
                // 主题变化时刷新克隆元素样式
                if (props.visible && clonedTargets.length) {
                    nextTick(() => {
                        refreshClonedTargetStyles();
                    });
                }
                break;
            }
        }
    });

    // 监听 document.documentElement (html 标签) 的属性变化
    themeObserver.observe(document.documentElement, {
        attributes: true,
        attributeFilter: ["class", "data-theme", "style"],
    });

    // 也监听 body 元素的变化
    themeObserver.observe(document.body, {
        attributes: true,
        attributeFilter: ["class", "data-theme", "style"],
    });
});

onUnmounted(() => {
    window.removeEventListener("resize", updatePosition);
    window.removeEventListener("scroll", updatePosition, true);

    if (resizeObserver) {
        resizeObserver.disconnect();
        resizeObserver = null;
    }

    if (themeObserver) {
        themeObserver.disconnect();
        themeObserver = null;
    }

    if (highlightRetryTimeout) {
        clearTimeout(highlightRetryTimeout);
        highlightRetryTimeout = null;
    }

    if (updatePositionRetryTimeout) {
        clearTimeout(updatePositionRetryTimeout);
        updatePositionRetryTimeout = null;
    }
});

watch(
    () => props.visible,
    (val) => {
        if (val) {
            currentStepIndex.value = props.startStep;
            // 使用 double RAF 确保在布局完全稳定后计算位置
            nextTick(() => {
                requestAnimationFrame(() => {
                    if (currentStep.value) {
                        highlightTargets();
                    }
                    requestAnimationFrame(() => {
                        if (currentStep.value) {
                            updatePosition();

                            const popupDom = document.querySelector(".guide-popup");
                            if (popupDom && !resizeObserver) {
                                resizeObserver = new ResizeObserver(() => {
                                    updatePosition();
                                });
                                resizeObserver.observe(popupDom);
                            }
                        }
                    });
                });
            });
        } else {
            if (resizeObserver) {
                resizeObserver.disconnect();
                resizeObserver = null;
            }
        }
    },
);

watch(
    currentStepIndex,
    (newIndex, oldIndex) => {
        if (props.visible) {
            const oldStep = props.steps[oldIndex];
            if (oldStep) {
                resetTargetStyle();
            }

            // 使用 double RAF 确保在布局完全稳定后计算位置
            nextTick(() => {
                requestAnimationFrame(() => {
                    if (currentStep.value) {
                        highlightTargets();
                    }
                    requestAnimationFrame(() => {
                        if (currentStep.value) {
                            updatePosition();
                        }
                    });
                });
            });
        }
    },
    { immediate: true },
);
</script>

<style lang="scss" scoped>
.guide-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 10000;
    pointer-events: auto;
}
.guide-mask {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: rgba(0, 0, 0, 0); // var(--uosai-history-list-mask-bg);
    z-index: 10000;
}
.guide-popup {
    box-sizing: border-box;
    background-color: var(--uosai-guide-popup-bg);
    border-radius: 10px;
    border: 1px solid rgba(0, 0, 0, 0.1);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.18);
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
    white-space: pre-line; // 支持字符串中的 \n 换行
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
        content: "";
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
    outline: 2px solid var(--activityColor, #0081ff);
    outline-offset: 2px;

    // 禁用所有子元素的交互(除非特别允许)
    * {
        pointer-events: none;
    }
}
</style>
