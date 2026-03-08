<template>
    <div class="welcome-page" :shortcutListTmp="shortcutListTmp">
        <custom-scrollbar class="scrollbar" id="page-scroll" :autoHideDelay="2000" :thumbWidth="6"
            :wrapperStyle="{ width: '100%', height: '100%' }" :style="{ height: '100%' }">
            <div class="top" :style="{ 'margin-top' : topMargin }">
                <img v-if="!(currentAssistant.iconPrefix === undefined ||  currentAssistant.icon === undefined)"  draggable="false" :src='currentAssistant.iconPrefix + currentAssistant.icon + "-110.svg"' alt="">
                <div class="welcome-tip">
                    {{ currentAssistant.displayname }}
                </div>
            </div>
            <div class="welcome-content-side" v-show="!isWindowMode && showWelcomeContent">
                <div class="tip" :style="{ 'margin-bottom' : faqMargin }">
                    {{ currentAssistant.description }}
                </div>
                <div class="shortcut-list-3" >
                    <div v-for="(item, index) in shortcutList.slice(0, 3)" :key="item" style="width: 100%;display: flex;justify-content: center;">
                        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="item.Question" :disabled="tooltipDisabled3[index]">
                            <div class="item" :class="{ disabled: recording }" 
                            :key="item" @click="!recording && handleItem(item.Question)">
                                <img v-if="!(item.iconPrefix.length === 0 || item.iconName.length === 0)" class="question-icon" :src='item.iconPrefix + item.iconName + ".png"' />
                                <div class="item-text" :ref="el => item3Texts[index] = el">
                                    {{ item.Question }}
                                </div>
                            </div>
                        </el-tooltip>
                    </div>
                </div>
                <UosAiRecommend v-if="currentAssistant.displayname === 'UOS AI'" 
                @update:currentAssistant="updateCurrentAssistant"
                @showAssistantList="showAssistantList"
                :assistantList="props.assistantList" 
                :isWindowMode="isWindowMode"/>
            </div>
            <div class="welcome-content" v-show="isWindowMode && showWelcomeContent">
                <div class="tip" :style="{ 'margin-bottom' : faqMargin }">
                    {{ currentAssistant.description }}
                </div>
                <div class="shortcut-list-6">
                    <div v-for="(item, index) in shortcutList.slice(0, 6)" :key="item"  style="width: 290px;display: flex; flex-wrap: wrap;justify-content: center;margin: 0px 5px 10px;">
                        <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="item.Question" :disabled="tooltipDisabled6[index]">
                            <div class="item-6" :class="{ disabled: recording }" 
                                @click="!recording && handleItem(item.Question)">
                                    <img  v-if="!(item.iconPrefix.length === 0 || item.iconName.length === 0)" class="question-icon" :src='item.iconPrefix + item.iconName + ".png"' />
                                    <div class="item-6-text" :ref="el => item6Texts[index] = el">
                                        {{ item.Question }}
                                    </div>
                            </div>
                        </el-tooltip>
                    </div>
                    
                </div>
                <UosAiRecommend v-if="currentAssistant.displayname === 'UOS AI'" 
                @update:currentAssistant="updateCurrentAssistant"
                @showAssistantList="showAssistantList"
                :assistantList="props.assistantList" 
                :isWindowMode="isWindowMode"/>
            </div>
            <div class="knowledge-base-config" :class="{'window-mode':isWindowMode}" 
                v-show="currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist || !isLLMExist)&& (isEmbeddingPluginsExist)">
                <div class="title">
                    {{ isKnowledgeBaseExist ? store.loadTranslations['Please configure the large model'] : store.loadTranslations['Please configure the knowledge base'] }}
                </div>
                <div class="content">
                    {{ isKnowledgeBaseExist ? store.loadTranslations['The personal knowledge assistant can only be used after configuring a large model.'] : store.loadTranslations['knowledge base configure content'] }}
                </div>
                <div class="config-btn" @click="configureClicked">
                    {{ store.loadTranslations['To configure'] }}
                </div>
            </div>

            <div class="embedding-plugins-config" :class="{'window-mode':isWindowMode}" 
                v-show="(currentAssistant.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT||currentAssistant.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT)&& (!isEmbeddingPluginsExist)">
                <div class="title">
                    {{ isEmbeddingPluginsExist ? store.loadTranslations['Please configure the large model'] : store.loadTranslations['Please install EmbeddingPlugins'] }}
                </div>
                <div class="content">
                    {{ isEmbeddingPluginsExist ? store.loadTranslations['The personal knowledge assistant can only be used after configuring a large model.'] : store.loadTranslations['EmbeddingPlugins install content'] }}
                </div>
                <div class="config-btn" @click="embeddingPluginsconfigureClicked">
                    {{ store.loadTranslations['To install'] }}
                </div>
            </div>


        </custom-scrollbar>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import CustomScrollbar from 'custom-vue-scrollbar';
