<template>
    <div class="conversion-mode" v-show="show" @click.stop>
        <div class="mode-option" @click="selectMode(store.ConversionMode.Normal)" style="margin-top: 8px;">
            <div class="mode-icon">
                <SvgIcon icon="conversion-normal" />
            </div>
            <span class="mode-text">{{ store.loadTranslations['General Chat'] }}</span>
        </div>
        <div class="mode-option" @click="selectMode(store.ConversionMode.Private)">
            <div class="mode-icon">
                <SvgIcon icon="conversion-private"/>
            </div>
            <span class="mode-text">{{ store.loadTranslations['Private Chat'] }}</span>
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, computed } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import CustomScrollbar from 'custom-vue-scrollbar'
import { Qrequest } from "@/utils";
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()

const store = useGlobalStore()
const props = defineProps({
    show: {
        type: Boolean,
        default: false
    }
});

const emit = defineEmits(['close', 'selectMode']);

const selectMode = (mode) => {
    emit('selectMode', mode);
    emit('close');
};
</script>

<style lang="scss" scoped>
.conversion-mode {
    position: absolute;
    top: 40px;
    left: 0;
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

    .mode-option {
        display: flex;
        align-items: center;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;

        .mode-icon {
            width: 20px;
            height: 20px;
            margin-left: 8px;
            margin-right: 4px;
            margin-top: 2px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 14px;
                height: 13.5px;
                fill: var(--uosai-color-conversion-mode-icon);
            }
        }

        .mode-text {
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
            .mode-text {
                color: #fff;
            }
        }
    }
}

.dark {
    .conversion-mode {
        box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.3);
    }
}
</style> 