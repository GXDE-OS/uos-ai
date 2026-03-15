<template>
    <div class="local-material-list" :class="{ 'dark-mode': isDarkMode }" :style="listPositionStyle" ref="localMaterialListRef">
        <!-- 标题栏 -->
        <div class="list-header">
            <span class="list-title">{{ store.loadTranslations['Local Materials'] }}</span>
            <div class="close-btn" @click.stop="$emit('close')">
                <SvgIcon icon="close"/>
            </div>
        </div>
        
        <!-- 可滚动的列表区域 -->
        <div class="list-content">
            <div class="material-list" v-if="materialList.length > 0">
                <div v-for="(item, index) in materialList"
                    :key="item.id || index"
                    class="material-item">

                    <!-- 序号 -->
                    <div class="item-index">{{ index + 1 + "." }}</div>
                    
                    <!-- 文件图标 -->
                    <div class="item-icon">
                        <img :src="getFileIcon(item.imgBase64, index)" alt=""/>
                    </div>
                    
                    <!-- 文件名 -->
                    <el-tooltip popper-class="uos-tooltip document-parsing-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="item.fileNameText" placement="top" :disabled="!isTextOverflow(index)">
                        <div class="item-name" ref="fileNameRef">
                            {{ item.fileNameText }}
                        </div>
                    </el-tooltip>
                    <!-- 删除按钮 (hover时显示) -->
                    <div class="delete-btn" @click.stop="handleDeleteItem(item.index)">
                        <SvgIcon icon="trash"/>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, nextTick, watch, onMounted, onBeforeUnmount } from 'vue';
import { useGlobalStore } from '@/store/global'

const store = useGlobalStore()
const props = defineProps({
    materialList: {
        type: Array,
        default: () => []
    },
    isDarkMode: {
        type: Boolean,
        default: false
    },
    targetBtnRef: {
        type: Object,
        default: () => null
    }
})

const refreshPosition = async () => {
    await updatePosition();
};

// 暴露给父组件
defineExpose({
    refreshPosition
});

const emit = defineEmits(['close', 'delete'])

const fileNameRef = ref([]);
const textOverflowFlags = ref([]);
const imageUrls = ref({});
const localMaterialListRef = ref(null);
const listPositionStyle = ref({});
let resizeObserver = null;

// 检查文本是否溢出
const isTextOverflow = (index) => {
    return textOverflowFlags.value[index] || false;
};

const checkTextOverflow = async () => {
    await nextTick();
    textOverflowFlags.value = [];

    if (fileNameRef.value) {
        fileNameRef.value.forEach((element, index) => {
            if (element) {
                textOverflowFlags.value[index] = element.scrollWidth > element.clientWidth;
            }
        });
    }
};

// 添加base64转换函数
const base64ToBlob = (base64) => {
    const byteCharacters = atob(base64);
    const byteNumbers = new Array(byteCharacters.length);
    for (let i = 0; i < byteCharacters.length; i++) {
        byteNumbers[i] = byteCharacters.charCodeAt(i);
    }
    const byteArray = new Uint8Array(byteNumbers);
    return new Blob([byteArray], { type: 'image/png' });
};

const createObjectURL = (blob) => {
    return URL.createObjectURL(blob);
};

// 根据文件名获取对应的图标
const getFileIcon = (imgBase64, index) => {
    // 如果有base64图片数据，优先使用
    if (!imageUrls.value[index]) {
        const blob = base64ToBlob(imgBase64);
        imageUrls.value[index] = createObjectURL(blob);
    }
    return imageUrls.value[index];
};

// 处理删除项目
const handleDeleteItem = (index) => {
    emit('delete', index);
};

const updatePosition = async () => {
    await nextTick();
    let targetElement = null;

    // 检查target是否是DOM元素
    if (props.targetBtnRef && props.targetBtnRef.nodeType && props.targetBtnRef.nodeType === Node.ELEMENT_NODE) {
        targetElement = props.targetBtnRef;
    }
    // 检查target是否是Vue组件实例
    else if (props.targetBtnRef && props.targetBtnRef.$el) {
        targetElement = props.targetBtnRef.$el;
    }

    if (targetElement && localMaterialListRef.value) {
        const targetRect = targetElement.getBoundingClientRect();
        const listRect = localMaterialListRef.value.getBoundingClientRect();
        listPositionStyle.value = {
            top: targetRect.top - listRect.height - 6 + 'px',
            left: targetRect.left + 'px'
        };
    } else {
        console.log("localMaterialList未找到target元素");
    }
};

// 窗口resize处理
const handleWindowResize = () => {
    updatePosition();
};

// 组件卸载时清理URL
onBeforeUnmount(() => {
    // 清理创建的object URLs
    Object.values(imageUrls.value).forEach(url => {
        if (url && url.startsWith('blob:')) {
            URL.revokeObjectURL(url);
        }
    });

    // 移除窗口resize监听
    window.removeEventListener('resize', handleWindowResize);

    // 移除点击外部区域监听（注意：需要与添加时使用相同的参数，包括捕获标志）
    document.removeEventListener('click', handleClickOutside, true);

    // 停止ResizeObserver
    if (resizeObserver) {
        resizeObserver.disconnect();
        resizeObserver = null;
    }
});

