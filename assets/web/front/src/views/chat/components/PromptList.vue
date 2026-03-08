<template>
    <div :class="{'enable-backdrop-filter': enableBackdropFilter }" class="propmpt-list,"
        v-show="showPropmptList">
        <div class="top">
            <div style="margin-left: 12px;align-items: center;display: flex;">
                <div  class="prompt-icon">
                    <SvgIcon icon="prompt"/>
                </div>
                <span style="margin-left: 8px" class="instruction">{{ store.loadTranslations['Instruction'] }}</span>
            </div>
            <div style="margin-right: 7px;align-items: center;display: flex;">
                <span style="margin-right: 13px; font-size: 12px; font-weight: 400;" class="prompt_tag">{{ store.loadTranslations['Type \"/\" in the input box to activate.'] }}</span>
                <div class="icon"  style="width: 16px; height: 16px; cursor: pointer;display: flex;justify-content: center;" @click="closePromptList">
                    <SvgIcon icon="close"/>
                </div>
            </div>
        </div>
        <custom-scrollbar class="promptListScrollbar" id="promptListScroll" :autoHideDelay="2000" :thumbWidth="6" 
            :wrapperStyle="{ width: '100%', height: '100%' ,padding: '0px' }" :style="{ height: '100%' }">
            <ul class="propmpt-list-ul" v-if="filteredItems.length"
                :promptLists="promptLists">
                <li
                    class="propmpt-list-ul-li"
                    v-for="(item, index) in filteredItems"
                    :key="item"
                    :class="{ active: highlightedIndex === index }"
                    @mouseup="cilckPropmt"
                    @mouseenter="activeItem(item, index)">
                    <span v-html="highlightMatch(item)" style="padding-left:10px"></span>
                </li>
            </ul>
    </custom-scrollbar>
    </div>
</template>

<script setup>
import { ref, watchEffect } from "vue";
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import _, { compact, forEach } from "lodash";
import { useRouter } from "vue-router";
import SvgIcon from "@/components/svgIcon/svgIcon.vue";
import CustomScrollbar from 'custom-vue-scrollbar';
const { chatQWeb, updateActivityColor, updateTheme, updateFont } = useGlobalStore()
const store = useGlobalStore()
const highlightedIndex = ref(0);
const currentIndex = ref()
const currentItem = ref()
// const promptLists = ref(['系统控制', '打开应用', '发送邮件', '创建日程', '系统调用']);
// 判断是否启用高级CSS特性（如backdrop-filter等）
const enableBackdropFilter = computed(() => store.IsEnableAdvancedCssFeatures)
const promptLists = computed(() => {
    let promptLists_ = []
    props.promptInfos.forEach(element => {
        promptLists_.push(element.tagName)
    })

    return promptLists_
});

const filteredItems = computed(() => {
    const filter = promptLists.value.filter(item =>item.toLowerCase().includes(props.searchQuery.toLowerCase()))
    if(filter.length == 0){
        return promptLists.value;
    }
    return filter;
});

const showPropmptList = computed ( () => {
    if (props.showPropmptList) {
        let scrollArea = document.getElementById('promptListScroll')
        scrollArea.scrollTop = 0;
    }
    return props.showPropmptList
})

const cilckPropmt = (event) => {
    if (event.button === 0) {  //鼠标左键
        selectPromptLists(currentItem.value, currentIndex.value)
        //选中后列表隐藏
        emit('sigHidePromptLists', false)
    }
}

const selectPromptLists = (value, index) => {
    showPropmptList.value = false;
    highlightedIndex.value = index
    //查找选中的值的对象
    const promptInfo = props.promptInfos.filter(item => item.tagName === value);

    //将选中的值发送到输入框
    emit('sigSelectOnePrompt', promptInfo[0])
};

const activeItem = (item, index) => {
    currentItem.value = item
    currentIndex.value = index
    highlightedIndex.value = index
}

const highlightPrevious = () => {
    if (highlightedIndex.value > 0) {
        highlightedIndex.value--;
        // let scrollArea = document.getElementById('promptListScroll')
        // scrollArea.scrollTop = 0;
        nextTick(() => handelScrol())
    }
};
const highlightNext = () => {
    if (highlightedIndex.value < filteredItems.value.length - 1) {
        highlightedIndex.value++;
        nextTick(() => handelScrol())
    }
};

const itemHeight = 30
const handelScrol = () => {
    let scrollArea = document.getElementById('promptListScroll')
    scrollArea.scrollTop = highlightedIndex.value * itemHeight
}

const handleEnterKeyDown = (event) => {
    selectHighlighted()
};

const selectHighlighted = () => {
    if (highlightedIndex.value >= 0) {
        selectPromptLists(filteredItems.value[highlightedIndex.value], highlightedIndex.value);
    }
};

// 高亮匹配文字的方法
function highlightMatch(text) {
    if (!props.searchQuery) {
        return text;
    }
    const regex = new RegExp(`(${props.searchQuery})`, 'gi');
    const matches = text.match(regex);
    if (matches) {
        const ret = text.replace(regex, '<span style="color: var(--activityColor)">$1</span>')
        return ret;
    }
    emit('sigHidePromptLists', false)  //没有匹配上则关闭列表
    return text;
}

const closePromptList = () => {
    emit('sigHidePromptLists', true)
}

const props = defineProps(['showPropmptList','searchQuery','promptInfos'])
const emit = defineEmits(['sigHidePromptLists','sigSelectOnePrompt','highlightedIndex'])
defineExpose({highlightPrevious, highlightNext, handleEnterKeyDown,highlightedIndex})
</script>

<style lang="scss" scoped>
.propmpt-list{
    border-radius: 8px 8px 0px 0px;
    font-family: var(--font-family);
    color: var(--uosai-color-prompt-item);
    background-color: var(--uosai-color-normal-bg);
    font-size: 1rem;
    font-weight: 500;
    overflow: hidden;
    padding-bottom: 10px;

    // QT6特定样式
    &.enable-backdrop-filter {
        background-color: var(--uosai-color-normal-bg-qt6);
    }

    .top{
        display: flex;
        align-items: center;
        height: 30px;
        margin-left: 6px;
        justify-content: space-between;

        .prompt-icon{
            display: flex;
            align-items: center;
            justify-content: center;
            width: 20px;
            height: 20px;
            // margin-top: 2px;

            svg{
                fill: var(--uosai-color-prompt-icon);
                width: 20px;
                height: 20px;
            }
        }

        .instruction{
            line-height: 1.2;
            color: var(--uosai-color-assistantmenu-name);
        }

        .prompt_tag{
            color: var(--uosai-color-prompt-tag);
        }

        .icon{
            display: flex;
            align-items: center;
            &:not(.disabled):hover {
                svg {
                    fill: var(--uosai-color-prompt-icon-hover);
                }
            }

            svg {
                fill: var(--uosai-color-prompt-icon);
                width: 9px; 
                height: 9px;
            }
        }
    }

    .promptListScrollbar{
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
        
        .propmpt-list-ul{
            list-style: none;
            -webkit-padding-start:0px;
            -webkit-margin-before:0px;
            // -webkit-margin-after: 10px;
            max-height: 140px;
            .propmpt-list-ul-li{
                height: 30px;
                margin-left: 6px;
                margin-right: 6px;
                display: flex;
                align-items: center;
                cursor: pointer;
                // background-color: var(--uosai-color-normalItem-bg);
            }
        }

        .active {
            border-radius: 8px;
            background-color: var(--uosai-color-activeItem-bg);
        }
    }

} 

</style>