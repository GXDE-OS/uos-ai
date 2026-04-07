<template>
    <div class="content-card" @click="openMarkdownEditor">
        <div class="content-card-content">
            <div class="content-card-icon">
                <SvgIcon icon="file-icon" />
            </div>
            <div class="content-card-title" :title="props.title" ref="titleElement">
                {{ truncatedTitle }}
            </div>
            <div class="content-card-download" @click="toggleDownloadCard" ref="downloadButton">
                <SvgIcon icon="download-file" />
            </div>
        </div>
        <!-- 使用 Teleport 将 DownloadCard 渲染到 body 下，避免被 .chat-history 的 overflow: hidden 裁剪 -->
        <Teleport to="body">
            <DownloadCard
                v-show="showDownloadCard"
                @close="closeDownloadCard"
                @selectMode="handleSelectMode"
                class="download-card-popup"
                :style="{
                    left: downloadCardPosition.absoluteLeft + 'px',
                    top: downloadCardPosition.absoluteTop + 'px',
                    position: 'fixed'
                }"
            />
        </Teleport>
        <div class="download-status" v-show="downloadStatus.status !== 'init'">
            <div class="download-status-icon">
                <img :src="getStatusIcon(downloadStatus.status)" :class="{ 'rotating': downloadStatus.status === 'downloading' }"  alt="" />
            </div>
            <div class="download-status-text">
                {{ downloadStatus.text }}
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch, computed, defineProps, defineEmits } from 'vue'
import SvgIcon from "@/components/svgIcon/svgIcon.vue"

import { Qrequest } from "@/utils";
import { useGlobalStore } from "@/store/global";
const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()

import DownloadCard from './DownloadCard.vue';
import { update } from 'lodash';

const props = defineProps({
    id: {
        type: String,
        default: ''
    },
    title: {
        type: String,
        default: ''
    },
    content: {
        type: String,
        default: ''
    },
})

const emit = defineEmits(['openMarkdownEditor'])

const showDownloadCard = ref(false);
const downloadButton = ref(null);
const downloadCardPosition = ref({ absoluteLeft: 0, absoluteTop: 0 }); // 绝对定位坐标（相对于视口）
const titleElement = ref(null); // 标题元素引用
const truncatedTitle = ref(props.title); // 截断后的标题

// 创建临时DOM元素来精确测量DownloadCard宽度
const measureDownloadCardWidth = () => {
    const tempElement = document.createElement('div');
    tempElement.style.position = 'absolute';
    tempElement.style.visibility = 'hidden';
    tempElement.style.whiteSpace = 'nowrap';
    tempElement.style.pointerEvents = 'none';
    
    // 复制DownloadCard的实际样式
    const computedStyle = window.getComputedStyle(document.documentElement);
    const fontSize = parseFloat(computedStyle.fontSize);
    
    tempElement.style.fontSize = fontSize + 'px';
    tempElement.style.fontWeight = '500';
    tempElement.style.paddingLeft = '8px';
    tempElement.style.paddingRight = '18px';
    tempElement.style.lineHeight = '34px';
    
    // 获取当前语言环境
    const store = useGlobalStore();
    const translations = store.loadTranslations;
    
    // 获取最长的文本
    const texts = [
        translations['Save as Word'] || 'Save as Word',
        translations['Save as PDF'] || 'Save as PDF',
        translations['Save as Markdown'] || 'Save as Markdown'
    ];
    
    const longestText = texts.reduce((longest, current) => 
        current.length > longest.length ? current : longest, ''
    );
    
    // 创建完整的HTML结构来测量
    tempElement.innerHTML = `
        <div style="display: flex; align-items: center; height: 34px;">
            <svg style="width: 16px; height: 16px; margin-right: 4px;" viewBox="0 0 16 16">
                <path d="M8 2L8 8M8 8L11 5M8 8L5 5" stroke="currentColor" stroke-width="1.5" fill="none"/>
            </svg>
            <span>${longestText}</span>
        </div>
    `;
    
    document.body.appendChild(tempElement);
    const width = tempElement.offsetWidth + 4; // 添加一些边距
    document.body.removeChild(tempElement);
    
    return width;
};

// 计算并设置下载卡片位置（使用绝对定位，相对于视口）
const calculateDownloadCardPosition = () => {
    if (downloadButton.value) {
        const buttonRect = downloadButton.value.getBoundingClientRect();
        
        // 使用实时测量的宽度
        const downloadCardWidth = measureDownloadCardWidth();
        
        // 计算绝对定位坐标（相对于视口）
        let absoluteLeft = buttonRect.left;
        let absoluteTop = buttonRect.top + buttonRect.height;
        
        // 边界检测：确保DownloadCard不会超出视口右边界
        const viewportWidth = window.innerWidth;
        const downloadCardRight = absoluteLeft + downloadCardWidth;
        
        // 如果DownloadCard会超出视口右边界，则向左调整位置
        if (downloadCardRight > viewportWidth - 10) { // 留10px间隔
            const overflow = downloadCardRight - (viewportWidth - 10);
            absoluteLeft = absoluteLeft - overflow;
        }
        
        downloadCardPosition.value.absoluteLeft = absoluteLeft;
        downloadCardPosition.value.absoluteTop = absoluteTop;
    }
};

