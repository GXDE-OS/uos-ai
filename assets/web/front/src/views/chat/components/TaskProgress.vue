<template>
    <div class="task-progress" :class="{ 'expanded': isExpanded }" :style="{ height: componentHeight }">
        <div class="task-progress-header" ref="headerRef">
            <div class="task-info">
                <div class="task-name">{{ task.name }}</div>
                <div class="task-status" :class="{ 'active': task.isActive, 'completed': task.isCompleted, 'failed': task.isFailed }">
                    <div class="task-status-icon" >
                        <img :src="getStatusIcon(status)" :class="{ 'rotating': status === store.ToolUseStatus.Calling }"  alt="" />
                    </div>
                </div>
            </div>
            <div class="expand-icon" v-if="needsExpandButton" @click="toggleExpand">
                <SvgIcon v-if="!isExpanded" icon="expand-bubble" />
                <SvgIcon v-else icon="collapse-bubble" />
            </div>
        </div>
        <div class="task-progress-content" ref="contentRef" v-if="parsedDisplayContent.length > 0">
            <template v-for="(item, index) in displayContentInCollapsedState" :key="index">
                <!-- AgentReasoning内容 -->
                <pre v-if="item.chatType === store.ChatAction.AgentReasoning"
                        v-html="item.content"
                        class="agent-reasoning-content"
                        :ref="el => { if (index === displayContentInCollapsedState.length - 1) lastContentItemRef = el }"></pre>

                <!-- AgentAction内容 (工具调用) -->
                <ToolUseItem v-else-if="item.chatType === store.ChatAction.AgentAction"
                                style="margin-left: 0px; width: 100%;"
                                :toolUseItem="item"
                                :errCode="task.errCode || 0"
                                :marginBottom="index === displayContentInCollapsedState.length - 1 ? '0px' : '10px'"
                                :ref="el => { if (index === displayContentInCollapsedState.length - 1) lastContentItemRef = el }"
                                @copyToolUseItem="copyToolUseItem" />
            </template>
        </div>
        <!-- 添加一个占位元素，确保内容与底边有间隙 -->
        <div class="content-spacer" v-if="parsedDisplayContent.length > 0"></div>
    </div>
</template>

<script setup>
import { ref, computed, watch, nextTick, onMounted, onUnmounted } from 'vue';
import ToolUseItem from './ToolUseItem.vue'; // 导入ToolUseItem组件
import SvgIcon from '@/components/svgIcon/svgIcon.vue'; // 导入SvgIcon组件
import { useGlobalStore } from "@/store/global"; // 导入store

const store = useGlobalStore();
const emit = defineEmits(['copyToolUseItem']); // 添加复制事件

const props = defineProps({
    task: {
        type: Object,
        required: true,
        default: () => ({
            name: "当前步骤名称",
            isActive: false,
            isCompleted: false,
            isFailed: false,
            displayContent: "[]", // 修改为displayContent，默认为空数组
            errCode: 0
        })
    }
});

const isExpanded = ref(false);
const contentRef = ref(null);
const lastContentItemRef = ref(null); // 最后一条内容元素的引用
const contentHeight = ref(0);
const lastItemHeight = ref(0); // 最后一项的实际高度（用于任务完成时的收起状态）
const MAX_COLLAPSED_CONTENT_HEIGHT = 146; // 折叠状态下内容区域的最大高度
const headerHeight = ref(40); // header实际高度，初始值
const headerRef = ref(null); // header元素的引用

const status = computed(() => {
   if (props.task.isActive === true)  {
       return store.ToolUseStatus.Calling
   } else if (props.task.isFailed === true) {
       return store.ToolUseStatus.Failed
   } else if (props.task.isCompleted === true) {
       return store.ToolUseStatus.Completed
   }
   return store.ToolUseStatus.Calling
});

const getStatusIcon = (status) => {
    switch (status) {
        case store.ToolUseStatus.Calling:
            return 'icons/mcp-loading.png';
        case store.ToolUseStatus.Completed:
            return 'icons/mcp-completed.png';
        case store.ToolUseStatus.Failed:
            return 'icons/mcp-warning.png';
        default:
            return '';
    }
};

