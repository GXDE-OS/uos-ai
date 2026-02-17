<template>
    <div class="shortcut-tip">
        <div class="shortcut-item" v-for="item, index in currentShortcutList" :key="item">
            <div class="super-icon" v-if="item === 'Super'">
                <SvgIcon icon="super"/>
            </div>
            <div class="space-text" v-else>{{ item }}</div>
            <span class="plus-sign" v-if="index <currentShortcutList.length - 1">+</span>
        </div>
        <span class="quick-text">{{ store.loadTranslations['Quick Open'] }}</span>
    </div>
</template>

<script setup>
// 组件逻辑可以在这里添加
import { useGlobalStore , } from "@/store/global";
const store = useGlobalStore()

const props = defineProps({
    currentShortcutList: {
        type: Array,
        default: () => []
    }
})

</script>

<style lang="scss" scoped>
.shortcut-tip {
    display: flex;
    align-items: center;
    justify-content: center;
    margin-left: auto;
    margin-right: 0px;
    font-size: 12px;
    font-weight: 500;
    line-height: 1.5;
    color: var(--uosai-color-shortcut-color);
    user-select: none;

   .shortcut-item {
       display: flex;
       align-items: center;
       justify-content: center;
   }


    .super-icon {
        width: 18px;
        height: 18px;
        border-radius: 4px;
        background-color: var(--uosai-color-shortcut-tip-bg);
        display: flex;
        align-items: center;
        justify-content: center;
        svg {
            width: 10px;
            height: 10px;
            fill: var(--uosai-color-shortcut-color);
        }
    }

    .plus-sign {
        width: fit-content;
        white-space: nowrap;
        height: 17px;
        text-align: center;
        margin: 0 4px;
        line-height: 1.2;
    }

    .space-text {
        width: fit-content;
        white-space: nowrap;
        height: 18px;
        padding: 0 3px;
        border-radius: 4px;
        background-color: var(--uosai-color-shortcut-tip-bg);
        display: flex;
        justify-content: center;
        text-align: center;
    }

    .quick-text {
        width: fit-content;
        white-space: nowrap;
        height: 17px;
        text-align: center;
        margin-left: 4px;
    }
}

</style>