const toggleDownloadCard = (event) => {
    // 点击事件过滤
    event.preventDefault(); // 阻止默认事件
    event.stopPropagation();
    if (store.IsEnableMcp) {
        showDownloadCard.value = !showDownloadCard.value;
        if (showDownloadCard.value) {
            nextTick(() => {
                calculateDownloadCardPosition(); // 显示时重新计算位置，确保使用最新的字体大小
            });
        }
    } else {
        // 直接下载markdown文件
        handleSelectMode('md');
    }
    
};

const mdEditorContent = ref({})
const openMarkdownEditor = () => {
    mdEditorContent.value = {
        id: props.id,
        title: props.title,
        content: props.content,
    }
    emit('openMarkdownEditor', mdEditorContent.value);
};

const closeDownloadCard = () => {
    showDownloadCard.value = false;
};

const handleSelectMode = async (mode) => {
    // 这里可以处理选择的下载模式
    const ret = await Qrequest(chatQWeb.downloadFile, props.id, props.title, props.content, mode);
    if (ret) {
        updateDownloadStatus(store.loadTranslations['Saving...'], 'downloading');
    }
};

// 点击外部关闭下载卡片
const handleClickOutside = (event) => {
    if (showDownloadCard.value &&
        !event.target.closest('.download-card-popup') &&
        !event.target.closest('.content-card-download')) {
        closeDownloadCard();
    }
};

// 滚轮滚动关闭下载卡片
const handleWheelScroll = () => {
    if (showDownloadCard.value) {
        closeDownloadCard();
    }
};

const downloadStatus = ref({
    text: '',
    status: 'init'  // init: 初始状态-不显示下载状态， downloading: 下载中状态， success: 下载成功状态， error: 下载失败状态
});

const getStatusIcon = (status) => {
    switch (status) {
        case 'downloading':
            return 'icons/mcp-loading.png';
        case 'success':
            return 'icons/mcp-completed.png';
        case 'error':
            return 'icons/mcp-warning.png';
        default:
            return '';
    }
};

const updateDownloadStatus = (text, status) => {
    // 下载卡片显示时，开始下载
    downloadStatus.value.text = text;
    downloadStatus.value.status = status;
}

// 计算标题截断
const calculateTitleTruncation = () => {
    if (!titleElement.value || !props.title) return;
    
    const maxWidth = 214; // 最大宽度（像素）
    const element = titleElement.value;
    const originalText = props.title;
    
    // 创建临时元素来测量文本宽度
    const canvas = document.createElement('canvas');
    const context = canvas.getContext('2d');
    
    // 获取当前字体样式
    const computedStyle = window.getComputedStyle(element);
    const fontSize = computedStyle.fontSize;
    const fontFamily = computedStyle.fontFamily;
    const fontWeight = computedStyle.fontWeight;
    
    context.font = `${fontWeight} ${fontSize} ${fontFamily}`;
    
    // 如果原文本宽度小于最大宽度，直接显示原文本
    const originalWidth = context.measureText(originalText).width;
    if (originalWidth <= maxWidth) {
        truncatedTitle.value = originalText;
        return;
    }
    
    // 需要截断，保留最后3个字符
    const suffix = originalText.slice(-3);
    const ellipsis = '⋯';
    
    // 计算可用宽度（减去省略号和后缀的宽度）
    const suffixWidth = context.measureText(suffix).width;
    const ellipsisWidth = context.measureText(ellipsis).width;
    const availableWidth = maxWidth - suffixWidth - ellipsisWidth;
    
    // 找到合适的截断位置
    let prefix = '';
    for (let i = 0; i < originalText.length - 3; i++) {
        const testPrefix = originalText.slice(0, i + 1);
        const testWidth = context.measureText(testPrefix).width;
        if (testWidth > availableWidth) {
            prefix = originalText.slice(0, i);
            break;
        }
        prefix = testPrefix;
    }
    
    truncatedTitle.value = prefix + ellipsis + suffix;
};

const sigDownloadFileFinished = (id, res) => {
    if (id === props.id) {
        if (res) {
            updateDownloadStatus(store.loadTranslations['Saved successfully!'], 'success');
        } else {
            updateDownloadStatus(store.loadTranslations['Save failed, please try again!'], 'error');
        }
    }
}

const responseAIFunObj = {
    sigDownloadFileFinished,
}

onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }

    updateDownloadStatus('', 'init');
    document.addEventListener('click', handleClickOutside);
    
    // 监听滚轮滚动事件，关闭下载卡片
    document.addEventListener('wheel', handleWheelScroll, { passive: true });
    
    // 初始化标题截断
    nextTick(() => {
        calculateTitleTruncation();
    });
    
    // 初始化时截断标题
    calculateTitleTruncation();
    
    // 监听窗口大小变化
    window.addEventListener('resize', handleResize);
    
    // 监听内容变化 - 只监听必要的元素变化，避免死循环
    const contentCardObserver = new MutationObserver(() => {
        nextTick(() => {
            calculateTitleTruncation();
            // 如果下载卡片显示，重新计算位置，确保使用最新的字体大小
            if (showDownloadCard.value) {
                calculateDownloadCardPosition();
            }
        });
    });
    
    // 只监听当前组件相关的元素，避免全局监听导致的死循环
    const contentCardElement = document.querySelector('.content-card');
    if (contentCardElement) {
        contentCardObserver.observe(contentCardElement, {
            childList: true,
            subtree: true,
            characterData: true,
            attributes: true,
            attributeFilter: ['style', 'class']
        });
    }
    
    // 保存观察者引用，便于清理
    window._contentCardObservers = window._contentCardObservers || [];
    window._contentCardObservers.push({ contentCardObserver });
});

// 监听标题变化
watch(() => props.title, () => {
    nextTick(() => {
        calculateTitleTruncation();
    });
}, { immediate: true });

watch(() => store.FontSize, (newFontSize) => {
    nextTick(() => {
        calculateDownloadCardPosition(); // 如果下载卡片显示，重新计算位置，确保使用最新的字体大小
    });
}, { immediate: true });

// 监听窗口大小变化，重新计算截断和下载卡片位置
const handleResize = () => {
    nextTick(() => {
        calculateTitleTruncation();
        if (showDownloadCard.value) {
            calculateDownloadCardPosition(); // 如果下载卡片显示，重新计算位置，确保使用最新的字体大小
        }
    });
};

onUnmounted(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
    window.removeEventListener('resize', handleResize);
    document.removeEventListener('click', handleClickOutside);
    document.removeEventListener('wheel', handleWheelScroll);
    
    // 清理观察者
    if (window._contentCardObservers) {
        window._contentCardObservers.forEach(({ contentCardObserver }) => {
            contentCardObserver?.disconnect();
        });
        delete window._contentCardObservers;
    }
});
</script>



<style scoped lang="scss">
.content-card {
     display: flex;          /* 关键：把容器变成 Flex 容器 */
    flex-direction: column; /* 子元素纵向排列 */
    align-items: center;
    justify-content: center;
    position: relative; /* 为DownloadCard提供定位上下文 */

    width:320px;
    height:96px;
    border:1px solid var(--uosai-color-content-card-border);
    border-radius:8px;
    background: var(--uosai-color-content-card-bg); /* 半透明渐变蒙版 */
    margin-top: 10px;

    .content-card-content {
        display: flex;
        align-items: center;
        justify-content: center;
        width: 100%;

        .content-card-icon {
            width: 24px;
            height: 24px;
            margin-left: 20px;
            margin-right: 6px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 14.4px;
                height: 16.8px;
                fill: var(--uosai-color-flat-btn-icon);
            }
        }

        .content-card-title {
            color:var(--uosai-color-content-card-text);
            font-size:1.14rem;
            font-weight:500;
            margin-right: auto;
            max-width: 214px;
            display: flex;
            overflow: hidden;
            white-space: nowrap;
            line-height: 1.2;
        }

        .content-card-download {
            display: flex;
            align-items: center;
            justify-content: center;
            width: 24px;
            height: 24px;
            margin-right: 18px;
            border-radius: 6px;
            background-color: var(--uosai-color-flat-btn-bg-hover);
            svg {
                width: 16px;
                height: 16px;
                fill: var(--uosai-color-flat-btn-icon);
            }

            &:not(.disabled):hover {
                background-color: var(--uosai-color-flat-btn-bg-press);

                svg {
                    fill: var(--uosai-color-flat-btn-icon);
                }
            }

            &:not(.disabled):active {
                background-color: var(--uosai-color-flat-btn-bg-press);

                svg {
                    fill: var(--activityColor);
                }
            }
        }
    }

    .download-status {
        display: flex;
        align-items: flex-start;
        justify-content: center;
        width: 100%;
        // height: 100%;
         margin-top: 7px;

        .download-status-icon {
            width: 16px;
            height: 16px;
            margin-left: 24px;
            margin-right: 6px;
            display: flex;
            align-items: center;
            justify-content: center;

            img {
                width: 16px;
                height: 16px;

                &.rotating {
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
            }
        }

        .download-status-text {
            margin-left: 4px;
            margin-right: auto;
            height: 17px;
            font-size: 0.85rem;
            font-weight: 400;
            line-height: 1.2;
            color: var(--uosai-color-content-card-download-status-text);
            
        }

        .download-status-close {
            width: 16px;
            height: 16px;
            margin-right: 18px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 12px;
                height: 12px;
                fill: var(--uosai-color-content-card-download-status-close);
            }

            &:hover {
                svg {
                    fill: var(--uosai-color-content-card-download-status-close-hover);
                }
            }
        }
    }
}

</style>