// 解析displayContent，处理不同类型的内容
const parsedDisplayContent = computed(() => {
    if (!props.task.displayContent) return [];

    try {
        // 尝试解析displayContent为JSON数组
        let displayContentArray = [];

        if (typeof props.task.displayContent === 'string') {
            displayContentArray = JSON.parse(props.task.displayContent);
        } else if (Array.isArray(props.task.displayContent)) {
            displayContentArray = props.task.displayContent;
        } else {
            // 如果不是字符串也不是数组，返回空数组
            return [];
        }

        // 确保返回的是数组
        if (!Array.isArray(displayContentArray)) {
            return [];
        }

        // 处理每个项目，确保有必要的字段
        return displayContentArray.map(item => {
            // 为工具调用类型设置默认值
            if (item.chatType === store.ChatAction.AgentAction && !item.status) {
                item.status = store.ToolUseStatus.Calling; // 默认为调用中状态
            }
            return item;
        });
    } catch (e) {
        console.error('Error parsing display content:', e);
        // 如果解析失败，尝试作为纯文本处理
        return [{
            chatType: store.ChatAction.AgentReasoning,
            content: String(props.task.displayContent)
        }];
    }
});

// 计算在收起状态下应该显示的内容
const displayContentInCollapsedState = computed(() => {
    if (isExpanded.value) {
        // 展开状态显示全部内容
        return parsedDisplayContent.value;
    }

    if (props.task.isCompleted) {
        // 任务完成或失败时，收起状态仅显示最后一条content
        if (parsedDisplayContent.value.length === 0) {
            return [];
        }

        const lastItem = parsedDisplayContent.value[parsedDisplayContent.value.length - 1];

        // 如果最后一条是 AgentReasoning 类型，检查是否包含多行文本
        if (lastItem.chatType === store.ChatAction.AgentReasoning && lastItem.content) {
            const lines = lastItem.content.split('\n').filter(line => line.trim() !== '');

            // 如果有多行文本，只显示最后一行
            if (lines.length > 1) {
                return [{
                    ...lastItem,
                    content: lines[lines.length - 1]
                }];
            }
        }

        // 否则直接返回最后一条
        return [lastItem];
    }

    // 任务进行中时，显示全部内容
    return parsedDisplayContent.value;
});

// 折叠状态下的总组件高度
const maxCollapsedHeight = computed(() => {
    return `${MAX_COLLAPSED_CONTENT_HEIGHT + headerHeight.value}px`;
});

// 计算内容是否需要展开/折叠功能
const needsExpandButton = computed(() => {
    if (props.task.isCompleted) {
        // 任务完成后，只有当内容超过1条时才需要展开按钮
        return parsedDisplayContent.value.length > 1;
    }
    // 任务进行中，根据内容高度判断
    return contentHeight.value > MAX_COLLAPSED_CONTENT_HEIGHT;
});

// 计算组件的动态高度
const componentHeight = computed(() => {
    if (isExpanded.value) {
        return 'auto'; // 展开状态自适应
    }

    if (props.task.isCompleted) {
        let spacerHeight = parsedDisplayContent.value.length > 0 ? 10 : 0;
        // 任务完成或失败时，收起状态高度 = header高度 + 最后一项内容的实际高度 + spacer高度
        return `${headerHeight.value + lastItemHeight.value + spacerHeight}px`;
    }

    // 任务进行中时的高度计算
    if (!needsExpandButton.value) {
        // 内容不超过MAX_COLLAPSED_CONTENT_HEIGHT，高度适配内容
        return `${Math.min(contentHeight.value + headerHeight.value, MAX_COLLAPSED_CONTENT_HEIGHT + headerHeight.value)}px`;
    }

    // 内容超过MAX_COLLAPSED_CONTENT_HEIGHT，折叠状态固定为maxCollapsedHeight
    return maxCollapsedHeight.value;
});

// 切换展开/收起状态
const toggleExpand = () => {
    isExpanded.value = !isExpanded.value;
};

// 复制工具调用内容
const copyToolUseItem = (content) => {
    emit('copyToolUseItem', content);
};

// 测量header的实际高度
const measureHeaderHeight = () => {
    if (headerRef.value) {
        headerHeight.value = headerRef.value.offsetHeight;
    }
};