import UosAiRecommend from './UosAiRecommend.vue';

const { chatQWeb } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps(['question', 'recording', 'historyLength', 'currentAssistant', 'isKnowledgeBaseExist', 'isLLMExist', 'currentAccount', 'isEmbeddingPluginsExist', 'assistantList','shortcutList'])
const emit = defineEmits(['update:question','update:currentAssistant', 'showAssistantList'])
var shortcutList = ref([])

const shortcutListTmp = computed(() => {
    shortcutList.value =  props.shortcutList
})

var currentAccountId = ""
const handleItem = (item) => emit('update:question', props.question + item)
var isWindowMode = true;
const getAiFAQ = async () => {
    const res = await Qrequest(chatQWeb.getAiFAQ)
    if (res.length === 0) {
        console.log("get Ai FAQ empty");
        shortcutList.value = [];
        return;
    }
    shortcutList.value = JSON.parse(res)
}
const sigWindowModeChanged = (res) => {
    // console.log("onWindowModeChanged, is window mode: ", res);
    isWindowMode = res;
    updateMargin()
}

const configureClicked = async () => {
   
     if (!props.isEmbeddingPluginsExist) {
        console.log("installEmbeddingPlugins: ", props.isEmbeddingPluginsExist);
        await Qrequest(chatQWeb.installEmbeddingPlugins);   //先验前后端数据是否联通 沿用知识库的方法 待商店安装接口弄好直接跳转商店安装
      // await Qrequest(chatQWeb.launchLLMConfigWindow, true);
    } else if(!props.isKnowledgeBaseExist){
        // await Qrequest(chatQWeb.launchLLMConfigWindow, false);
        await Qrequest(chatQWeb.launchKnowledgeBaseConfigWindow);
    }

   //!isLLMExist 
   else if(props.currentAccount.displayname === undefined && props.isEmbeddingPluginsExist && props.isKnowledgeBaseExist)
   {
    console.log("isKnowledgeBaseExist,isEmbeddingPluginsExist exist");
    await Qrequest(chatQWeb.launchLLMConfigWindow, false); 
}

}

const embeddingPluginsconfigureClicked = async () => {
    if (!props.isEmbeddingPluginsExist) {
        await Qrequest(chatQWeb.installEmbeddingPlugins);
    } else {
        await Qrequest(chatQWeb.launchLLMConfigWindow, false);
    }  
}

const updateCurrentAssistant = (assistant) => {
    emit('update:currentAssistant', assistant);
}

const showAssistantList = () => {
    emit('showAssistantList');
}

const sigKnowledgeBaseFAQGenFinished = () => {
    getAiFAQ();
}

const sigWebchat2BeHiden = (res) => {
    let scrollArea = document.getElementById('page-scroll')
    scrollArea.scrollTop = 0;
}

const responseAIFunObj = {
    sigWindowModeChanged,
    sigKnowledgeBaseFAQGenFinished,
    sigWebchat2BeHiden
}

function getImageUrl(name) { 
    var path = "icons/" + name + ".png";
    console.log("image path: ", path);
    return path;  
}  

const showWelcomeContent = computed(() => {
    if (props.currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT
        && (!props.isKnowledgeBaseExist || !props.isLLMExist || !props.isEmbeddingPluginsExist)
    ) {
        return false
    }

    if((props.currentAssistant.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||props.currentAssistant.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT) 
        && !props.isEmbeddingPluginsExist ){
        return false
    }

    return true
})

const topMargin = ref("0px")  //顶部距离
const faqMargin = ref("0px")  //随机问题距离
const updateMargin = () => {
    if (isWindowMode){  //窗口模式
        if (props.currentAssistant.type === store.AssistantType.UOS_AI) {
            topMargin.value = "0px"
            faqMargin.value = "18px"
        } else {
            topMargin.value = "40px"
            faqMargin.value = "46px"
        }
    } else {  //侧边栏模式
        topMargin.value = "50px"
        if (props.currentAssistant.type === store.AssistantType.UOS_AI) {
            faqMargin.value = "40px"
        } else {
            faqMargin.value = "115px"
        }
    }
}

const item3Texts = ref([]);
const item6Texts = ref([]);
const tooltipDisabled3 = ref([]);
const tooltipDisabled6 = ref([]);

// 新增：监听系统字号变化的ResizeObserver
const systemFontSizeObserver = ref(null);

// 监听窗口模式变化
watch(() => isWindowMode, () => {
    item3Texts.value = []; // 清空 ref 数组
    item6Texts.value = [];
}, { immediate: true });

