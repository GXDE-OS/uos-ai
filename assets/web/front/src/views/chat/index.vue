<template>
    <div class="main-content">
        <ChatTop />
        <WelcomePage ref="welcomePageRef" 
            v-model:question="question" 
            :recording="recording" 
            :historyLength="history.length"
            :currentAssistant="currentAssistant"
            :isKnowledgeBaseExist="isKnowledgeBaseExist"
            :isLLMExist="isLLMExist"
            :currentAccount="currentAccount"
            v-show="history.length === 0" />
        <div class="chat-history" v-show="history.length > 0">
            <custom-scrollbar class="history-scrollbar" id="chatHistory" 
                :autoHideDelay="2000" :thumbWidth="6" 
                :wrapperStyle="{height: '100%'}" :style="{ width: '100%', height: '100%'}" >
                <div class="bubble-div">
                    <ChatBubble v-for="(item, index) in history" :key="index"
                        v-model:playAudioID="playAudioID" 
                        :item="item" :showStop="showStop"
                        :isLast="history.length === index + 1" 
                        :recording="recording" 
                        :netState="netState" 
                        :hasOutput="hasOutput"
                        @handleShowTip="handleShowTip" 
                        @retryRequest="retryRequest" 
                        :currentAssistant="currentAssistant"
                    />
                </div>
            </custom-scrollbar>
        </div>
        <div class="chat-bottom">
            <div class="handle-tip">
                <div class="tip-item" v-show="showTopTip">
                    {{ topTipMsg }}
                </div>
                <div class="tip-item" v-show="showCount" v-if="store.loadTranslations['Stop recording after %1 seconds']">
                    {{ store.loadTranslations['Stop recording after %1 seconds'].replace('%1', countDown) }}
                </div>
                <div class="top-stop tip-item" v-show="showStop"  @click="stopRequest">
                    <svgIcon icon="stop" /> {{ store.loadTranslations['Stop'] }}
                </div>
            </div>
            <div class="top">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="store.loadTranslations['Clear conversation history']">
                    <div class="clear" :class="{ 'disabled': history.length === 0 || disabled || recording }"
                        @click="clearHistory">
                        <SvgIcon icon="clear" />
                    </div>
                </el-tooltip>
                <SwitchModel ref="switchModel" v-model:currentAccount="currentAccount" v-model:accountList="accountList" 
                    v-model:assistantList="assistantList" v-model:currentAssistant="currentAssistant"
                    :disabled="disabled || recording"  @update:currentAssistant="updateCurAssistant"/>
            </div>
            <div class="input-content" :class="{ 'foucs': isFocus}">
                <!-- <el-input style="display: none;"></el-input> -->
                <el-input v-model="question" 
                    ref="questionInput" 
                    resize="none"
                    :autosize="{ minRows: 4, maxRows: 10 }"
                    type="textarea" 
                    :placeholder="store.loadTranslations['Input question']" 
                    @focus="isFocus = true"
                    @blur="isFocus = false" 
                    @keydown.enter.native="handeleEnter" 
                    :disabled="recording || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && !isKnowledgeBaseExist)" />
                <div class="send-btn">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2"
                        :content="netState ? !isAudioInput ? store.loadTranslations['Please connect the microphone and try again'] : store.loadTranslations['Voice input'] : store.loadTranslations['Voice input is temporarily unavailable, please check the network!']">
                        <div class="voice-btn btn"
                            :class="{ recording, disabled: !isAudioInput || showStop || !netState || 
                                (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && !isKnowledgeBaseExist)
                                , notalking: audioLevel === 0 }"
                            @click="handleRecorder">
                            <SvgIcon icon="voice" />
                        </div>
                    </el-tooltip>
                    <div class="send btn" :class="{ 'disabled': question.trim().length === 0 || disabled 
                        || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && !isKnowledgeBaseExist)}"
                        @click="sendQuestion">
                        <SvgIcon icon="send" />
                    </div>
                </div>
            </div>
            <div class="tip">
                {{ store.loadTranslations[`The content generated by AI is for reference only, please pay attention to the accuracy of the information.`] }}
            </div>
        </div>
    </div>
</template>

<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import _ from "lodash";
import SwitchModel from "./components/SwitchModel.vue";
import WelcomePage from "./components/welcomePage.vue";
import ChatBubble from "./components/ChatBubble.vue";
import ChatTop from "./components/ChatTop.vue";
import CustomScrollbar from 'custom-vue-scrollbar';
import 'custom-vue-scrollbar/dist/style.css';
import { ref } from "vue";
import { useRouter } from "vue-router";

