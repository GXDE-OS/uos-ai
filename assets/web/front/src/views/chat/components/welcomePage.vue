<template>
    <div class="welcome-page">
        <custom-scrollbar class="scrollbar" id="page-scroll" :autoHideDelay="2000" :thumbWidth="6"
            :wrapperStyle="{ width: '100%', height: '100%' }" :style="{ height: '100%' }">
            <div class="top">
                <img draggable="false" :src='currentAssistant.iconPrefix + currentAssistant.icon + "-110.svg"' alt="">
                <div class="welcome-tip">
                    {{ currentAssistant.displayname }}
                </div>
            </div>
            <div class="welcome-content-side" v-show="!isWindowMode && 
                (currentAssistant.type !== store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && isKnowledgeBaseExist && isLLMExist))">
                <div class="tip">
                    {{ currentAssistant.description }}
                </div>
                <div class="shortcut-list-3">
                    <div class="item" :class="{ disabled: recording }" v-for="item in shortcutList.slice(0, 3)"
                        :key="item" @click="!recording && handleItem(item.Question)">
                        <img class="question-icon" :src='item.iconPrefix + item.iconName + ".png"' />
                        {{ item.Question }}
                    </div>
                </div>
            </div>
            <div class="welcome-content" v-show="isWindowMode && 
                (currentAssistant.type !== store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && isKnowledgeBaseExist && isLLMExist))">
                <div class="tip">
                    {{ currentAssistant.description }}
                </div>
                <div class="shortcut-list-6">
                    <div class="item-6" :class="{ disabled: recording }" v-for="item in shortcutList" :key="item"
                        @click="!recording && handleItem(item.Question)">
                        <img class="question-icon" :src='item.iconPrefix + item.iconName + ".png"' />
                        {{ item.Question }}
                    </div>
                </div>
            </div>
            <div class="knowledge-base-config" :class="{'window-mode':isWindowMode}" 
                v-show="currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist || !isLLMExist)">
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
        </custom-scrollbar>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import CustomScrollbar from 'custom-vue-scrollbar';

const { chatQWeb } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps(['question', 'recording', 'historyLength', 'currentAssistant', 'isKnowledgeBaseExist', 'isLLMExist', 'currentAccount'])
const emit = defineEmits(['update:question'])
var shortcutList = ref([])
var currentAccountId = ""
const handleItem = (item) => emit('update:question', props.question + item)
var isWindowMode = false;
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
}

const configureClicked = async () => {
    if (!props.isKnowledgeBaseExist) {
        await Qrequest(chatQWeb.configureKnowledgeBase);
    } else {
        await Qrequest(chatQWeb.launchLLMConfigWindow, true);
    }
}

const sigKnowledgeBaseFAQGenFinished = () => {
    getAiFAQ();
}

const responseAIFunObj = {
    sigWindowModeChanged,
    sigKnowledgeBaseFAQGenFinished
}

function getImageUrl(name) { 
    var path = "icons/" + name + ".png";
    console.log("image path: ", path);
    return path;  
}  
onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }

    getAiFAQ();
    isWindowMode = await Qrequest(chatQWeb.isWindowMode);
    console.log("welcome page on mounted, isWindowMode: ", isWindowMode);
});

onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
});

watch(() => props.historyLength, (newValue) => {
    console.log("history length changed.")
    if (newValue === 0) 
        getAiFAQ()
}, { deep: true });
defineExpose({ getAiFAQ });

watch(() => props.currentAssistant, (newValue) => {
    //console.log("props.currentAssistant changed: ", newValue);

    getAiFAQ();
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
    width: calc(100% - 8px);
    height: 100%;
    max-width: 1000px;
    overflow: hidden;
    // background-color: aquamarine;

    .scrollbar {
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
        width: 100%;

        .top {
            width: 100%;
            height: calc(96px + 32px + 17px + 10px);
            text-align: center;
            margin-top: 35px;
            margin-left: 10px;
            // background-color: azure;

            img {
                width: 96px;
                height: 96px;
            }

            .welcome-tip {
                font-size: 1.42rem;
                font-weight: 600;
                font-style: normal;
                margin-top: 10px;
                color: var(--uosai-color-title);
            }

        }

        .welcome-content-side {
            padding-top: 90px;
            margin-left: 20px;
            // background-color: bisque;

            .tip {
                color: var(--uosai-color-tip-welcome);
                font-size: 0.85rem;
                font-weight: 400;
                font-style: normal;
                margin: 50px auto 15px;
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
                margin: 50px auto 15px;
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
                    width: 260px;
                    min-height: 20px;
                    padding: 20px 15px;
                    margin: 3px 7px 12px;  //top  left-right  bottom
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
            margin-top: 120px;
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
                box-shadow: 0 4px 4px var(--boxShadow);
                font-size: 1rem;
                font-weight: 500;
                color: rgb(255, 255, 255);

                &:hover {  
                    background-color: var(--activityColorHover);
                }
            }
        }
    }
}
</style>
