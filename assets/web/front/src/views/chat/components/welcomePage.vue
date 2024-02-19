<template>
    <div class="welcome-page">
        <div class="top">
            <img draggable="false" style="user-select: none;" src="../../../svg/logo.svg" alt="">
            <div class="welcome-tip">
                {{ store.loadTranslations['Welcome to UOS AI'] }}
            </div>
        </div>
        <div class="welcome-content">
            <div class="tip">
                {{ store.loadTranslations['Here are some of the things UOS AI can help you do'] }}
            </div>
            <div class="shortcut-list">
                <div class="item" :class="{ disabled: recording }" v-for="item in shortcutList"
                    @click="!recording && handleItem(item.Question)">
                    <svgIcon :icon="item.iconName" />
                    {{ item.Question }}
                </div>
            </div>
        </div>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";

const { chatQWeb } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps(['question', 'recording', 'historyLength'])
const emit = defineEmits(['update:question'])
const shortcutList = ref([])
const handleItem = (item) => emit('update:question', props.question + item)
const getAiFAQ = async () => {
    const res = await Qrequest(chatQWeb.getAiFAQ)
    shortcutList.value = JSON.parse(res)
}
onMounted(() => getAiFAQ())
watch(() => props.historyLength, (newValue) => {
    if (newValue === 0) getAiFAQ()
}, { deep: true })
defineExpose({ getAiFAQ })
</script>

<style lang="scss" scoped>
.welcome-page {
    flex: 1 1 0;
    position: relative;

    .top {
        width: 100%;
        text-align: center;
        margin-top: 5px;

        .welcome-tip {
            font-size: 20px;
            font-weight: 600;
            font-style: normal;
            margin-top: 10px;
            user-select: none;
            color: var(--uosai-color-title);
        }

    }

    .welcome-content {
        position: absolute;
        left: 50%;
        transform: translateX(-50%);
        top: 45%;
    }

    .tip {
        color: var(--uosai-color-tip-welcome);
        font-size: 12px;
        font-weight: 400;
        font-style: normal;
        margin: 0 auto 15px;
        user-select: none;
        width: calc(100vw - 96px);
        text-align: center;
    }

    .shortcut-list {
        .item {
            width: calc(100vw - 96px);
            // height: 50px;
            display: flex;
            align-items: center;
            border-radius: 12px;
            border: 1px solid var(--uosai-color-shortcut-border);
            box-shadow: 0px 2px 3px var(--uosai-color-shortcut-shadow);
            background-color: var(--uosai-color-shortcut-bg);
            color: var(--uosai-color-shortcut);
            font-size: 13px;
            font-weight: 500;
            font-style: normal;
            text-align: left;
            margin-bottom: 10px;
            // padding-left: 20px;
            cursor: pointer;
            user-select: none;
            padding: 15px 15px 15px 20px;

            &:hover {
                background-color: var(--uosai-color-shortcut-hover);
            }

            &:active {
                background-color: var(--uosai-color-shortcut-active);
            }

            .svg-icon {
                margin-right: 10px;
                width: 18px;
                height: 18px;
                fill: var(--uosai-color-shortcut);
                flex-shrink: 0;
            }
        }
    }
}
</style>