// 异步检测文本溢出
const checkTextOverflow = async (element) => {
    return new Promise(resolve => {
        let retryCount = 0;
        const maxRetries = 10; // 最大重试次数

        const check = () => {
            retryCount++;
            if (!element) {
                resolve(false);
                return;
            }
            
            if (element.clientWidth === 0) {
                if (retryCount < maxRetries) {
                    requestAnimationFrame(check);
                } else {
                    resolve(false);
                }
                return;
            }
            
            const isOverflow = element.scrollWidth > element.clientWidth || element.scrollHeight > element.clientHeight;
            resolve(isOverflow);
        };
        check();
    });
};

// 更新tooltip状态
const updateTooltipState = async () => {
    try {
        tooltipDisabled3.value = await Promise.all(
            item3Texts.value.map(async (el, index) => {
                const disabled = !(await checkTextOverflow(el));
                return disabled;
            })
        );
        
        tooltipDisabled6.value = await Promise.all(
            item6Texts.value.map(async (el, index) => {
                const disabled = !(await checkTextOverflow(el));
                return disabled;
            })
        );
    } catch (error) {
        console.error('Error updating tooltip states:', error);
    }
};

// 初始化系统字号变化监听
const initSystemFontSizeObserver = () => {
    // 方法1：监听整个文档的字体变化
  const observer = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
      if (mutation.type === 'attributes' && mutation.attributeName === 'style') {
        console.log('Detected system font size change');
        updateTooltipState();
        break;
      }
    }
  });

  // 监听body元素的style变化（系统字号变化通常会反映在这里）
  observer.observe(document.body, {
    attributes: true,
    attributeFilter: ['style'],
    subtree: false
  });

  // 方法2：额外监听文档根元素（部分系统可能修改html标签）
  const htmlElement = document.documentElement;
  observer.observe(htmlElement, {
    attributes: true,
    attributeFilter: ['style'],
    subtree: false
  });
};

// 监听相关变化
watch([() => shortcutList, () => isWindowMode], () => {
    nextTick(() => {
        updateTooltipState();
    });
}, { immediate: true, deep: true });

onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }

    getAiFAQ();
    isWindowMode = await Qrequest(chatQWeb.isWindowMode);
    console.log("welcome page on mounted, isWindowMode: ", isWindowMode);

    initSystemFontSizeObserver();
});

onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }

    if (systemFontSizeObserver.value) {
        systemFontSizeObserver.value.disconnect();
    }
});

watch(() => props.historyLength, (newValue) => {
    console.log("history length changed.")
    if (newValue === 0) 
        getAiFAQ()
}, { deep: true });
defineExpose({ getAiFAQ });

watch(() => props.currentAssistant, (newValue) => {
    let scrollArea = document.getElementById('page-scroll')
    scrollArea.scrollTop = 0;

    getAiFAQ();

    // 更新间距
    updateMargin()
}, { deep: true });

watch(() => props.currentAccount, (newValue) => {
    console.log("props.currentAccount changed: ", newValue);
    if (currentAccountId === newValue.id)
        return;

    currentAccountId = newValue.id;
    getAiFAQ();
}, { deep: true });

watch(() => props.isKnowledgeBaseExist, (newValue) => {
    if (!props.isKnowledgeBaseExist && props.currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT)
        getAiFAQ();
}, { deep: true });

</script>