const router = useRouter();
const { chatQWeb, updateActivityColor, updateTheme, updateFont } = useGlobalStore()
const store = useGlobalStore()
const instance = getCurrentInstance()
const question = ref('')
const isFocus = ref(false)
const showTopTip = ref(false)
// showStop控制 底部的清除icon、发送按钮、语音图标、模型切换、状态切换按钮、<>置灰，播放按钮置灰，设置不置灰，可正常点击
const showStop = ref(false)
const topTipMsg = ref('')
const history = ref([])
const accountList = ref([])
const assistantList = ref([])
const currentAccount = ref('')
const currentAssistant = ref('')
const playAudioID = ref('')
const isKnowledgeBaseExist = ref(false)
const isLLMExist = ref(false)
const disabled = computed(() => {
    return showStop.value
})
const isAudioInput = ref(false)
const initChat = async () => {
    const _history = await Qrequest(chatQWeb.getAiChatRecords, false)
    const resAccount = await Qrequest(chatQWeb.queryLLMAccountList)
    const resCurAccountID = await Qrequest(chatQWeb.currentLLMAccountId)
    const resAssistant = await Qrequest(chatQWeb.queryAssistantList)
    const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
    isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable)
    hasOutput.value = await Qrequest(chatQWeb.isAudioOutputAvailable)
    netState.value = await Qrequest(chatQWeb.isNetworkAvailable)
    isKnowledgeBaseExist.value = await Qrequest(chatQWeb.isKnowledgeBaseExist)

    history.value = JSON.parse(_history)
    console.log(history.value)

    console.log("assistant list: ", resAssistant);
    console.log("current assistant id: ", resCurAssistantID);
    if (resAssistant) {
        const list = JSON.parse(resAssistant);
        assistantList.value = list;
        list.forEach(element => {
            if (element.id === resCurAssistantID) {
                element.active = true
                currentAssistant.value = element
            }
        });
    }
    if (resAccount) {
        const list = JSON.parse(resAccount)
        isLLMExist.value = list.length > 0;
        list.forEach(element => {
            if (element.id === resCurAccountID) {
                element.active = true
                currentAccount.value = element
            }
        });
        accountList.value = list;
    }
    nextTick(() => handelScrol())
}

const talkID = ref('')
const waitSend = ref(false)
const sendQuestion = async () => {
    if (question.value.trim().length === 0 || disabled.value) return
    // 录音中点击发送按钮
    if (recording.value) {
        waitSend.value = true
        sigAudioASRError()
        return
    }
    await Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
    if (recording.value) sigAudioASRError()
    
    const { id, model, icon, displayname } = currentAccount.value
    const res = await Qrequest(chatQWeb.sendAiRequest, id ? id : '', model ? model : '', 1, question.value, 2)
    if (res.length <= 0)
        return;

    history.value.push({
        role: 'user',
        content: question.value
    })

    history.value.push({
        role: 'assistant',
        anwsers: [
            {
                content: '',
                talkID: res,
                status: 0,
                type: 0,
                llmIcon: icon ? icon : '',
                llmName: displayname ? displayname : ''
            }
        ]
    })
    talkID.value = res
    handelScrol()
    showStop.value = true
    question.value = ''
}
const clearHistory = async () => {
    if (history.value.length === 0 || disabled.value || recording.value) return
    await Qrequest(chatQWeb.clearAiChatRecords)
    await Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
    history.value = []
    handleShowTip(store.loadTranslations['Chat history cleared'])
    // Qrequest(chatQWeb.showToast,1)
}
const handeleEnter = (event) => {

    if (!event.ctrlKey) {
        // sendQuestion()
    } else {
        event.preventDefault();
        event.stopPropagation();
        document.execCommand('insertText', false, '\n')
    }

}
const handleShowTip = (msg) => {
    showTopTip.value = true
    topTipMsg.value = msg
    setTimeout(() => {
        showTopTip.value = false
    }, 3000)
}