// 处理点击外部区域关闭列表
const handleClickOutside = (event) => {
    if (localMaterialListRef.value && !localMaterialListRef.value.contains(event.target)) {
        // 检查是否点击了触发按钮（避免立即关闭再打开的问题）
        let targetElement = null;
        if (props.targetBtnRef && props.targetBtnRef.nodeType && props.targetBtnRef.nodeType === Node.ELEMENT_NODE) {
            targetElement = props.targetBtnRef;
        } else if (props.targetBtnRef && props.targetBtnRef.$el) {
            targetElement = props.targetBtnRef.$el;
        }

        // 如果点击的不是列表区域，也不是触发按钮，则关闭列表
        if (!targetElement || !targetElement.contains(event.target)) {
            emit('close');
        }
    }
};

onMounted(async () => {
    checkTextOverflow();

    // 添加窗口resize监听
    window.addEventListener('resize', handleWindowResize);

    // 添加点击外部区域监听
    // 使用捕获阶段监听，这样即使子元素阻止了事件冒泡也能捕获到
    // 使用 setTimeout 延迟添加监听器，避免组件挂载时的点击事件立即触发关闭
    setTimeout(() => {
        document.addEventListener('click', handleClickOutside, true);
    }, 0);

    // 使用ResizeObserver监听列表尺寸变化
    if (localMaterialListRef.value) {
        resizeObserver = new ResizeObserver(() => {
            updatePosition();
        });
        resizeObserver.observe(localMaterialListRef.value);
    }
});

// 监听target变化
watch(() => props.targetBtnRef, updatePosition, { immediate: true });

// 监听材料列表数量变化
watch(() => props.materialList.length, async () => {
    await nextTick();
    updatePosition();
});
</script>

<style lang="scss" scoped>
.local-material-list {
    position: fixed;
    width: 330px;           /* 固定宽度330px */
    max-height: 350px;     /* 最大高度350px */
    height: auto;           /* 实际高度根据内容自动调整 */
    display: flex;
    flex-direction: column;
    background-color: var(--uosai-color-conversion-mode-bg);
    border-radius: 18px;
    box-shadow:0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
    overflow: hidden;
    z-index: 1000;
    cursor: default;
}

/* 标题栏 */
.list-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0px 0px 0px 18px;
}

.list-title {
    margin: 0;
    font-size: 16px;
    font-weight: 500;
    color: var(--uosai-color-title);
    user-select: none;
}

.close-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    width: 44px;
    height: 44px;

    svg {
        fill: var(--uosai-color-prompt-icon);
        width: 9px;
        height: 9px;
    }

    &:hover {
        background-color: var(--uosai-color-clear-hover-bg);

        svg {
            fill: var(--uosai-color-prompt-icon-hover);
        }
    }
}

/* 列表内容区域 */
.list-content {
    flex: 1;
    overflow: auto;
    display: flex;
    flex-direction: column;
    padding-bottom: 6px;
}

.material-list {
    flex: 1;
    overflow-y: auto;
}

/* 列表项 */
.material-item {
    display: flex;
    align-items: center;
    padding: 5px 8px;
    transition: background-color 0.2s;
    position: relative;
    margin-left: 10px;
    margin-right: 10px;
    border-radius: 8px;
    cursor: default;

    &:not(.disabled):hover {
        background-color: var(--uosai-color-clear-hover-bg);
    }

    &:hover .delete-btn {
        display: flex;
    }
}

.item-index {
    height: 18px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 14px;
    color: var(--uosai-color-title);
    border-radius: 50%;
}

.item-icon{
    display: flex;
    width: 16px;
    height: 16px;
    flex-shrink: 0; /* 防止被flex布局挤压 */
    align-items: center; /* 水平方向居中 */  
    justify-content: center; /* 垂直方向居中 */

    img {
        width: 100%;
        height: 100%;
        object-fit: contain; /* 根据需要选择 none | contain | cover | fill */
    }
}

.item-name {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    color: var(--uosai-color-title);
    font-size: 14px;
    padding-left: 2px;
    user-select: none;
}

.delete-btn {
    display: none;
    z-index: 3;
    justify-content: center;
    align-items: center;
    width: 20px;
    height: 20px;
    cursor: pointer;

    svg {
        fill: var(--uosai-color-prompt-icon);
        width: 16px;
        height: 16px;
    }

    &:not(.disabled):hover {
        border-radius: 4px;
        background-color: var(--uosai-color-svgbtn-bg);
    }

    &:not(.disabled):active {
        border-radius: 4px;
        background-color: var(--uosai-color-svgbtn-bg);

        svg {
            fill: var(--activityColor);
        }
    }
}

/* 深色模式样式 */
.dark-mode .list-header {
  border-bottom-color: var(--uosai-color-border-dark);
}

.dark-mode .material-item:hover {
  background-color: var(--uosai-color-hover-dark);
}

.dark-mode .close-btn:hover {
  background-color: var(--uosai-color-hover-dark);
}

.dark-mode .delete-btn:hover {
  background-color: var(--uosai-color-danger-bg-dark);
}

/* 滚动条样式 */
.material-list::-webkit-scrollbar {
  width: 6px;
}

.material-list::-webkit-scrollbar-track {
  background: transparent;
}

.material-list::-webkit-scrollbar-thumb {
  background-color: var(--uosai-color-scrollbar);
  border-radius: 3px;
}

.material-list::-webkit-scrollbar-thumb:hover {
  background-color: var(--uosai-color-scrollbar-hover);
}
</style>