<style lang="scss" scoped>
.welcome-page {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 0;
    margin-bottom: 20px;
    width: calc(100% - 4px);
    height: 100%;
    max-width: 1004px;
    overflow: hidden;
    // background-color: aquamarine;

    .scrollbar {
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
        width: 100%;

        .top {
            width: 100%;
            // height: calc(96px + 32px + 17px + 10px);
            text-align: center;
            // margin-top: 20px;
            margin-left: 10px;
            // background-color: azure;

            img {
                width: 96px;
                height: 96px;
                user-select: none;
            }

            .welcome-tip {
                font-size: 1.42rem;
                font-weight: 600;
                font-style: normal;
                // margin-top: 10px;
                color: var(--uosai-color-title);
            }

        }

        .welcome-content-side {
            // padding-top: 30px;
            margin-left: 20px;
            // background-color: bisque;

            .tip {
                color: var(--uosai-color-tip-welcome);
                font-size: 0.85rem;
                font-weight: 400;
                font-style: normal;
                margin: 6px auto 40px;
                // user-select: none;
                width: calc(100% - 50px);
                text-align: center;
            }

            .shortcut-list-3 {
                display: flex;
                flex-direction: column;
                align-items: center;
                // background-color: rgb(190, 154, 140);
                .item {
                    width: calc(100% - 85px);
                    // height: 50px;
                    display: flex;
                    align-items: center;
                    border-radius: 12px;
                    border: 1px solid var(--uosai-color-shortcut-border);
                    box-shadow: 0px 2px 3px var(--uosai-color-shortcut-shadow);
                    background-color: var(--uosai-color-shortcut-bg);
                    color: var(--uosai-color-shortcut);
                    font-size: 0.93rem;
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

                    .item-text {
                        display: -webkit-box;
                        -webkit-line-clamp: 2;
                        -webkit-box-orient: vertical;
                        overflow: hidden;
                        text-overflow: ellipsis;
                        word-break: break-all;
                        white-space: normal;
                        line-height: 1.4;
                        max-height: 2.8em;
                        width: 100%;
                    }
                }
            }
        }

        .welcome-content {
            width: calc(100% - 16px);
            height: 400px;
            margin-left: 18px;

            .tip {
                color: var(--uosai-color-tip-welcome);
                font-size: 0.85rem;
                font-weight: 400;
                font-style: normal;
                margin: 6px auto 10px;
                // user-select: none;
                width: calc(100% - 50px);
                text-align: center;
            }


            .shortcut-list-6 {
                display: flex;
                flex-wrap: wrap;
                justify-content: center;

                .item-6 {
                    // width: calc(100% - 35px);
                    width: 290px;
                    min-height: 20px;
                    padding: 20px 15px;
                    // margin: 3px 5px 10px;  //top  left-right  bottom
                    display: flex;
                    // flex-direction: column;
                    align-items: center;
                    border-radius: 12px;
                    border: 1px solid var(--uosai-color-shortcut-border);
                    box-shadow: 0 2px 3px var(--uosai-color-shortcut-shadow);
                    background-color: var(--uosai-color-shortcut-bg);
                    cursor: pointer;
                    color: var(--uosai-color-shortcut);
                    // user-select: none;

                    &:hover {
                        background-color: var(--uosai-color-shortcut-hover);
                    }

                    &:active {
                        background-color: var(--uosai-color-shortcut-active);
                    }

                    .item-6-text {
                        display: -webkit-box;
                        -webkit-line-clamp: 2;
                        -webkit-box-orient: vertical;
                        overflow: hidden;
                        text-overflow: ellipsis;
                        word-break: break-all;
                        white-space: normal;
                        line-height: 1.4;
                        max-height: 2.8em;
                        width: 100%;
                    }
                }
            }
        }

        .question-icon {
            // width: 24px;
            height: 24px;
            padding-right: 5px;
        }

        .knowledge-base-config {
            width: calc(100% - 60px);
            height: fit-content;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 20px;
            margin-left: 20px;
            margin-top: 70px;
            border-radius: 8px;
            background-color: var(--uosai-color-shortcut-border);
            text-align: center;

            &.window-mode {
                width: calc(100% - 140px);
                margin-left: 60px;
            }

            .title {
                width: 100%;
                font-size: 1rem;
                font-weight: 500;
                color: var(--uosai-color-title);
                margin: 10px;
            }
            .content {
                width: 100%;
                font-size: 1rem;
                font-weight: 400;
                color: var(--uosai-color-title);;
                margin-bottom: 10px;
            }
            .config-btn {
                width: 240px;
                height: 36px;
                border-radius: 8px;
                margin: 10px auto;
                line-height: 36px;
                background-color: var(--activityColor);
                border: 1px solid rgba(0, 0, 0, 0.03);
                box-shadow: 0 2px 4px var(--boxShadow);
                font-size: 1rem;
                font-weight: 500;
                color: rgb(255, 255, 255);
                cursor: default;

                &:hover {  
                    background-color: var(--activityColorHover);
                }
            }
        }


        
        .embedding-plugins-config {
            width: calc(100% - 60px);
            height: fit-content;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 20px;
            margin-left: 20px;
            margin-top: 70px;
            border-radius: 8px;
            background-color: var(--uosai-color-shortcut-border);
            text-align: center;
            margin-bottom: 60px;

            &.window-mode {
                width: calc(100% - 140px);
                margin-left: 60px;
    }

            .title {
                width: 100%;
                font-size: 1rem;
                font-weight: 500;
                color: var(--uosai-color-title);
                margin: 10px;
}
            .content {
                width: 100%;
                font-size: 1rem;
                font-weight: 400;
                color: var(--uosai-color-title);;
                margin-bottom: 10px;
            }
            .config-btn {
                width: 240px;
                height: 36px;
                border-radius: 8px;
                margin: 10px auto;
                line-height: 36px;
                background-color: var(--activityColor);
                border: 1px solid rgba(0, 0, 0, 0.03);
                box-shadow: 0 2px 4px var(--boxShadow);
                font-size: 1rem;
                font-weight: 500;
                color: rgb(255, 255, 255);
                cursor: default;

                &:hover {  
                    background-color: var(--activityColorHover);
                }
            }
        }
    }
}
</style>
