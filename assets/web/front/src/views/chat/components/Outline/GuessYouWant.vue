<template>
    <div v-show="props.show" class="guess-you-want">
        <div class="guess-you-want-title">
            {{ store.loadTranslations["Content generation completed, but you can still provide revision suggestions in AI Assistant"] }}
        </div>
        <div v-for="item in props.GuessYouWantLists" :key="item">
            <div class="guess-you-want-item" @click="handleClick(item)">
                <div class="guess-you-want-item-text">{{ item }}</div>
                <SvgIcon icon="guess-you-want"/>
            </div>
            
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, computed, nextTick, defineProps, onUnmounted } from 'vue'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'

import { Qrequest } from "@/utils";
import { useGlobalStore } from "@/store/global";
const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()

const props = defineProps({
    show: {
        type: Boolean,
        default: false
    },
    GuessYouWantLists: {
        type: Array,
        default: () => []
    }
});
const emit = defineEmits(['guessYouWantClick'])

const handleClick = (item) => {
    emit('guessYouWantClick', item);
};

</script>

<style lang="scss" scoped>
.guess-you-want {
    display: block;

    .guess-you-want-title {
        font-size: 1rem;
        line-height: 1.2;
        font-weight: 500;
        color: var(--uosai-color-guess-you-want-title-text);
        margin-bottom: 10px;
        display: flex;
        align-items: center;
    }

    .guess-you-want-item {
        font-size: 1rem;
        line-height: 1.2;
        font-weight: 400;
        height: 30px;
        color: var(--uosai-color-guess-you-want-content-text);
        background-color: var(--uosai-color-guess-you-want-content-bg-normal);
        border-radius: 8px;
        margin-bottom: 6px;
        cursor: pointer;
        transition: background-color 0.2s ease;
        display: inline-flex;
        align-items: center;
        max-width: calc(100% - 40px);

        .guess-you-want-item-text{
            // 限制文本宽度，超出部分省略号显示
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
            display: block;
            align-items: center;
            line-height: 1.28rem;
            margin-left: 8px;
            flex: 1;
            min-width: 0;
        }

        &:hover {
            background-color: var(--uosai-color-guess-you-want-content-bg-hover);
        }

        &:active {
            background-color: var(--uosai-color-guess-you-want-content-bg-active);
        }

        svg {
            margin-left: 8px;
            margin-right: 8px;
            width: 16px;
            height: 16px;
            fill: var(--uosai-color-guess-you-want-title-text);
        }
    }
}

</style>