// 计算内容区域的实际高度
const calculateContentHeight = () => {
    if (contentRef.value) {
        nextTick(() => {
            nextTick(() => {
                if (parsedDisplayContent.value.length === 0) {
                    contentHeight.value = 0;
                    lastItemHeight.value = 0;
                    return;
                }
                // 使用scrollHeight获取内容的实际高度
                contentHeight.value = contentRef.value.scrollHeight + 10; // +10 for spacer

                // 计算最后一项的高度（用于任务完成时的收起状态）
                // 只有在收起状态且任务已完成/失败时才更新lastItemHeight
                if (!isExpanded.value && (props.task.isCompleted)) {
                    // 直接使用最后一条内容元素的高度
                    if (lastContentItemRef.value) {
                        // lastContentItemRef.value 可能是 DOM 元素或组件实例
                        const element = lastContentItemRef.value.$el || lastContentItemRef.value;
                        if (element && element.offsetHeight) {
                            lastItemHeight.value = element.offsetHeight;
                        } else {
                            // 如果无法获取元素高度，回退到使用 contentRef 的 scrollHeight
                            lastItemHeight.value = contentRef.value.scrollHeight;
                        }
                    } else {
                        // 如果 lastContentItemRef 不存在，使用 contentRef 的 scrollHeight
                        lastItemHeight.value = contentRef.value.scrollHeight;
                    }
                }
            });
        });
    }
};

// 监听任务状态变化，自动展开显示思考过程
watch(() => props.task.isActive, (newVal) => {
    if (newVal && !isExpanded.value && needsExpandButton.value) {
        isExpanded.value = true;
    }
});

// 添加滚动到最新内容的函数
const scrollToLatestContent = () => {
    if (isExpanded.value === false) {
        // 在收起状态下才自动滚动
        nextTick(() => {
            // 再次使用 nextTick 确保 DOM 完全渲染
            nextTick(() => {
                if (contentRef.value) {
                    contentRef.value.scrollTop = contentRef.value.scrollHeight;
                }
            });
        });
    }
};

// 监听parsedDisplayContent变化，当有新内容时自动滚动和重新计算高度
watch(parsedDisplayContent, (newContent, oldContent) => {
    calculateContentHeight(); // 重新计算高度

    if (!oldContent || newContent.length > oldContent.length) {
        scrollToLatestContent();
    }
}, { deep: true, immediate: true });

// 监听展开/收起状态变化，重新计算高度
watch(isExpanded, (newVal, oldVal) => {
    if (oldVal === true && newVal === false) {
        // 从展开状态切换到收起状态，确保显示最新内容
        scrollToLatestContent();
    }
    // 状态变化时重新计算高度（因为显示的内容可能不同）
    nextTick(() => {
        calculateContentHeight();
    });
});

// 监听任务状态变化，当任务完成时滚动到最新内容并重新计算高度
watch(() => props.task.isCompleted, (newVal) => {
    if (newVal) {
        // 任务完成时，自动收起并重新计算高度
        isExpanded.value = false;
        calculateContentHeight();
        scrollToLatestContent();
    }
});

// 监听任务失败状态，同样处理
watch(() => props.task.isFailed, (newVal) => {
    if (newVal) {
        isExpanded.value = false;
        calculateContentHeight();
        scrollToLatestContent();
    }
});

// 组件挂载时计算初始高度
onMounted(() => {
    // 先测量header高度，确保后续计算使用正确值
    measureHeaderHeight();

    // 在下一个tick测量内容高度，此时header高度已更新
    nextTick(() => {
        calculateContentHeight();
    });

    // 监听header尺寸变化
    if (headerRef.value) {
        const headerResizeObserver = new ResizeObserver(() => {
            measureHeaderHeight();
            nextTick(() => calculateContentHeight());
        });
        headerResizeObserver.observe(headerRef.value);
        onUnmounted(() => headerResizeObserver.disconnect());
    }

    // 监听content宽度变化，当窗口模式切换到侧边栏模式时重新计算高度
    if (contentRef.value) {
        const resizeObserver = new ResizeObserver(() => {
            // 宽度变化时重新计算高度（特别是完成状态下的最后一项高度）
            if (props.task.isCompleted) {
                // 使用 nextTick 确保 DOM 布局完全更新后再计算高度
                nextTick(() => {
                    nextTick(() => {
                        calculateContentHeight();
                    });
                });
            }
        });

        resizeObserver.observe(contentRef.value);

        // 组件卸载时清理observer
        onUnmounted(() => {
            resizeObserver.disconnect();
        });
    }
});