const updateCurAssistant = async (assistant) => {
    console.log("current assistant id changed: ", assistant.id);
    const _history = await Qrequest(chatQWeb.getAiChatRecords, false)
    history.value = JSON.parse(_history)
}
// 接受AI文本信息
const sigAiReplyStream = (type, value, status) => {
    console.log("sigAiReplyStream: ", { type, value, status })
    if (status === 0) {
        _.last(_.last(history.value).anwsers).content = _.last(_.last(history.value).anwsers).content + value
    } else if (status === 200) {
        _.last(_.last(history.value).anwsers).status = status
    } else {
        _.last(_.last(history.value).anwsers).content = value
        _.last(history.value).status = status
        _.last(_.last(history.value).anwsers).status = status
    }
    if (status !== 0) {
        const _question = history.value[history.value.length - 2].content
        const { type, content } = _.last(_.last(history.value).anwsers)
        console.log(talkID.value, _question, content, isRetry.value, status, '', type)
        const { icon, displayname } = currentAccount.value
        Qrequest(chatQWeb.logAiChatRecord, talkID.value, _question, content, isRetry.value, status, '', type, icon, displayname)
        showStop.value = false
        isRetry.value = false
    }
    handelScrol()
}

const handelScrol = () => {
    let chatHistory = document.getElementById('chatHistory')
    if (chatHistory.scrollHeight > chatHistory.clientHeight) {
        setTimeout(() => {
            chatHistory.scrollTop = chatHistory.scrollHeight;
        }, 0);
    }
}

const stopRequest = async () => {
    await Qrequest(chatQWeb.cancelAiRequest, talkID.value)
    const _question = history.value[history.value.length - 2].content
    const { type, content } = _.last(_.last(history.value).anwsers)
    const { icon, displayname } = currentAccount.value
    Qrequest(chatQWeb.logAiChatRecord, talkID.value, _question, content, isRetry.value, 0, '', type, icon ? icon : '', displayname ? displayname : '')
    showStop.value = false
    isRetry.value = false
}

// 重新生成
const isRetry = ref(false)
const retryRequest = async () => {
    const _question = history.value[history.value.length - 2].content
    await Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
    if (recording.value) sigAudioASRError()
    const { id, model, icon, displayname } = currentAccount.value
    const res = await Qrequest(chatQWeb.sendAiRequest, id ? id : '', model ? model : '', 1, _question, 2)
    _.last(history.value).anwsers.push({
        content: '',
        status: '',
        talkID: res,
        type: 0,
        llmIcon: icon ? icon : '',
        llmName: displayname ? displayname : ''
    })
    talkID.value = res
    isRetry.value = true
    showStop.value = true
    // question.value = ''
}

const recording = ref(false)
const startQus = ref('')
const endQus = ref('')

const handleRecorder = async () => {
    console.log(recording.value)
    if (!isAudioInput.value || showStop.value || !netState.value) return
    if (recording.value) return sigAudioASRError()
    if (!recording.value) {
        isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable)
        await Qrequest(chatQWeb.stopPlayTextAudio)
        if (isAudioInput.value) {
            await Qrequest(chatQWeb.startRecorder, 0)
            const insert = questionInput.value.textarea.selectionStart;
            startQus.value = questionInput.value.textarea.value.substr(0, insert)
            endQus.value = questionInput.value.textarea.value.substr(insert)
            recording.value = true
            playAudioID.value = ''
        }
    }
}

// AI模型列表更新
const llmAccountLstChanged = (id, list) => {
    isLLMExist.value = id.trim().length > 0;
    console.log("index.vue: llmAccountLstChanged.........  id: ", id);
    console.log("index.vue: llmAccountLstChanged.........  list: ", list);
    if (list) {
        const accounts = JSON.parse(list)
        isLLMExist.value = accounts.length > 0;
        accounts.forEach(element => {
            if (element.id === id) {
                element.active = true
                currentAccount.value = element
            }
        });
        accountList.value = list;
    }
    instance.proxy.$Bus.emit("llmAccountLstChanged", { id, list });
}

const questionInput = ref()
const sigAudioASRStream = (res, isEnd) => {
    question.value = startQus.value + res + endQus.value
    // console.log('sigAudioASRStream', res, isEnd)
    if (isEnd) {
        if (waitSend.value) {
            waitSend.value = false
            sendQuestion()
        }
        sigAudioASRError()
        questionInput.value.focus()
    }
}

const countDown = ref(0)
const showCount = ref(false)
const sigAudioCountDown = (res) => {
    countDown.value = res
    if (res > 0) showCount.value = true
    if (res === 0) {
        setTimeout(() => showCount.value = false, 1000)
    }
}

