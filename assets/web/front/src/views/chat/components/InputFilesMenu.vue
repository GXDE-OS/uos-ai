<template>
    <div class="input-files-menu" v-show="show" :style="inputFilesMenuStyle" @click.stop ref="inputFilesMenuRef">
        <div class="menu-action" @click="handleSelectMaterials" style="margin-top: 8px;">
            <div class="action-icon">
                <SvgIcon icon="local-materials" />
            </div>
            <span class="action-text">{{ store.loadTranslations['Local Materials'] }}</span>
        </div>
        <div class="menu-action" @click="handleSelectFileOutline">
            <div class="action-icon">
                <SvgIcon icon="file-outline"/>
            </div>
            <span class="action-text">{{ store.loadTranslations['File Outline'] }}</span>
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted, computed, nextTick } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'

const store = useGlobalStore()
const props = defineProps({
    show: {
        type: Boolean,
        default: false
    },
    target: {
        type: Object,
        default: () => ({})
    }
});

const emit = defineEmits(['select']);
const inputFilesMenuStyle = ref({});
const inputFilesMenuRef = ref(null);

const handleSelectMaterials = () => {
    emit('select', store.DocFileCategory.LocalMaterial);  // 发送选择事件
};

const handleSelectFileOutline = () => {
    emit('select', store.DocFileCategory.FileOutline);  // 发送选择事件
};

const updatePosition = async () => {
    await nextTick();
    let targetElement = null;
    
    // 检查target是否是DOM元素
    if (props.target && props.target.nodeType && props.target.nodeType === Node.ELEMENT_NODE) {
        targetElement = props.target;
    }
    // 检查target是否是Vue组件实例
    else if (props.target && props.target.$el) {
        targetElement = props.target.$el;
    }
    
    if (targetElement) {
        const targetRect = targetElement.getBoundingClientRect();
        inputFilesMenuStyle.value = {
            left: targetRect.right - inputFilesMenuRef.value.getBoundingClientRect().width + 'px',
            top: targetRect.top - inputFilesMenuRef.value.getBoundingClientRect().height + 'px'
        };
    } else {
        console.log("inputFilesMenu未找到target元素");
    }
};

// 监听target变化
watch(() => props.target, updatePosition, { immediate: true });

// 监听show变化，当显示时重新计算位置
watch(() => props.show, (newVal) => {
    if (newVal) {
        updatePosition();
    }
});

// 监听窗口尺寸变化
onMounted(() => {
    window.addEventListener('resize', updatePosition);
});

onUnmounted(() => {
    window.removeEventListener('resize', updatePosition);
});
</script>

<style lang="scss" scoped>
.input-files-menu {
    position: fixed;
    width: 162px;
    height: 84px;
    background-color: var(--uosai-color-conversion-mode-bg);
    border-radius: 18px;
    box-shadow:0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
    z-index: 1000;
    display: flex;
    flex-direction: column;
    user-select: none;
    overflow: hidden;

    .menu-action {
        display: flex;
        align-items: center;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;

        .action-icon {
            width: 20px;
            height: 20px;
            margin-left: 8px;
            margin-right: 4px;
            margin-top: 1px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 12px;
                height: 14px;
                fill: var(--uosai-color-conversion-mode-icon);
            }
        }

        .action-text {
            font-size: 1rem;
            font-weight: 500;
            color: var(--uosai-color-conversion-mode-text);
            user-select: none;
        }

        &:hover {
            background-color: var(--activityColor);
            svg {
                fill: #fff;
            }
            .action-text {
                color: #fff;
            }
        }
    }
}

.dark {
    .input-files-menu {
        box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.3);
    }
}
</style> 