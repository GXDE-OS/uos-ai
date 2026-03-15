<template>
    <div v-show="visible" ref="menuContainer" class="menu-container" :class="[positionClass, customClass]" :style="{ left: offsetX !== null ? offsetX + 'px' : '' }">
        <div class="menu-list">
            <div
                v-for="(item, index) in items"
                :key="index"
                class="menu-item"
                :class="{ disabled: item.disabled, checked: item.checked }"
                @click="handleItemClick(item)"
            >
                <SvgIcon v-if="item.icon" class="menu-icon" :icon="item.icon" :color="item.iconColor || 'currentColor'" />
                <span class="menu-text">{{ item.label }}</span>
                <span v-if="item.checked" class="menu-check">
                    <SvgIcon icon="ok-acitve" :color="item.iconColor || 'currentColor'"/>
                </span>
            </div>
        </div>
    </div>
</template>

<script setup>
import { computed, ref } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from './svgIcon/svgIcon.vue'

const store = useGlobalStore()

// 菜单容器 DOM 引用
const menuContainer = ref(null)

// 暴露给父组件
defineExpose({
    menuContainer
})

const props = defineProps({
    // 是否显示菜单
    visible: {
        type: Boolean,
        default: false
    },
    // 菜单项列表
    items: {
        type: Array,
        default: () => []
    },
    // 自定义类名
    customClass: {
        type: String,
        default: ''
    },
    // 菜单位置
    position: {
        type: String,
        default: 'bottom-right' // top-left, top-right, bottom-left, bottom-right
    },
    // 水平偏移量（用于动态调整位置）
    offsetX: {
        type: Number,
        default: null
    }
})

const emit = defineEmits(['select'])

const positionClass = computed(() => {
    return `position-${props.position}`
})

const handleItemClick = (item) => {
    if (item.disabled) return
    emit('select', item)
}
</script>

<style scoped lang="scss">
.menu-container {
    position: absolute;
    z-index: 1000;
    // min-width: 150px;
    background-color: var(--uosai-color-menu-bg);
    border-radius: 18px;
    box-shadow: var(--uosai-color-menu-boxshadow-bg);
    padding: 8px 0;
    overflow: hidden;
    min-width: 164px;

    // 位置样式
    &.position-bottom-right {
        top: calc(100% + 8px);
        right: 0;
    }

    &.position-bottom-left {
        top: calc(100% + 8px);
        left: 0;
    }

    &.position-top-right {
        bottom: calc(100% + 8px);
        right: 0;
    }

    &.position-top-left {
        bottom: calc(100% + 8px);
        left: 0;
    }

    .menu-list {
        display: flex;
        flex-direction: column;
    }

    .menu-item {
        display: flex;
        align-items: center;
        // padding: 8px 16px;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;
        user-select: none;
        font-size: 1rem;
        font-weight: 500;
        color: var(--uosai-color-menu-text-normal);

        &:hover:not(.disabled) {
            background-color: var(--activityColor);
            color: rgba(255, 255, 255, 1);
        }

        &:active:not(.disabled) {
            // color: var(--uosai-color-menu-text-active);
            color: rgba(255, 255, 255, 1);
        }

        &.disabled {
            opacity: 0.4;
            cursor: not-allowed;
        }

        &.checked .menu-check {
            // color: var(--uosai-color-menu-text-hover);
        }

        .menu-icon {
            width: 20px;
            height: 20px;
            margin: 0 10px;
            flex-shrink: 0;
        }

        .menu-text {
            flex: 1;
            white-space: nowrap;
            margin-right: 10px;
        }

        .menu-check {
            margin-right: 10px;
            flex-shrink: 0;
        }
    }
}
</style>
