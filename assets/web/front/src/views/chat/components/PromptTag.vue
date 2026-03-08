<template>
    <pre class="prompt-tag"><span class="prompt-tag-text" :class="{'mcp-tag': isMcpTag}" style="text-align: center;"
        :style="{'line-height': dynamicLineHeight, 'padding-top': isMcpTag ? '2px' : '0', 'padding-bottom': isMcpTag ? '3px' : '0', 'padding-left': isMcpTag ? '5px' : '10px', 'padding-right': isMcpTag ? '5px' : '10px'}">{{ promptTag }}</span></pre>
</template>

<script setup>
import { ref, computed, onMounted } from "vue";
import { useGlobalStore } from "@/store/global";
import _, { compact } from "lodash";
const { chatQWeb, updateActivityColor, updateTheme, updateFont } = useGlobalStore()

// 获取系统缩放比
const deviceScale = ref(1)

const promptTag = computed ( () => {
    return props.promptTag
})

// 计算动态行高
const dynamicLineHeight = computed(() => {
    return deviceScale.value > 1 ? 1.0 : 1.2
})

const props = defineProps({
    isMcpTag: {
        type: Boolean,
        default: false
    },
    promptTag: {
        type: String,
        default: ''
    }
})

const emit = defineEmits([''])

// 获取系统缩放比
const getDeviceScale = () => {
    if (window.devicePixelRatio) {
        deviceScale.value = window.devicePixelRatio
    }
}

onMounted(() => {
    getDeviceScale()
})

defineExpose({})
</script>

<style lang="scss" scoped>
.prompt-tag{
    border-radius: 4px;
    // border: 1px solid rgb(60, 169, 241);
    background-color: var(--activityColorPromptTag);
    font-family: var(--font-family);
    font-size: 1rem;
    font-weight: 500;
    color: var(--activityColor);
    display: flex;
    align-items: center;
    justify-content: center;
    width: fit-content;
    min-height: 20px;
    cursor: pointer;

}

.dark{
    .prompt-tag-text:not(.mcp-tag){
        filter: brightness(1.5);
    }
}
</style>