<template>
    <div class="conversion-mode" v-show="show" @click.stop :style="{...conversionModeStyle, 'width': store.IsSimplifiedChinese ? '164px' : '214px'}">
        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="mcpServerEnable">
            <div class="mode-option" @click="switchMcpStatus()" style="margin-top: 8px;">
                <span class="mode-text">{{ mcpServerEnable }}</span>
            </div>
        </el-tooltip>
        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="store.loadTranslations['Configure MCP Server']">
            <div class="mode-option" @click="mcpSetting()" style="margin-bottom: 8px;">
                <span class="mode-text">{{ store.loadTranslations['Configure MCP Server'] }}</span>
            </div>
        </el-tooltip>
        
        
    </div>
</template>

<script setup>
import { ref, watch, onMounted, computed, nextTick } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import CustomScrollbar from 'custom-vue-scrollbar'
import { Qrequest } from "@/utils";
import { ClickOutside } from 'element-plus';
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()

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

const mcpServerEnable = computed(() => {
    // 等待翻译加载完成
    if (!store.loadTranslations || Object.keys(store.loadTranslations).length === 0) {
        return '' // 翻译未加载完成时返回空字符串
    }
    
    if (store.IsOpenMcpServer && store.IsInstallUOSAiAgent) {
        return store.loadTranslations['Disable MCP Server'] || 'Disable MCP Server'
    } else {
        const translation = store.loadTranslations['Enable MCP Server&']
        return translation ? translation.replace('&', '') : 'Enable MCP Server'
    }
})

const emit = defineEmits(['close', 'clickMcpIcon']);

const mcpSetting = () => {
    Qrequest(chatQWeb.launchMcpConfigWindow)
    emit('close');  // 关闭弹窗
};

// 切换MCP服务状态
const switchMcpStatus = () => {
    emit('clickMcpIcon');  // 点击MCP图标
    emit('close');  // 关闭弹窗
};

// 计算conversion-mode的left位置，使其与target元素对齐
const conversionModeStyle = ref({
    left: '0px'
});

// 更新位置函数
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
        conversionModeStyle.value = {
            left: targetRect.left - 10 + 'px'
        };
    } else {
        console.log("未找到target元素");
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
</script>

<style lang="scss" scoped>
.conversion-mode {
    position: absolute;
    top: 36px;
    // width: 164px;
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
        // justify-content: center;  
        line-height: 1.2;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;
        
        .mode-text {
            font-size: 1rem;
            font-weight: 500;
            margin-left: 32px;
            margin-right: 10px;
            color: var(--uosai-color-conversion-mode-text);
            user-select: none;
            white-space: nowrap;
            text-overflow: ellipsis;
            overflow: hidden;/* 超出部分不显示 */
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