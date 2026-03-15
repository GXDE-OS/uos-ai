<template>
    <div class="reference-card"
        :class="{ 'advanced-features': store.IsEnableAdvancedCssFeatures }"
        @mouseenter="$emit('mouseenter')"
        @mouseleave="$emit('mouseleave')">
        <div class="reference-card-header" v-show="(referenceItem.website !== '')">
            <div class="reference-card-header-icon">
                <div class="content-card-icon" v-if="referenceItem.icon === ''" >
                    <SvgIcon icon="reference"/>
                </div>
                <img width="24" height="24" :src="imageUrl" alt="" :imgBase64="imgBase64" v-else/>
            </div>
            <div class="reference-card-header-website"> <span> {{ referenceItem.website !== "" ? referenceItem.website : "" }} </span></div>
        </div>
        <div class="reference-card-title" @click="openUrl" v-show="referenceItem.title != '' || referenceItem.url != ''">
            <div class="reference-card-title-text">
                {{ referenceItem.title != '' ? referenceItem.title : referenceItem.url}}
            </div>
            <div class="url-arrow">
                <SvgIcon icon="url-arrow"/>
            </div>
        </div>
        <div class="reference-card-content" :class="{'large-font': store.FontSize > 16, 'small-font': store.FontSize < 13 }" :referenceItem="referenceItem" v-show="referenceItem.snippet != ''">
            {{ referenceItem.snippet }}
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted, onBeforeUnmount,computed, nextTick } from 'vue'
import { useGlobalStore } from '@/store/global'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import CustomScrollbar from 'custom-vue-scrollbar'
import { Qrequest } from "@/utils";
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()

const store = useGlobalStore()
const emit = defineEmits(['close', 'mouseenter', 'mouseleave']);

const props = defineProps({
    index: {
        type: Number,
        default: 0
    },
    content: {
        type: Array,
        default: []
    },
});

const content = computed(() => {
    return props.content[props.index - 1] ? props.content[props.index - 1].content : ''
})

// 引用默认值
const referenceItemDefault = {
    icon:"",  // 引用图标
    id:"ref_1",
    title:"",  // 引用标题
    url:"",   // 引用的URL
    website:"",  // 引用来源的网站
    snippet:""  // 引用内容的摘要
}
const referenceItem = computed(() => {
    let item = props.content[props.index - 1] ? props.content[props.index - 1].content : ''
    try {
        item = JSON.parse(item)
    } catch (error) {
        item = referenceItemDefault
    }
    return item
})

const imageUrl = ref(''); // 用于存储图片的URL
const imgBase64 = computed(() => {
    setFileIcon(referenceItem.value.icon)
})
const setFileIcon = (imgBase64_) =>{
    const blob = base64ToBlob(imgBase64_);
    imageUrl.value = createObjectURL(blob);
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

const openUrl = async(event) => {
    event.preventDefault()
    // 如果referenceItem.value.url以http开头，直接网页打开打开
    if (referenceItem.value.url.startsWith('http')) {
        await Qrequest(chatQWeb.openUrl, referenceItem.value.url)
        return;
    }
     await Qrequest(chatQWeb.openFile, referenceItem.value.url)
}



// 强制重新计算样式以解决Qt WebEngine渲染问题
onMounted(() => {
    nextTick(() => {
        const card = document.querySelector('.reference-card');
        if (card) {
            card.style.display = 'none';
            card.offsetHeight; // 触发重排
            card.style.display = 'block';
            card.style.boxShadow = '0 0 0 1px var(--uosai-color-reference-card-shadow), 0 6px 20px 0 rgba(0, 0, 0, 0.2)';
        }
    });
});

</script>

<style lang="scss" scoped>
    .reference-card {
        width: 360px;
        max-height: 140px;
        // box-shadow:0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
        background-color: var(--uosai-color-reference-card-bg);
        border-radius: 8px;
        padding: 10px 10px 10px 10px;
        display: block;
        overflow: hidden;
        white-space: normal;
        text-overflow: ellipsis;
        color: var(--uosai-color-reference-card-text);

        &.advanced-features {
            background-color: var(--uosai-color-reference-card-bg-qt6);
            backdrop-filter: blur(30px);
        }

        .reference-card-header {
            display: flex;
            justify-content: flex-start;
            align-items: center;

            .reference-card-header-icon {
                margin-right: 4px;
                display: flex;
                justify-content: flex-start;
                align-items: center;
                width: 24px;
                height: 24px;
                
                .content-card-icon {
                    width: 24px;
                    height: 24px;
                    display: flex;
                    // 让当前div变成圆形
                    border-radius: 50%;
                    background-color: var(--uosai-color-reference-card-icon-bg);
                    justify-content: center;
                    align-items: center;

                    svg {
                        width: 18px;
                        height: 18px;
                    }
                }
                
                img {
                    width: 24px;
                    height: 24px;
                    border-radius: 50%;
                    overflow: hidden;
                }
            }

            .reference-card-header-website {
                color: var(--uosai-color-reference-card-website);
                cursor: pointer;
                display: flex;
                align-items: center;
                height: 20px;
                font-size: 1rem;
                font-weight: 500;
                min-width: 0;
                max-width: 100%;

                span {
                    overflow: hidden;
                    white-space: nowrap;
                    text-overflow: ellipsis;
                    min-width: 0;
                }
            }
        }

        .reference-card-title {
            color: var(--uosai-color-reference-card-title);
            cursor: pointer;
            // 设置display为两端对齐
            display: flex;
            justify-content: space-between;
            align-items: center;
            height: 20px;
            margin-top: 6px;
            margin-bottom: 8px;
            font-size: 1rem;
            font-weight: 500;

            .reference-card-title-text {
                overflow: hidden;
                text-overflow: ellipsis;
                white-space: nowrap; // 超出部分显示省略号
                max-width: 354px;
                line-height: 20px;
            }

            .url-arrow {
                width: 16px;
                height: 16px;
                display: flex;
                justify-content: center;
                align-items: center;
                margin-left: 10px;
                margin-right: 0px;
                svg {
                    width: 16px;
                    height: 16px;
                    fill: var(--uosai-color-flat-btn-icon);
                }
            }
        }

        &:hover {
            .reference-card-title {
                color: var(--activityColor);

                svg {
                    fill: var(--activityColor);
                }
            }
        }

        .reference-card-content {
            // 超出自动换行
            word-break: break-word;
            overflow: hidden;
            max-height: 76px;
            font-size: 0.92rem; 
            // line-height: 1.2;
            color: var(--uosai-color-reference-card-text);
            font-weight:400;
            // 多行文本省略号
            display: -webkit-box;
            -webkit-line-clamp: 4;  // 默认限制显示4行
            -webkit-box-orient: vertical;
            text-overflow: ellipsis;
        }

        // 当字号大于16px时，最多显示3行
        .large-font {
            -webkit-line-clamp: 3;
        }

        .small-font {
            -webkit-line-clamp: 5;
        }
    }
</style>