const sigAudioASRError = () => {
    recording.value = false
    countDown.value = 0
    audioLevel.value = 0
    showCount.value = false
    Qrequest(chatQWeb.stopRecorder)
}
// 语音播放结束
const sigPlayTTSFinished = (res) => {
    console.log('sigPlayTTSFinished', res)
    if (playAudioID.value === res) playAudioID.value = ''
}

const sigPlayTTSError = (res) => playAudioID.value = ''

// const sigAudioRecShortcutPressed = () => handleRecorder()
const sigAudioInputDevChange = (res) => {
    isAudioInput.value = res
    if (!res) recording.value = false
}
const hasOutput = ref(true)
const sigAudioOutputDevChanged = (res) => {
    if (!res) playAudioID.value = ''
    hasOutput.value = res
}

const sigChatConversationType = (id, type) => {
    // console.log(id, action)
    if (_.last(history.value) === undefined)
        return;
    _.last(_.last(history.value).anwsers).type = type
}
// 接受AI图片信息
const sigText2PicFinish = (id, paths) => {
    _.last(_.last(history.value).anwsers).content = JSON.stringify(paths)
    const _question = history.value[history.value.length - 2].content
    const _type = _.last(_.last(history.value).anwsers).type
    console.log(talkID.value, _question, paths, isRetry.value, 0, '', _type)
    const { icon, displayname } = currentAccount.value

    Qrequest(chatQWeb.logAiChatRecord, talkID.value, _question, JSON.stringify(paths), isRetry.value, 0, '', _type, icon ? icon : '', displayname ? displayname : '')
    handelScrol()
    showStop.value = false
    isRetry.value = false
}
const welcomePageRef = ref()
const sigWebchat2BeHiden = () => {
    Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
    welcomePageRef.value.getAiFAQ()
    sigAudioASRError()
    }
// 监听AI进程被杀
// const connectStateChanged = (status) => instance.proxy.$Bus.emit("connectStateChanged", status)
// 活动色改变
const sigActiveColorChanged = (res) => updateActivityColor(res)
const audioLevel = ref(0)
const sigAudioSampleLevel = (res) => audioLevel.value = res
const sigThemeChanged = (res) => updateTheme(res)
const sigFontChanged = (family, pixelSize) => updateFont(family, pixelSize)
const netState = ref(true)
const sigNetStateChanged = (res) => {
    netState.value = res;
    if (recording.value) {
        recording.value = false;
        Qrequest(chatQWeb.stopRecorder)
    }
}
//ctrl+super+c跳转到数字人
// const chatTopRef = ref()
const sigDigitalModeActive = () => {
    if (router.currentRoute.value.name == "DigitalImage") {

    } else {
        Qrequest(chatQWeb.stopPlayTextAudio)
        router.push("/DigitalImage");
    }
};

const sigChatModeActive = () => {
    
}

const handleActive = (res) => {
    let maskDom = document.querySelector('#drop-mask');
    if (!maskDom) {
        maskDom = document.createElement("div");
        maskDom.id = "drop-mask"
        document.body.appendChild(maskDom);
    }
    maskDom.style.height = '100vh'
    maskDom.style.width = '100vw'
    maskDom.style.display = res ? 'flex' : 'none'
}
const switchModel = ref()
const sigWebchatActiveChanged = (res) => {
    if (!res) {
        switchModel.value.showSwitchMenu = false
        // chatTopRef.value.showSeting = false
    }
    // handleActive(res)
}
const sigWebchatModalityChanged = (res) => handleActive(res)

const sigKnowledgeBaseStatusChanged = (status) => {
    console.log("knowledge base status changed: ", status);
    isKnowledgeBaseExist.value = status;
}
// const sigKnowledgeBaseFAQGenFinished = () => {

// }

function handleKeyDown(event) {
    if (event.key === "Enter") {
        event.stopPropagation()
        event.preventDefault()
        console.log('chat')
        sendQuestion()
    }
}

const responseAIFunObj = {
    sigAiReplyStream,
    llmAccountLstChanged,
    sigActiveColorChanged,
    sigAudioASRStream,
    sigAudioASRError,
    sigAudioCountDown,
    sigPlayTTSFinished,
    sigPlayTTSError,
    // sigAudioRecShortcutPressed,
    sigAudioInputDevChange,
    sigAudioOutputDevChanged,
    sigChatConversationType,
    sigText2PicFinish,
    sigThemeChanged,
    sigFontChanged,
    sigWebchat2BeHiden,
    sigAudioSampleLevel,
    sigNetStateChanged,
    sigDigitalModeActive,
    sigChatModeActive,
    sigWebchatActiveChanged,
    sigWebchatModalityChanged,
    sigKnowledgeBaseStatusChanged
    // sigKnowledgeBaseFAQGenFinished
}

onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }
    document.addEventListener("keydown", handleKeyDown);
    initChat()

    useGlobalStore().loadTranslations = await Qrequest(chatQWeb.loadTranslations)
    console.log(useGlobalStore().loadTranslations)

    var fontInfo = await Qrequest(chatQWeb.fontInfo);
    var fontInfoList = fontInfo.split('#');
    document.documentElement.style.fontFamily = fontInfoList[0];
    document.documentElement.style.fontSize = fontInfoList[1] + 'px';
})
onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
    document.removeEventListener("keydown", handleKeyDown);
})
</script>

<style lang="scss" scoped>
.main-content {
    display: flex;
    flex-direction: column; /* 垂直方向顺序布局 */  
    align-items: center; /* 水平方向居中 */  
    justify-content: center; /* 垂直方向居中 */
    height: 100vh;
    width: 100vw;
    overflow: hidden;

    .chat-history {
        width: calc(100% - 8px);
        padding: 15px 0;
        max-width: 1020px;
        flex: 1 1 0;
        overflow: hidden;
        // background-color: aquamarine;

        .history-scrollbar {
            overflow-y: overlay;
            overflow-x: hidden;
            height: 100%;

            .bubble-div {
                width: calc(100% - 12px);
                margin-left: 16px;
            }
        }
    }

    .chat-bottom {
        padding: 0;
        margin-top: auto;
        margin-bottom: 10px;
        position: relative;
        max-width: 1000px;
        width: calc(100% - 20px);

        .handle-tip {
            position: absolute;
            left: 50%;
            transform: translateX(-50%);
            top: -50px;

            .tip-item {
                color: var(--uosai-color-tip);
                font-size: 0.93rem;
                font-weight: 500;
                font-style: normal;
                padding: 6px 15px;
                border-radius: 18px;
                border: 1px solid rgba(0, 0, 0, 0.05);
                box-shadow: 0px 6px 10px rgba(0, 0, 0, 0.05);
                background-color: var(--uosai-color-tip-bg);
                margin: 0 auto;
                text-align: center;
                margin-bottom: 10px;
                width: fit-content;
                user-select: none;
            }

            .top-stop {
                color: var(--activityColor);
                background-color: var(--uosai-color-stop-bg);
                border-radius: 8px;
                display: flex;
                align-items: center;

                cursor: pointer;
                box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.05);
                border: none;

                svg {
                    width: 15px;
                    height: 15px;
                    margin-right: 4px;
                }
            }
        }

        .top {
            position: relative;
            margin-bottom: 10px;
            padding-left: 5px;
            display: flex;
            align-items: center;

            .clear {
                width: 36px;
                height: 36px;
                background-color: var(--uosai-color-clear-bg);
                border-radius: 8px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                margin-right: auto;

                &:not(.disabled):hover {
                    background-color: var(--uosai-color-clear-hover-bg);

                    svg {
                        fill: var(--uosai-color-clear-hover);
                    }
                }

                &:not(.disabled):active {
                    background-color: var(--uosai-color-clear-press-bg);

                    svg {
                        fill: var(--activityColor);
                    }
                }

                svg {
                    height: 20px;
                    width: 20px;
                    fill: var(--uosai-color-clear);
                }
            }
        }

        .input-content {
            min-height: 120px;
            border-radius: 8px;
            opacity: 1;
            background-color: var(--uosai-color-inputcontent-bg);
            margin-bottom: 7px;
            padding-bottom: 8px;
            border: 2px solid rgba(0, 0, 0, 0);

            &.foucs {
                border: 2px solid var(--activityColor);
            }

            :deep(.el-textarea) {
                padding-top: 10px;
                max-height: 185px;

                .el-textarea__inner {
                    box-shadow: none;
                    background: none;
                    color: var(--uosai-color-inputcontent);
                    max-height: 185px;
                    // font-size: 0.93rem;  // 会影响高度

                    &::placeholder {
                        color: var(--uosai-color-inputcontent-placeholder);
                        font-size: 0.93rem;
                        font-weight: 500;
                        user-select: none;
                    }

                    &::-webkit-scrollbar {
                        background: none;
                        width: 6px;
                        height: 6px;

                        &:hover {
                            width: 8px;
                            height: 8px;
                        }
                    }

                    &::-webkit-scrollbar-thumb {
                        border-radius: 4px;
                        background: var(--uosai-color-scroll-bg);
                        border: 1px solid var(--uosai-color-border);
                        // box-shadow: 0px 0px 10px 0px var(--uosai-color-border);
                    }

                    &:focus {
                        outline: 0;
                        box-shadow: none;
                    }
                }
            }

            .send-btn {
                height: 30px;
                display: flex;
                justify-content: flex-end;
                margin-top: 6px;
                // background-color: violet;

                .btn {
                    // box-shadow: 0px 4px 6px rgba(44, 167, 248, 0.4);
                    background-color: var(--activityColor);
                    width: 30px;
                    height: 30px;
                    border-radius: 50%;
                    margin-right: 8px;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    cursor: pointer;

                    &.send {
                        box-shadow: 0 4px 6px 0 var(--boxShadow);

                        svg {
                            fill: #fff;
                        }
                    }

                    svg {
                        width: 12px;
                        height: 12px;
                    }

                    &.disabled {
                        opacity: 0.4;
                        cursor: not-allowed;
                    }

                    &.send:not(.disabled):hover {
                        filter: brightness(1.2);
                    }

                    &.send:not(.disabled):active {
                        filter: brightness(1.1);

                        svg {
                            fill: rgba(255, 255, 255, 0.6);
                        }
                    }

                }

                .voice-btn {
                    background-color: var(--uosai-color-voicebtn-bg);
                    margin-right: 15px;

                    &:not(.disabled):hover {
                        background-color: var(--uosai-color-voicebtn-bg-hover);
                    }

                    &:not(.disabled):active {
                        background-color: var(--uosai-color-voicebtn-bg-active);

                        svg {
                            opacity: 0.6;
                        }
                    }

                    svg {
                        width: 12px;
                        height: 17px;
                        fill: var(--activityColor);
                    }

                    &.recording::before {
                        content: '';
                        position: absolute;
                        opacity: 0.4;
                        border-radius: 50%;
                    }

                    &.recording::after {
                        content: '';
                        position: absolute;
                        opacity: 0.1;
                        border-radius: 50%;

                    }

                    @keyframes Wave1 {
                        0% {
                            border: 2px solid var(--activityColor);
                            width: 30px;
                            height: 30px;
                        }

                        100% {
                            border: 6px solid var(--activityColor);
                            width: 30px;
                            height: 30px;
                        }
                    }

                    @keyframes Wave2 {
                        0% {
                            border: 3px solid var(--activityColor);
                            width: 34px;
                            height: 34px;
                        }

                        100% {
                            border: 5px solid var(--activityColor);
                            width: 40px;
                            height: 40px;

                        }
                    }

                    &:not(.notalking).recording::before {
                        animation: Wave1 0.4s ease-in-out infinite;
                        animation-direction: alternate-reverse;

                    }

                    &:not(.notalking).recording::after {
                        animation: Wave2 0.4s ease-in-out infinite;
                        animation-direction: alternate-reverse;
                    }

                    @keyframes notalking1 {
                        0% {
                            border: 1px solid var(--activityColor);
                            width: 30px;
                            height: 30px;
                        }

                        100% {
                            border: 3px solid var(--activityColor);
                            width: 30px;
                            height: 30px;
                        }
                    }

                    @keyframes notalking2 {
                        0% {
                            border: 3px solid var(--activityColor);
                            width: 32px;
                            height: 32px;
                        }

                        100% {
                            border: 4px solid var(--activityColor);
                            width: 36px;
                            height: 36px;

                        }
                    }

                    &.notalking::before {
                        animation: notalking1 0.8s ease-in-out infinite;
                        animation-direction: alternate-reverse;

                    }

                    &.notalking::after {
                        animation: notalking2 0.8s ease-in-out infinite;
                        animation-direction: alternate-reverse;
                    }
                }
            }
        }

        .tip {
            color: var(--uosai-bottom-tip-color);
            font-size: 12px;
            font-weight: 500;
            font-style: normal;
            text-align: left;
            padding-left: 10px;
            user-select: none;
        }
    }
}

.dark {
    .top-stop {
        box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.15) !important;
    }
}
</style>
