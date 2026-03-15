<template>
    <div class="conversion-mode" :class="{ 'advanced-features': store.IsEnableAdvancedCssFeatures }" @click.stop>
        <div class="mode-option" @click="selectMode('docx')" style="margin-top: 8px;">
            <div class="mode-icon">
                <img :src="imageUrl['doc']" alt="">
            </div>
            <span class="mode-text">{{ store.loadTranslations['Save as Word'] }}</span>
        </div>
        <div class="mode-option" @click="selectMode('pdf')">
            <div class="mode-icon">
                <img :src="imageUrl['pdf']" alt="">
            </div>
            <span class="mode-text">{{ store.loadTranslations['Save as PDF'] }}</span>
        </div>
        <div class="mode-option" @click="selectMode('md')">
            <div class="mode-icon">
                <img :src="imageUrl['md']" alt="">
            </div>
            <span class="mode-text">{{ store.loadTranslations['Save as Markdown'] }}</span>
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted, onBeforeUnmount,computed } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import CustomScrollbar from 'custom-vue-scrollbar'
import { Qrequest } from "@/utils";
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()

const store = useGlobalStore()
const emit = defineEmits(['close', 'selectMode']);

const selectMode = (mode) => {
    emit('selectMode', mode);
    emit('close');
};

const downloadListSuffix = ['doc', 'pdf', 'md']
const imageUrl = ref({})


const setFileIcon = (imgBase64_) =>{
    const blob = base64ToBlob(imgBase64_);
    return createObjectURL(blob);
}

// 将Base64字符串转换为Blob
function base64ToBlob(base64) {
    const byteCharacters = atob(base64);
    const byteNumbers = new Array(byteCharacters.length);
    for (let i = 0; i < byteCharacters.length; i++) {
        byteNumbers[i] = byteCharacters.charCodeAt(i);
    }
    const byteArray = new Uint8Array(byteNumbers);
    return new Blob([byteArray], { type: 'image/png' });
}

// 创建Object URL
function createObjectURL(blob) {
  return URL.createObjectURL(blob);
}


const sigIconThemeChanged = async (theme) => {
    // 查询图标base64
    for (let index = 0; index < downloadListSuffix.length; index++) {
        const element = downloadListSuffix[index];
        const fileIcon = await Qrequest(chatQWeb.getDownloadListIcon, element)
        imageUrl.value[element] = setFileIcon(fileIcon)
    }
}

const responseAIFunObj = {
    sigIconThemeChanged
}

onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }

    sigIconThemeChanged()
    
});

onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
});
</script>

<style lang="scss" scoped>
.conversion-mode {
    // position: absolute;  // 移除，因为现在通过 Teleport 渲染到 body 下，使用 fixed 定位
    // top: 40px;  // 移除，位置由父组件动态计算
    // left: 0;  // 移除，位置由父组件动态计算
    // width: 162px;
    height: 118px;
    background-color: var(--uosai-color-conversion-mode-bg);
    border-radius: 18px;
    box-shadow:0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
    z-index: 1000;
    display: flex;
    flex-direction: column;
    user-select: none;
    overflow: hidden;

    &.advanced-features {
        background-color: var(--uosai-color-download-card-bg-qt6);
        backdrop-filter: var(--uosai-color-download-card-filter);
    }

    .mode-option {
        display: flex;
        align-items: center;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;

        .mode-icon {
            width: 16px;
            height: 16px;
            margin-left: 8px;
            margin-right: 4px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 14px;
                height: 13.5px;
                fill: var(--uosai-color-conversion-mode-icon);
            }

            img {
                width: 16px;
                height: 16px;
            }
        }

        .mode-text {
            font-size: 1rem;
            font-weight: 500;
            color: var(--uosai-color-conversion-mode-text);
            user-select: none;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
            margin-right: 18px;
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