</script>

<style scoped>
.task-progress {
    width: 100%;
    border: 1px solid rgba(0, 0, 0, 0.05);
    box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.08);
    background-color: var(--uosai-color-assistant-bg);
    border-radius: 8px;
    overflow: hidden;
    transition: height 0.3s ease;
    display: flex;
    flex-direction: column;
}

/* 展开状态使用自适应高度 */
.task-progress.expanded {
    height: auto !important; /* 自适应高度，展开状态 */
    min-height: 120px; /* 最小高度 */
    overflow-y: auto; /* 内容超出时显示垂直滚动条 */
}

.task-progress-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px;
    user-select: none;
}

.task-info {
    display: flex;
    align-items: center;
    flex: 1;
}

.task-info .task-name {
    font-weight: 400;
    color: var(--uosai-think-title-color);
    white-space: normal;
    word-break: break-word;
}

.task-info .task-status {
    display: flex;
    align-items: center;
    white-space: nowrap;
    flex-shrink: 0;
}

.task-info .task-status .task-status-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 16px;
    height: 16px;
    margin-left: 4px;
    margin-right: 2px;
    padding-top: 1px;
}

.task-info .task-status .task-status-icon img {
    width: 16px;
    height: 16px;
}

.task-info .task-status .task-status-icon img.rotating {
    animation: spin 0.77s linear infinite;
}

@keyframes spin {
    from {
        transform: rotate(0deg);
    }
    to {
        transform: rotate(360deg);
    }
}

.expand-icon {
    transition: transform 0.2s ease;
    display: flex;
    align-items: center;
    justify-content: center;
    height: 20px;
    width: 20px;
    border-radius: 4px;
    flex-shrink: 0;
    line-height: 1;

    svg {
        padding-left: 2px;
        fill: var(--uosai-color-prompt-icon);
        width: 16px;
        height: 16px;
    }

    &:not(.disabled):hover {
        background-color: var(--uosai-color-svgbtn-bg);
    }

    &:not(.disabled):active {
        background-color: var(--uosai-color-svgbtn-bg);

        svg {
            fill: var(--activityColor);
        }
    }
}

.task-progress-content {
    width: 100%;
    font-size: 13px;
    line-height: 1.5rem;
    color: var(--uosai-think-content-color);
    white-space: pre-wrap;
    word-break: break-word;
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: 0px 10px 0px;
    box-sizing: border-box;
}

/* 新增样式：文本内容样式 */
.agent-reasoning-content {
    width: 100%;
    white-space: pre-wrap; /* 保留空格和换行，但允许自动换行 */
    word-break: break-word; /* 在单词内部换行 */
    overflow-wrap: break-word; /* 确保长文本换行 */
    margin: 0; /* 移除pre标签默认的margin */
}

/* 添加内容与底边的间距 */
.content-spacer {
    height: 10px; /* 设置与底边的间距 */
    width: 100%;
}

/* 滚动条样式 */
.task-progress-content::-webkit-scrollbar {
    width: 6px;
}


.task-progress-content::-webkit-scrollbar-track {
    background: transparent;
}

.task-progress-content::-webkit-scrollbar-thumb {
    background-color: var(--uosai-color-scroll-bg);
    border:1px solid var(--uosai-color-scroll-border);
    box-shadow: var(--globalScrollbar-boxShadow);
    border-radius: 3px;
    width: 6px;
}

.task-progress-content::-webkit-scrollbar-thumb:hover {
    background-color: var(--uosai-color-scroll-bg-hover);
    border:1px solid var(--uosai-color-scroll-border);
    box-shadow: var(--globalScrollbar-boxShadow);
    border-radius: 4px;
    width: 8px !important;
}

.task-progress-content::-webkit-scrollbar-thumb:active {
    background-color: var(--uosai-color-scroll-bg-press);
    border:1px solid var(--uosai-color-scroll-border);
    box-shadow: var(--globalScrollbar-boxShadow);
    border-radius: 4px;
    width: 8px !important;
}

</style>
