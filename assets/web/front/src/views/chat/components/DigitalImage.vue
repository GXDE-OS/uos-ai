<template>
    <div
    @dragstart="handleDragStart"
    class="main-content"
    :style="{ '--scale': animateScale, '--scale1': animateScale1,}"
    >
    <div class="" style="position: relative; flex: 1 1 0">
        <div class="logo">
        <img v-if="!(currentAssistant.iconPrefix === undefined ||  currentAssistant.icon === undefined)" :src='currentAssistant.iconPrefix + currentAssistant.icon + "-110.svg"' class="" />
        <div> {{ currentAssistant.displayname }} </div>
        </div>
        <div
        class="animate-box"
        @click="activeFun(true)"
        :class="{ disabledDig: playStatus }"
        >
        <video class="video1" 
            autoplay loop muted
            v-show="videoStatus == 1"
            @play="onPlay == 1 && !ChatWindowState"
            @pause="onPlay !== 1 || ChatWindowState"
            v-if="!ChatWindowState"
        >
            <source src="../../../assets/video/silence.webm" type="video/webm" />
        </video>
        <div class="listen" v-show="videoStatus == 2 || videoStatus == 7">
            <div class="scale">
            <img
                src="../../../assets/images/listen2.png"
                class="listen-img rotate"
            />
            </div>
            <div class="scale1">
            <img
                src="../../../assets/images/listen1.png"
                class="listen-img rotate"
            />
            </div>
        </div>
        <video class="video2"
            autoplay loop muted
            v-show="videoStatus == 2 || videoStatus == 7"
            @play="!ChatWindowState && (onPlay == 2 || onPlay == 7)"
            @pause="(onPlay !== 2 && onPlay !== 7) || ChatWindowState"
            v-if="!ChatWindowState"
        >
            <source src="../../../assets/video/listen.webm" type="video/webm" />
        </video>
        <video class="video3" 
            autoplay loop muted
            v-show="videoStatus == 3"
            @play="onPlay == 3 && !ChatWindowState"
            @pause="onPlay !== 3 || ChatWindowState"
            v-if="!ChatWindowState"
        >
            <source src="../../../assets/video/thinking.webm" type="video/webm" />
        </video>
        <video class="video4"
            autoplay loop muted 
            v-show="videoStatus == 4"
            @play="onPlay == 4 && !ChatWindowState"
            @pause="onPlay !== 4 || ChatWindowState"
            v-if="!ChatWindowState"
        >
            <source src="../../../assets/video/play.webm" type="video/webm" />
        </video>
        <video class="video5"
            autoplay muted
            v-show="
            videoStatus == 5 ||
            videoStatus == 6 ||
            videoStatus == 8 ||
            videoStatus == 9 ||
            videoStatus == 10
            "
            @play="
            !ChatWindowState &&
                (onPlay == 5 ||
                onPlay == 6 ||
                onPlay == 8 ||
                onPlay == 9 ||
                onPlay == 10)
            "
            @pause="
            (onPlay !== 5 &&
                onPlay !== 6 &&
                onPlay !== 8 &&
                onPlay !== 9 &&
                onPlay !== 10) ||
                ChatWindowState
            "
            v-if="!ChatWindowState"
        >
            <source src="../../../assets/video/error.webm" type="video/webm" />
        </video>
        </div>
    </div>
    <div class="active-box">
        <div class="active-con">
        <div>
            <SvgIcon v-show="videoStatus == 2" icon="voice-digital" />
            {{
            Array.isArray(statusLabel) ? statusLabel[0] : statusLabel
            }}<span  class="errDisplayInfo" v-show="showConfigLink">{{ errDisplayInfo }}</span>
            <br v-show="showConfigLink">
            <span class="go-config" v-show="showConfigLink" @click="goConfig">{{ configMsg }}></span>
            <br v-show="showConfigLink && !store.IsGotFreeCredits" >
            <span class="go-config" v-show="showConfigLink && !store.IsGotFreeCredits" @click="goGetFreeCredits">{{ store.loadTranslations['Claim Free Credits'] }}></span>
            <br v-show="showConfigLink && !store.IsGotFreeCredits" >         
            <span
            class="dian"
            v-show="
                videoStatus == 1 ||
                videoStatus == 2 ||
                videoStatus == 3 ||
                videoStatus == 4
            "
            >...</span
            >
            <div class="fz-14" v-if="Array.isArray(statusLabel)">
            {{ statusLabel[1] }}
            </div>
        </div>
        </div>
        <div
        class="handel"
        :class="{ disabledDig: playStatus }"
        @mouseenter="handleMouseOver()"
        @mouseleave="handleMouseOut()"
        @click="activeFun()"
        v-if="
            videoStatus == 2 ||
            videoStatus == 3 ||
            videoStatus == 4 ||
            videoStatus == 7
        "
        >
        <div class="svgBg">
            <SvgIcon icon="dig-stop" />
        </div>
        <div class="svgBg-hover"></div>
        <Tips :tip="tipCon" :class="className" v-show="isTipsVisible"> </Tips>
        </div>
        <div
            class="handel"
            @click="activeFun()"
            @mouseenter="handleMouseOver()"
            @mouseleave="handleMouseOut()"
            :style="{ 'margin-top': showConfigLink && !store.IsGotFreeCredits ? '8.6vh' : '14.6vh' }"
            :class="{ disabledDig: playStatus }"
            v-else
        >
        <div class="svgBg">
            <SvgIcon icon="dig-play" />
        </div>
        <div class="svgBg-hover"></div>
        <Tips :tip="tipCon" :class="className" v-show="isTipsVisible"> </Tips>
        </div>
    </div>
    <!-- 提示信息 -->
    <div class="tip">
        {{ store.loadTranslations[`The content generated by AI is for reference only, please pay attention to the accuracy of the information.`] }}
    </div>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore();
const store = useGlobalStore();
import { Qrequest } from "@/utils";
import _ from "lodash";
import { useRouter } from "vue-router";
import Tips from "../../../components/tips/tips.vue";
import { computed, watch } from "vue";

const showEmbeddingPluginsConfig=ref(false);//点击发送问题成功 触发是否显示插件配置标志
const showKnowledgeBaseConfig=ref(false);//点击发送问题成功 触发是否显示插件配置标志

const instance = getCurrentInstance();
const router = useRouter();
const animateScale = ref(1); //动画缩放比例最底层
const animateScale1 = ref(1); //动画缩放比例第二层
const videoStatus = ref(2); //动画状态1:silence,2:listen,3:think,4:input,5:error
const playStatus = ref(false); //动画，播放按钮点击状态
const question = ref("");
const isAudioInput = ref(false);
const isAudioOutput = ref(false);
const playAudioID = ref("");
const AudioASRStreamState = ref(false); //区分是不是要路由跳转
const ChatWindowState = ref(false); //窗口关闭true,打开false
const onPlay = ref(2);
const tipCon = computed(() => {
    if (
    videoStatus.value == 2 ||
    videoStatus.value == 3 ||
    videoStatus.value == 4 ||
    videoStatus.value == 7
    ) {
    return useGlobalStore().loadTranslations.Stop;
    } else {
    return useGlobalStore().loadTranslations.Activate;
    }
});
const className = ref("top-right"); //tips组件tips位置类
const statusLabel = computed(() => {
    // 根据返回值枚举映射到对应的文字状态
    switch (videoStatus.value) {
    case 1:
        return [
        useGlobalStore().loadTranslations.Sleeping,
        useGlobalStore().loadTranslations[
            "Click on the animation or Ctrl+Super+Space to activate"
        ],
        ];
    case 2:
        return [
        useGlobalStore().loadTranslations.Listening,
        useGlobalStore().loadTranslations[
            "Click the animation or press Enter to send"
        ],
        ];
    case 3:
        return [
        useGlobalStore().loadTranslations.Thinking,
        useGlobalStore().loadTranslations["Click animation to interrupt"],
        ];
    case 4:
        return [
        useGlobalStore().loadTranslations.Answering,
        useGlobalStore().loadTranslations["Click animation to interrupt"],
        ];
    case 5:
        return useGlobalStore().loadTranslations[
        "Unable to connect to the server, please check your network or try again later."
        ]; //播放按钮不可点击直到接收到信号满足激活条件
    case 6:
        return useGlobalStore().loadTranslations["Microphone not detected"]; //播放按钮不可点击直到接收到信号满足激活条件
    case 7:
        return useGlobalStore().loadTranslations[
        "Stop recording after %1 seconds"
        ].replace("%1", countDown.value);
    case 8:
        return ""
        // return errorCon.value; //播放按钮不可点击直到接收到信号满足激活条件比如无模型，账号过期或额度用完
    case 9:
        return useGlobalStore().loadTranslations[
        "Unable to connect to the server, please check your network or try again later."
        ];
        //return errorCon.value; //录音中的异常
    case 10:
        return useGlobalStore().loadTranslations[
        "Sound output device not detected"
        ]; //没有输出设备
    default:
        return [
        useGlobalStore().loadTranslations.Listening,
        useGlobalStore().loadTranslations[
            "Click the animation or press Enter to send"
        ],
        ];
    }
});
const accountList = ref([]);
const currentAccount = ref("");
const currentAssistant = ref('')
const errorCon = ref("");
const showStop = ref(false); //思考回答为true
const isTipsVisible = ref(false); //tips是否显示
let tipsTimerId = null;
const isKnowledgeBaseExist = ref(false)
const isEmbeddingPluginsExist = ref(false)
const props = defineProps(['question', 'recording', 'historyLength', 'currentAssistant', 'isKnowledgeBaseExist', 'isLLMExist', 'currentAccount', 'isEmbeddingPluginsExist'])
/**鼠标移入tips显示 */
const handleMouseOver = () => {
    tipsTimerId && clearTimeout(tipsTimerId);
    tipsTimerId = setTimeout(() => {
    isTipsVisible.value = true;
    }, 2000); // 延迟2秒
};
/**鼠标移出tips隐藏 */
const handleMouseOut = () => {
    tipsTimerId && clearTimeout(tipsTimerId);
    isTipsVisible.value = false;
};
/**初始化 */
const assistantList = ref([])
const functionList = ref([])
const initChat = async () => {
    const resAccount = await Qrequest(chatQWeb.queryLLMAccountList);
    const resCurAccountID = await Qrequest(chatQWeb.currentLLMAccountId);
    const resAssistant = await Qrequest(chatQWeb.queryAssistantList)
    const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
    isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable);
    isAudioOutput.value = await Qrequest(chatQWeb.isAudioOutputAvailable);
    isKnowledgeBaseExist.value = await Qrequest(chatQWeb.isKnowledgeBaseExist)
    isEmbeddingPluginsExist.value = await Qrequest(chatQWeb.isEmbeddingPluginsExist)
    const isNetworkAvailable = await Qrequest(chatQWeb.isNetworkAvailable);

    // 获取当前对话信息
    await getNowConversationInfo(resCurAssistantID)

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    //新版查询历史记录
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)

    //查询当前后端统一背景色
    const backgroundColor = await Qrequest(chatQWeb.getMainContentbackgroundColor);
    sigMainContentBackgroundColor(backgroundColor)

    await Qrequest(chatQWeb.setInputFileSize, 0)  // 同步更新当前输入框中存在的文件

    if (resAssistant) {
        const list = JSON.parse(resAssistant);
        assistantList.value = list;
        for (const element of list) {
            if (element.id === resCurAssistantID) {
                element.active = true
                currentAssistant.value = element
                //查询功能按钮list
                const functionListStr = await Qrequest(chatQWeb.getAssistantFunctions, currentAssistant.value.type)
                if (functionListStr !== "") {
                    functionList.value = JSON.parse(functionListStr)
                }
            }
            
        }
    }
    if (resAccount) {
    const list = JSON.parse(resAccount);
    list.forEach((element) => {
        if (element.id === resCurAccountID) {
        element.active = true;
        currentAccount.value = element;
        }
    });
    accountList.value = list;
    }
    if (!isNetworkAvailable) {
    videoStatus.value = 5;
    return;
    }
    if (!isAudioInput.value) {
    videoStatus.value = 6; 
    } else if (!isAudioOutput.value) {
    videoStatus.value = 10;
    } else {
    chatQWeb.playSystemSound && Qrequest(chatQWeb.playSystemSound, 0);
    handleRecorder();
    startCounter();
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};
//10s没有声音自动进入休眠
let count = 0; // 计数器的初始值
let timerId = null; // 用于存储计时器的ID
// 创建一个计时器，每秒更新计数器的值
const startCounter = () => {
    console.log("startCounter-------");
    timerId = setInterval(() => {
    count++;
    console.log("startCounter", count, question.value.trim().length);
    if (count == 2 && question.value.trim().length !== 0) {
        console.log("5count", "startCounter", count);
        sigAudioASRError();
    } else if (count == 10) {
        console.log("10count", count);
        sigAudioASRError();
        videoStatus.value = 1;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    }, 1000); // 每秒更新一次计数器的值
};
// 销毁计时器，停止更新计数器的值
const stopCounter = () => {
    console.log("stopCounter------");
    if (timerId) {
    clearInterval(timerId);
    timerId = null;
    count = 0;
    }
};
/**录音倒计时 */
const countDown = ref(0);
const sigAudioCountDown = (res) => {
    countDown.value = res;
    if (countDown.value > 0) {
    videoStatus.value = 7;
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    } else {
    sigAudioASRError();
    }
};
const recording = ref(false); //录音中为true
/**语音聆听--录音 */
const handleRecorder = async () => {
    console.log(
    999,
    "handleRecorder",
    recording.value,
    showStop.value,
    isAudioInput.value
    );
    if (!isAudioInput.value || showStop.value) return;
    if (recording.value) return sigAudioASRError();
    if (!recording.value) {
    isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable);
    await Qrequest(chatQWeb.stopPlayTextAudio);
    if (isAudioInput.value && router.currentRoute.value.name == 'DigitalImage') {
        console.log("startRecorder-------");
        await Qrequest(chatQWeb.startRecorder, 1);
        recording.value = true;
        playAudioID.value = "";
    }
    }
};
/**录音信号返回 */
const sigAudioASRStream = (res, isEnd) => {
    console.log("sigAudioASRStream", res, isEnd);
    question.value = res;
    if (res !== "") {
    stopCounter();
    if (videoStatus.value == 2 || videoStatus.value == 7) {
        startCounter();
    }
    }
    //因为信号有延迟所以只能加判断等信号结束再跳转把之前录音丢掉
    if (isEnd && AudioASRStreamState.value) {
    stopCounter();
    router.push("/chat");
    } else if (isEnd && videoStatus.value !== 1 && videoStatus.value !== 10) {
    // sendQuestion();
    sendQuestion();
    }
};
/**录音声波起伏 */
const sigAudioSampleLevel = (res) => {
    const data = 1 + (res / 100) * 0.7;
    const data1 = 1 + (res / 100) * 1.2;

    animateScale.value = data;
    animateScale1.value = data1;
};

const talkID = ref("");

const currentTime = () => {
    //获取当前年月日时分秒
    var today = new Date().toISOString().slice(0, 10);

    const now = new Date();
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const seconds = String(now.getSeconds()).padStart(2, '0');
    const milliseconds = String(now.getMilliseconds()).padStart(3, '0');

    return `${today} ${hours}:${minutes}:${seconds}.${milliseconds}`;
}

const likeOrNotAgent = ['ztbbd', 'xxzsk', 'yzsbs']
const initLikeOrNotExt = () => {
    //招投标初始化ext
    if(likeOrNotAgent.includes(currentAssistant.value.id)){
        let questionVec = []
        for (let index = 0; index < history.value.length; index++) {
            const element = history.value[index];
            
            questionVec.push({
                content: element.question.content,
                role:"user"
            })

            if (index < history.value.length - 1) {
                questionVec.push({
                    content: _.last(element.answers).displayContent,
                    role:"assistant"
                })
            }
        }

        let ext = [{
            type: store.ExtentionType.LikeOrNot,
            question: JSON.stringify(questionVec),
            questionTime:currentTime(),
            answer: "",
            answerTime: currentTime(),
            assistantName: currentAssistant.value.displayname ? currentAssistant.value.displayname : '',
            modelType:currentAccount.value.model ? currentAccount.value.model : '',
            llm:currentAccount.value.displayname ? currentAccount.value.displayname : '',
            likeOrNot: store.LikeOrNot.NONE
        }]
        _.last(_.last(history.value).answers).extention = JSON.stringify(ext)
    }
}

const lastConversationInfo = ref({})
const getNowConversationInfo = async (assistantId) => {
    //更新最后会话信息情况：
    //1. 切换助手
    //2. 新建会话 已做
    //3. 切换会话
    //4. 初始化 已做
    // TODO:重新获取当前助手会话的信息，再根据conversationTitle去判断当前是否为新会话
    const lastConversationInfo_= await Qrequest(chatQWeb.getLastConversation, assistantId)  //拿到当前助手的最后一个会话信息
    lastConversationInfo.value = JSON.parse(lastConversationInfo_)  //解析当前助手的最后一个会话信息
    await Qrequest(chatQWeb.setCurrentConversationId, assistantId, lastConversationInfo.value.conversationId)  //设置当前助手的最后一个会话信息
}

//修改历史记录结构体
const history = ref([])
const sendQuestion = async () => {
    console.log(" showEmbeddingPluginsConfig:", showEmbeddingPluginsConfig.value);
    console.log("isEmbeddingPluginsExist:", isEmbeddingPluginsExist.value);
    if(!isEmbeddingPluginsExist.value)
    {
        // showEmbeddingPluginsConfig.value=!showEmbeddingPluginsConfig.value;
        showEmbeddingPluginsConfig.value=true;
        console.log(" showEmbeddingPluginsConfig:Success:", showEmbeddingPluginsConfig.value);
        // videoStatus.value = 1;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    else{
        showEmbeddingPluginsConfig.value=false;
    }

    console.log(" showKnowledgeBaseConfig:", showKnowledgeBaseConfig.value);
    console.log("isKnowledgeBaseExist:", isKnowledgeBaseExist.value);
    if(!isKnowledgeBaseExist.value)
    {
        // showEmbeddingPluginsConfig.value=!showEmbeddingPluginsConfig.value;
        showKnowledgeBaseConfig.value=true;
        console.log(" showKnowledgeBaseConfig:Success:", showKnowledgeBaseConfig.value);
        // videoStatus.value = 1;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    else{
        showKnowledgeBaseConfig.value=false;
    }


    console.log("sendQuestion", question.value);
    if (question.value.trim().length === 0 || ChatWindowState.value) {
        question.value = "";
        return;
    }
    await Qrequest(chatQWeb.stopPlayTextAudio);
    playAudioID.value = "";
    if (recording.value) sigAudioASRError();
    stopCounter();

    const extention = []
    const ques = {
        displayContent:question.value,
        reqId:0,
        isRetry:false,
        onlineSearch: store.IsSearchOnline,
            extention:"[]"
    }

    // ques添加openThink字段
    if (currentAccount.value.model == store.Uos_Free && !store.IsOpenMcpServer) {
        ques.openThink = store.IsDeepThink
        ques.onlineSearch = store.IsSearchOnline
    }else{
        ques.openThink = false
        ques.onlineSearch = false
    }

    // MCP开关 uos ai智能体才支持
    if (store.IsOpenMcpServer && currentAssistant.value.type == store.AssistantType.UOS_AI && store.IsInstallUOSAiAgent) {
        extention.push({
            type: store.ExtentionType.McpBtnStatus,
            McpServers: true,
        })
        ques.extention = JSON.stringify(extention)
    }

    // 功能按钮存在
    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT||currentAssistant.value.type === store.AssistantType.AI_TEXT_PROCESSING_ASSISTANT) {
        const idx = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type).index
        if (idx !== -1) {
            extention.push({
                type: store.ExtentionType.FunctionButton,
                functionButton:functionList.value[idx].Function,
            })
            ques.extention = JSON.stringify(extention)
        }
    }

    // 知识库开关
    if (currentAssistant.value.type === store.AssistantType.UOS_AI && store.IsOpenKnowledgeBase) {
        extention.push({
            type: store.ExtentionType.KnowledgeBaseBtnStatus,
            knowledgeBaseBtnStatus: store.IsOpenKnowledgeBase  // 开启才会发送这个字段
        })
        ques.extention = JSON.stringify(extention)
    }

    // 文本处理智能体
    if (currentAssistant.value.type === store.AssistantType.AI_TEXT_PROCESSING_ASSISTANT) {
        const idx = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type).index
        if (idx !== -1) {
            ques.displayContent = "[" + functionList.value[idx].Name + "]\n " + ques.displayContent
        }
    }

    let sendQuestionAccount = currentAccount.value
    const { id, model, icon, displayname } = sendQuestionAccount
    
    history.value.push({
        question:ques,
        answers:[
            {
                displayContent:"",
                content:"",
                displayHash:"",
                reqId:"0",
                llmIcon: icon ? icon : '',
                llmName: displayname ? displayname : '',
                llmId: id ? id : '',
                llmModel:model,
                assistantId: currentAssistant.value.id ? currentAssistant.value.id : '',
                errCode:0,
                errInfo:"{}",  //补充json
                chatType:0,  //问答、文生图、FunctionCall
                isRetry:false,
                extention:"[]",
                thinkTime: "-1",
                openThink: false,  //数字形象默认关闭深度思考
                onlineSearch: true,  //数字形象默认开启联网搜索
                knowledgeSearchStatus: (store.IsOpenKnowledgeBase && currentAssistant.value.type === store.AssistantType.UOS_AI) ? true : false,  //是否显示知识库搜索
            }
        ]        
        })

    const quesResp = await Qrequest(chatQWeb.sendRequest, id ? id : '', JSON.stringify(ques))
    try{
        JSON.parse(quesResp)
    }
    catch(e){
        return
    }

    isFoundThinkStart.value = false
    isFoundThinkStop.value = false

    _.last(history.value).question = JSON.parse(quesResp)
    _.last(_.last(history.value).answers).reqId = JSON.parse(quesResp).reqId

    //初始化like or not
    initLikeOrNotExt()

    talkID.value = JSON.parse(quesResp).reqId;
    showStop.value = true;
    question.value = "";
    if (videoStatus.value !== 6 && videoStatus.value !== 10) {
        videoStatus.value = 3;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
}

const handleDragStart = (event) => {
    event.preventDefault();
    event.stopPropagation();
}

// 接受AI文本信息
const thinkStart = ref("<think>\n\n")
const thinkEnd = ref("\n\n</think>\n\n")
const startThinkTime = ref('')
const endThinkTime = ref('')
const isFoundThinkStart = ref(false)  //是否找到think开始标签
const isFoundThinkStop = ref(false)  //是否找到think结束标签

const isThinking = ref(false)
const replyMsg = ref({})
const answerDisplayMsg = ref([])
// 上次接收到的message对象
const lastMsg = ref({chatType: store.ChatAction.ChatNone})
// 处理可能的多信号叠加情况
const processSingleMessage = (msgValue, errCode) => {
    try {
        replyMsg.value = JSON.parse(msgValue);

        // 获取思考开始时间
        if (!isThinking.value && replyMsg.value.message.chatType == store.ChatAction.ChatTextThink){
            isThinking.value = true
            startThinkTime.value = new Date().getTime()
            _.last(_.last(history.value).answers).thinkTime = "-1"  //初始化think时间
        }

        // 获取思考结束时间
        if (isThinking.value && replyMsg.value.message.chatType != store.ChatAction.ChatTextThink){
            endThinkTime.value = new Date().getTime()
            _.last(_.last(history.value).answers).thinkTime = (Math.floor((endThinkTime.value - startThinkTime.value) / 1000)).toString()
            isThinking.value = false
        }

        /**
         * 直接push的情况
         * 1.上一个chatType不等于当前chatType且当前chatType不为工具调用
         * 2.当前chatType为工具调用且status为调用中
         */
         if((lastMsg.value.chatType !== replyMsg.value.message.chatType && replyMsg.value.message.chatType !== store.ChatAction.ChatToolUse) 
        || (replyMsg.value.message.chatType  === store.ChatAction.ChatToolUse && replyMsg.value.message.status === store.ToolUseStatus.Calling)){
            switch (errCode) {
                case -11000:  //智能体服务不可用
                    replyMsg.value.message.content = store.loadTranslations['Agent server is not available']
                    break;
                case -11001:  //智能体服务异常
                    replyMsg.value.message.content = store.loadTranslations['Agent server exception']
                    break;
                case -11100:  //MCP服务不可用
                    // replyMsg.value.message.content =  store.loadTranslations['MCP server is not available']
                    break;
                default:
            } 
            answerDisplayMsg.value.push(replyMsg.value.message)
        }

        /**
         * ++的情况
         * 思考内容，上一个也是思考内容
         * 正文内容，上一个也是正文内容
         */
        if (replyMsg.value.message.chatType === lastMsg.value.chatType && replyMsg.value.message.chatType !== store.ChatAction.ChatToolUse) {
            // 在answerDisplayMsg数组中找到最后一个chatType为replyMsg.value.message.chatType的元素
            for (let index = answerDisplayMsg.value.length - 1; index >= 0; index--) {
                const element = answerDisplayMsg.value[index];
                if (element.chatType === replyMsg.value.message.chatType) {
                    answerDisplayMsg.value[index].content += replyMsg.value.message.content
                    break
                }
            }
        }

        /**
         * 修改工具调用状态的情况
         */
        if (replyMsg.value.message.chatType === store.ChatAction.ChatToolUse ) {
            for (let index = 0; index < answerDisplayMsg.value.length; index++) {
                // 根据index和name更新工具调用状态
                if (answerDisplayMsg.value[index].index === replyMsg.value.message.index && answerDisplayMsg.value[index].name === replyMsg.value.message.name) {
                    answerDisplayMsg.value[index] = replyMsg.value.message
                }
            }
        }
        

        _.last(_.last(history.value).answers).displayContent = JSON.stringify(answerDisplayMsg.value)

        lastMsg.value = replyMsg.value.message
        
        return true;
    } catch (e) {
        Qrequest(chatQWeb.writeVueLog, store.LogLevel.CRITICAL, "DigitalImage.vue processSingleMessage error:" + e.message)
        return false;
    }
};

const sigAiReplyStream = (type, value, status) => {
    // 尝试处理原始value
    if (!processSingleMessage(value, status)) {
        // 如果处理失败，尝试分割可能的多信号数据
        const potentialMessages = value.split('}\n{').map((msg, i) => 
            i === 0 ? msg + '}' : 
            i === value.split('}\n{').length - 1 ? '{' + msg : 
            '{' + msg + '}'
        );
        
        for (const msg of potentialMessages) {
            processSingleMessage(msg, status);
        }
    }

    if (status != 0) {
        _.last(_.last(history.value).answers).errCode = status
        _.last(_.last(history.value).answers).reqId = _.last(history.value).question.reqId
        // const { icon, displayname } = currentAccount.value
        // _.last(_.last(history.value).answers).llmIcon = icon
        // _.last(_.last(history.value).answers).llmName = displayname
        // _.last(_.last(history.value).answers).llmId = currentAccount.value.id
        _.last(_.last(history.value).answers).assistantId = currentAssistant.value.id
        _.last(_.last(history.value).answers).assistantName =  currentAssistant.value.displayname

        if(likeOrNotAgent.includes(currentAssistant.value.id)){
            //填充点赞踩结束时间
            let ext = JSON.parse(_.last(_.last(history.value).answers).extention)
            let answer = {
                answer: _.last(_.last(history.value).answers).displayContent,
                reference:""
            }

            for (let index = 0; index < ext.length; index++) {
                const element = ext[index];
                if (element.type == store.ExtentionType.PerView) {
                    answer.reference = element.sources[0].docContents
                }
            }

            let rate = {}
            for (let index = 0; index < ext.length; index++) {
                const element = ext[index];
                if (element.type == store.ExtentionType.LikeOrNot) {
                    ext[index].answer = JSON.stringify(answer)
                    ext[index].answerTime = currentTime()
                    rate = ext[index]
                }
            }

            _.last(_.last(history.value).answers).extention = JSON.stringify(ext)

            //写数据库
            Qrequest(chatQWeb.rateAnwser, history.value.length - 1, _.last(history.value).answers.length - 1, store.LikeOrNot.EMPTY, JSON.stringify(rate))
        }

        if (status == 200) {
            if (videoStatus.value !== 6 && videoStatus.value !== 10) {
                videoStatus.value = 4;
            }
        }else {
            if (status === -9002 || status === -9000 || status === -9001 || status === -9003 || status === -9004 || status === -9100) {
                errorCon.value = answerDisplayMsg.value.content;
                videoStatus.value = 8;
            } else {
                videoStatus.value = 4;
            }
        }

        playTextAudio();

        _.last(_.last(history.value).answers).errInfo = JSON.stringify(judgeInstallOrConfig(status, _.last(_.last(history.value).answers).knowledgeSearchStatus))
        //存日志
        // Qrequest(chatQWeb.logConversations,  JSON.stringify(history.value))
        Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

        isFoundThinkStart.value = false
        isFoundThinkStop.value = false

        answerDisplayMsg.value = []
        lastMsg.value = {chatType: store.ChatAction.ChatNone}
    }

    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);  //更新动画状态
};

//判断当前历史记录为去安装还是去配置
const needConfigErrCodeArr = [-9000, -9001, -9003, -9002, -9004, -9100]
const judgeInstallOrConfig = (errCode, isOpenKnowledgeBase) => {
    const errInfo = {
        info:"",
        exec:''
    }
    
    if(!needConfigErrCodeArr.includes(errCode)){
        errInfo.info = 'Not Need Config'        
        return errInfo
    }

    //无模型
    if(accountList.value.length == 0){
        errInfo.info = store.loadTranslations['Go to configuration']
        errInfo.exec = 'Qrequest(chatQWeb.launchLLMConfigWindow, false)'
        return errInfo
    }

    if(currentAssistant.value.type == store.AssistantType.UOS_SYSTEM_ASSISTANT 
        || currentAssistant.value.type == store.AssistantType.DEEPIN_SYSTEM_ASSISTANT 
        || currentAssistant.value.type == store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT
        || (currentAssistant.value.type == store.AssistantType.UOS_AI && isOpenKnowledgeBase)){
        if(!isEmbeddingPluginsExist.value){
            errInfo.info = store.loadTranslations['To install']
            errInfo.exec = 'Qrequest(chatQWeb.installEmbeddingPlugins)'
            return errInfo
        }
        if(!isKnowledgeBaseExist.value && currentAssistant.value.type == store.AssistantType.UOS_AI && isOpenKnowledgeBase){
            errInfo.info = store.loadTranslations['Go to configuration']
            errInfo.exec = 'Qrequest(chatQWeb.launchKnowledgeBaseConfigWindow)'  //TODO:跳转到具体配置位置
            return errInfo
        }
    }

    errInfo.info = store.loadTranslations['Go to configuration']
    errInfo.exec = 'Qrequest(chatQWeb.launchLLMConfigWindow, false)'
    return errInfo
}


const goConfig = async(event) => {
    if(history.value.length == 0) return
    eval(JSON.parse(_.last(_.last(history.value).answers).errInfo).exec)
    return
}

const configMsg = computed ( () => {
    if(history.value.length == 0) return ""
    if(_.last(_.last(history.value).answers).errCode <= -9000){
        return JSON.parse(_.last(_.last(history.value).answers).errInfo).info
    }
    return ""
})

const goGetFreeCredits = async(event) => {
    await Qrequest(chatQWeb.getFreeCredits, true)
    return
}

const showConfigLink = computed ( () => {
    if(history.value.length == 0) return false
    if(_.last(_.last(history.value).answers).errCode <= -9000 && videoStatus.value == 8) return true
    return false
})

const errDisplayInfo = computed ( () => {
    if(history.value.length == 0) return ""
    if(_.last(_.last(history.value).answers).errCode <= -9000 && videoStatus.value == 8) {
    return JSON.parse(_.last(_.last(history.value).answers).displayContent)[0].content
    }
    return ""
})

/**激活静置状态切换 */
const activeFun = (res) => {
    if (playStatus.value) return;
    console.log("activeFun", videoStatus.value, res);

    if (videoStatus.value == 2 || videoStatus.value == 7) {
    if (res && question.value.trim().length !== 0) {
        sigAudioASRError();
        return;
    }
    sigAudioASRError(); //stopRecorder
    videoStatus.value = 1;
    //聆听状态
    } else if (videoStatus.value == 3) {
    //思考状态
    //大模型回答播报状态
    stopRequest(); //logAiChatRecord
    videoStatus.value = 1;
    } else if (videoStatus.value == 4) {
    //聆听状态
    videoStatus.value = 1;
    stopPlayTextAudio();
    } else if (videoStatus.value == 1 || videoStatus.value == 9) {
    //休眠状态,异常结束
    videoStatus.value = 2;
    recording.value = false;
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};
/**数字形象自然语音状态切换 */
const handelModel = () => {
    if (videoStatus.value == 2) {
    AudioASRStreamState.value = true;
    if (question.value.trim().length === 0) {
        recording.value = false;
        Qrequest(chatQWeb.stopRecorder);
        stopCounter();
        router.push("/chat");
    } else {
        sigAudioASRError();
    }
    } else if (videoStatus.value == 3 || videoStatus.value == 4) {
    return;
    } else {
    stopCounter();
    router.push("/chat");
    }
};
/**监听麦克风状态 */
const sigAudioInputDevChange = (res) => {
    console.log("sigAudioInputDevChange");
    isAudioInput.value = res;
    if (!res) recording.value = false;
    if (!res) {
    videoStatus.value = 6;
    } else if (!isAudioOutput.value) {
    videoStatus.value = 10;
    } else {
    videoStatus.value = 1;
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};
/**监听输出设备状态 */
const sigAudioOutputDevChanged = async (res) => {
    isAudioOutput.value = res;
    if (!res) {
    playAudioID.value = "";
    if (videoStatus.value == 2 || videoStatus.value == 7) {
        await Qrequest(chatQWeb.stopRecorder);
    } else if (videoStatus.value == 3) {
        stopRequest(); //logAiChatRecord
    } else if (videoStatus.value == 4) {
        //聆听状态
        stopPlayTextAudio();
    }
    videoStatus.value = 10;
    } else if (!isAudioInput.value) {
    videoStatus.value = 6;
    } else {
    videoStatus.value = 1;
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};
/**快捷键开启录音 */
// const sigAudioRecShortcutPressed = () => {
//   console.log("sigAudioRecShortcutPressed")
//   activeFun();
// };

//语音播放
const playTextAudio = async (res) => {
    // await Qrequest(chatQWeb.playTextAudio, talkID, res || content, true);
    // playAudioID.value = talkID;

    const { displayContent, reqId } = _.last(history.value).answers[0];
    let displayContentObj = JSON.parse(displayContent)

    // 新增：拼接所有chatType为ChatTextPlain的content
    let playText = ''
    displayContentObj.forEach(obj => {
        if (obj.chatType === store.ChatAction.ChatTextPlain) {
            playText += obj.content
        }
    })

    await Qrequest(chatQWeb.playTextAudio, reqId, res || playText, true, false);
    playAudioID.value = reqId;
};
const stopPlayTextAudio = async () => {
    await Qrequest(chatQWeb.stopPlayTextAudio);
    playAudioID.value = "";
};

/**录音异常结束 */
const sigAudioASRError = async (err, text) => {
    console.log("sigAudioASRError", err, text);
    recording.value = false;
    await Qrequest(chatQWeb.stopRecorder);
    if (err && AudioASRStreamState.value) {
    router.push("/chat");
    }
    if (err && videoStatus.value !== 6) {
    videoStatus.value = 9; // Only
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    errorCon.value = text;
    }
};

// 语音播放结束
const sigPlayTTSFinished = (res) => {
    console.log("sigPlayTTSFinished", res);
    if (playAudioID.value === res) playAudioID.value = "";
    //上一条语音播放结束自动进入激活状态再去聆听
    if (
    videoStatus.value !== 5 &&
    videoStatus.value !== 6 &&
    videoStatus.value !== 8 &&
    videoStatus.value !== 9
    ) {
    videoStatus.value = 1;
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    showStop.value = false;
};
const sigPlayTTSError = (res) => (playAudioID.value = "");

//停止请求
const stopRequest = async () => {
    await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

    if(likeOrNotAgent.includes(currentAssistant.value.id)){
        //填充结束时间
        let ext = JSON.parse(_.last(_.last(history.value).answers).extention)
        for (let index = 0; index < ext.length; index++) {
            const element = ext[index];
            if (element.type == store.ExtentionType.LikeOrNot) {
                ext[index].answerTime = currentTime()
            }
        }
        _.last(_.last(history.value).answers).extention = JSON.stringify(ext)
    }    

    if (_.last(_.last(history.value).answers).errCode == 0) {  //正常返回
        _.last(_.last(history.value).answers).errCode = 298  //请求被取消
    }
    
    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

    showStop.value = false;
    isFoundThinkStart.value = false
    isFoundThinkStop.value = false

    isThinking.value = false
    answerDisplayMsg.value = []
    lastMsg.value = {chatType: store.ChatAction.ChatNone}

    stopCounter();
}
// AI模型列表更新
const llmAccountLstChanged = (id, list) => {
    console.log(6555, id, list);
    const _list = JSON.parse(list);
    const index = _list.findIndex((item) => item.id === currentAccount.value.id);
    if (
    (id !== currentAccount.value.id &&
    _list.length !== 0 &&
    videoStatus.value !== 5 &&
    videoStatus.value !== 6 && isKnowledgeBaseExist.value) || (accountList.value.length == 0)
    ) {
    videoStatus.value = 1;
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    if (!id) currentAccount.value = {};
    if (index > -1) {
    _list[index].active = true;
    currentAccount.value = _list[index];
    } else {
    _list.forEach((element) => {
        if (element.id === id) {
        element.active = true;
        currentAccount.value = element;
        }
    });
    }
    accountList.value = _list;
};

/**enter键,主动发送 */
const handleEnterKey = (event) => {
    console.log("handleEnterKey", question.value);
    if (question.value.trim().length !== 0) {
    sigAudioASRError();
    }
};
/**网络连接情况 */
const sigNetStateChanged = (res) => {
    if (!res) {
    if (videoStatus.value == 2) {
        sigAudioASRError();
    }
    if (videoStatus.value == 3) {
        stopRequest();
    }
    if (videoStatus.value == 4) {
        stopPlayTextAudio();
    }
    videoStatus.value = 5;
    } else {
    videoStatus.value = 1;
    if (!isAudioInput.value) {
        videoStatus.value = 6;
    } else if (!isAudioOutput.value) {
        videoStatus.value = 10;
    }
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};

// 接受AI图片信息
const sigText2PicFinish = (id, paths) => {
    _.last(_.last(history.value).answers).displayContent = JSON.stringify(paths)
    const _type = _.last(_.last(history.value).answers).chatType;
    const _status = _.last(_.last(history.value).answers).errCode;
    _.last(_.last(history.value).answers).errInfo = JSON.stringify(judgeInstallOrConfig(_status, _.last(_.last(history.value).answers).knowledgeSearchStatus))

    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

    showStop.value = false;
    if (_status >= 0 && _type === 2 && paths) {
    playTextAudio(
        useGlobalStore().loadTranslations[
        "The picture has been generated, please switch to the chat interface to view it."
        ]
    );
    if (videoStatus.value !== 6 && videoStatus.value !== 10) {
        videoStatus.value = 4;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    }
};

watch( () => videoStatus.value, (newValue) => {
    onPlay.value = newValue;
    console.log("watch", newValue);
    if (newValue == 1) {
        chatQWeb &&
        chatQWeb.playSystemSound &&
        Qrequest(chatQWeb.playSystemSound, 1);
        stopPlayTextAudio();
        stopCounter();
        showStop.value = false;
    } else if (newValue == 2) {
        chatQWeb &&
        chatQWeb.playSystemSound &&
        Qrequest(chatQWeb.playSystemSound, 0);
        showStop.value = false;
        recording.value = false;
        handleRecorder();
        startCounter();
    } else {
        stopCounter();
    }
    if (newValue == 4 && ChatWindowState.value) {
        stopPlayTextAudio();
    }
    if (newValue == 5 || newValue == 6 || newValue == 8 || newValue == 10) {
        if (document.querySelector(".video5")) {
        document.querySelector(".video5").currentTime = 0;
        document.querySelector(".video5").play();
        }
        playStatus.value = true;
    } else if (newValue == 9) {
        if (document.querySelector(".video5")) {
        document.querySelector(".video5").currentTime = 0;
        document.querySelector(".video5").play();
        }
        playStatus.value = false;
    } else {
        document.querySelector(".video5") &&
        document.querySelector(".video5").pause();
        playStatus.value = false;
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, newValue);
    },
    { deep: true, immediate: false }
);
/** */
const sigChatConversationType = (id, type) => {
    if (_.last(history.value) === undefined)
    return;

    _.last(_.last(history.value).answers).chatType = type;
};
/** 窗口关闭*/
const sigWebchat2BeHiden = () => {
    ChatWindowState.value = true;
    if (videoStatus.value == 2) {
    sigAudioASRError();
    videoStatus.value = 1;
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
    if (videoStatus.value == 4) {
    stopPlayTextAudio();
    }
};
/** 窗口打开*/
const sigWebchat2BeShowed = async () => {
    ChatWindowState.value = false;
    if (videoStatus.value == 1) {
    videoStatus.value = 2;
    }
    if (videoStatus.value == 4) {
    sigPlayTTSFinished();
    videoStatus.value = 2;
    }
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
};
const sigThemeChanged = (res) => {
    updateTheme(res);
};
const sigFontChanged = (family, pixelSize) => {
    updateFont(family, pixelSize);
};
/** ctrl+super+c激活*/
const sigDigitalModeActive = () => {
    if (videoStatus.value == 1) {
    videoStatus.value = 2;
    Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
};
const sigChatModeActive = () => {
    handelModel();
}
// 活动色改变
const sigActiveColorChanged = (res) => updateActivityColor(res);

const isModalState = ref(false)
const handleActive = (res) =>{
    let maskDom = document.querySelector('#drop-mask');
    if (!maskDom) {
        maskDom = document.createElement("div");
        maskDom.id = "drop-mask"
        document.body.appendChild(maskDom);
    }
    maskDom.style.height = '100vh'
    maskDom.style.width = '100vw'
    maskDom.style.display = res ? 'flex' : 'none'
    isModalState.value = res
}
const sigWebchatModalityChanged = (res) => handleActive(res)

const sigMainContentBackgroundColor = (color) => {
    updateMainContentBackgroundColor(color);
}

const sigKnowledgeBaseStatusChanged = (status) => {
    console.log("knowledge base status changed: ", status);
    if(!isKnowledgeBaseExist.value && status){
    videoStatus.value=1;
    }
    isKnowledgeBaseExist.value = status;
  
    // if( isKnowledgeBaseExist.value==true){
    //   videoStatus.value=1;
    // //  Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    // }
};
const sigEmbeddingPluginsStatusChanged = (status) => {
    if(!isEmbeddingPluginsExist.value && status){   
        videoStatus.value=1;
    }
    isEmbeddingPluginsExist.value = status;
  
    // if(isEmbeddingPluginsExist.value==true){
    // videoStatus.value=1;
    //Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    // }
};

const sigAppendWordWizardConv = async (type) => {
    //打断正在进行的对话
    if(showStop.value){
        await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

        _.last(_.last(history.value).answers).errCode = 298  //请求被取消

        //存日志
        // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
        await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

        showStop.value = false;
        stopCounter();
        videoStatus.value = 1;
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    store.ConversationModeStatus = store.ConversionMode.Normal
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    // 根据type创建新会话
    for (let index = 0; index < assistantList.value.length; index++) {
        const element = assistantList.value[index];
        if (element.type === type) {
            // 切换助手
            element.active = true
            currentAssistant.value = element
            await Qrequest(chatQWeb.setCurrentAssistantId, element.id)

            //创建新会话
            await Qrequest(chatQWeb.createNewConversation) //创建新会话
            await getNowConversationInfo(currentAssistant.value.id)  //设置当前助手的最后一个会话信息
            break
        }
    }

    //通知后端添加划词对话历史记录到UOS AI历史记录
    Qrequest(chatQWeb.appendWordWizardConv, type)

    //切回聊天
    handelModel()    
}

const sigPreviewReference = async (preViewList) => {
    let extention = []
    extention = extention.concat(JSON.parse(_.last(_.last(history.value).answers).extention))
    extention.push(JSON.parse(preViewList))
    _.last(_.last(history.value).answers).extention = JSON.stringify(extention)
}

const sigAsyncWorker = async (type) => {
    if (type === 1) {  // 覆盖输入框内容
        //打断正在进行的对话
        if(showStop.value){
            await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

            _.last(_.last(history.value).answers).errCode = 298  //请求被取消

            //存日志
            // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
            await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

            showStop.value = false;
            stopCounter();
            videoStatus.value = 1;
            Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
        }

        assistantList.value.forEach(element => {
            if (element.type === store.AssistantType.UOS_AI) {
                Qrequest(chatQWeb.setCurrentAssistantId, element.id)
            }
        });

        // TODO: 切换会话模式后， 需要将当前模式同步到后端
        store.ConversationModeStatus = store.ConversionMode.Normal
        await Qrequest(chatQWeb.setConversationMode, store.ConversionMode.Normal)

        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    } else if (type === 2) {  // 继续对话
        //打断正在进行的对话
        if(showStop.value){
            await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

            _.last(_.last(history.value).answers).errCode = 298  //请求被取消

            //存日志
            // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
            await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

            showStop.value = false;
            stopCounter();
            videoStatus.value = 1;
            await Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
        }
        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    } else if (type === 3) {  // 问一问
        //打断正在进行的对话
        if(showStop.value){
            await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

            _.last(_.last(history.value).answers).errCode = 298  //请求被取消

            //存日志
            // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
            await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

            showStop.value = false;
            stopCounter();
            videoStatus.value = 1;
            Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
        }

        //打断正在进行的语音输入
        if (recording.value) {
                recording.value = false;
            await Qrequest(chatQWeb.stopRecorder)
        }

        // 切文本助手
        if (currentAssistant.value.type !== store.AssistantType.UOS_AI) {
            assistantList.value.forEach(element => {
                if (element.type === store.AssistantType.UOS_AI) {
                    Qrequest(chatQWeb.setCurrentAssistantId, element.id)
                }
            });    

            //切助手和模型
            currentAssistant.value.active = false
            const resCurAccountID = await Qrequest(chatQWeb.currentLLMAccountId)
            const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
            for (const element of assistantList.value) {
                    if (element.id === resCurAssistantID) {
                    element.active = true
                    currentAssistant.value = element
                    //查询功能按钮list
                    const functionListStr = await Qrequest(chatQWeb.getAssistantFunctions, currentAssistant.value.type)
                    if (functionListStr !== "") {
                        functionList.value = JSON.parse(functionListStr)
                    }
                }
             }
        
            currentAccount.value.active = false
            accountList.value.forEach(element => {
                if (element.id === resCurAccountID) {
                    element.active = true
                    currentAccount.value = element
                }
            });
            
            // TODO: 切换会话模式后， 需要将当前模式同步到后端
            store.ConversationModeStatus = store.ConversionMode.Normal
            await Qrequest(chatQWeb.setConversationMode, store.ConversionMode.Normal)
        }
        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
        //切回聊天
        handelModel()  
    } else if (type === 4) {  // 截图OCR
        //打断正在进行的对话
        if(showStop.value){
            await Qrequest(chatQWeb.cancelAiRequest, talkID.value);

            _.last(_.last(history.value).answers).errCode = 298  //请求被取消

            //存日志
            // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
            await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

            showStop.value = false;
            stopCounter();
            videoStatus.value = 1;
            Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
        }

        //打断正在进行的语音输入
        if (recording.value) {
                recording.value = false;
            await Qrequest(chatQWeb.stopRecorder)
        }

        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
        //切回聊天
        handelModel()  
    }
}

const sigGetFreeCreditsResult = async (res, msg) => {
    store.IsGotFreeCredits = res
    if (res) {
        videoStatus.value = 2;  // 激活状态
        Qrequest(chatQWeb.updateVoiceConversationStatus, videoStatus.value);
    }
}

const sigIsGotFreeCredits = (isGot) => {
    store.IsGotFreeCredits = isGot
}


const sigAssistantListChanged = async () => {
    const resAssistant = await Qrequest(chatQWeb.queryAssistantList)
    const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
    if (resAssistant) {
        const list = JSON.parse(resAssistant);
        //判断当前助手ID是否在列表中
        const isExist = list.some(item => item.id === resCurAssistantID)
        assistantList.value = list;
        if (!isExist) {
            //打断正在进行的语音输入
            if (recording.value) {
                recording.value = false;
                await Qrequest(chatQWeb.stopRecorder)
            }
            //打断正在进行的对话
            if(showStop.value){
                await Qrequest(chatQWeb.cancelAiRequest, talkID.value)

                _.last(_.last(history.value).answers).errCode = 298  //请求被取消

                //存日志
                // await Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
                await Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

                //打断语音播放
                await Qrequest(chatQWeb.stopPlayTextAudio)

                showStop.value = false
            }
            //清除历史记录
            //切助手到UOS AI
            assistantList.value.forEach(async (element) => {
                if (element.type === store.AssistantType.UOS_AI) {
                    element.active = true
                    currentAssistant.value = element
                    await Qrequest(chatQWeb.setCurrentAssistantId, element.id)
                }
            });

            // TODO: 切换会话模式后， 需要将当前模式同步到后端
            store.ConversationModeStatus = store.ConversionMode.Normal
            await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

        }else {
            //切助手到当前助手
            assistantList.value.forEach(element => {
                if (element.id === resCurAssistantID) {
                    element.active = true
                    currentAssistant.value = element
                }
            });
        }
    }
}

const responseAIFunObj = {
    sigAiReplyStream,
    sigAudioASRStream,
    sigAudioASRError,
    sigAudioCountDown,
    sigPlayTTSFinished,
    sigPlayTTSError,
    sigActiveColorChanged,
    sigAudioInputDevChange,
    sigAudioSampleLevel,
    sigNetStateChanged,
    sigText2PicFinish,
    sigChatConversationType,
    sigWebchat2BeHiden,
    sigWebchat2BeShowed,
    sigThemeChanged,
    sigFontChanged,
    llmAccountLstChanged,
    sigDigitalModeActive,
    sigChatModeActive,
    sigAudioOutputDevChanged,
    sigWebchatModalityChanged,
    sigMainContentBackgroundColor,
    sigKnowledgeBaseStatusChanged,
    sigEmbeddingPluginsStatusChanged,
    //随航继续对话接口
    sigAppendWordWizardConv,  //停止当前对话,获取随航历史记录

    //预览列表
    sigPreviewReference,
    sigAssistantListChanged,  //助手列表变化
    sigAsyncWorker,
    sigGetFreeCreditsResult,  // 领取免费额度结果
    sigIsGotFreeCredits,  // 是否领取过免费额度
};
function handleKeyDown(event) {
    if (event.key === "Enter") {
    event.stopPropagation(); // 阻止事件冒泡
    // 或者
    event.preventDefault(); // 阻止默认行为
    // 然后触发你想要的方法
    handleEnterKey(event);
    }
}
onMounted(async () => {
    for (const key in responseAIFunObj) {
    if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
        chatQWeb[key].connect(responseAIFunObj[key]);
    }
    }
    document.addEventListener("keydown", handleKeyDown);
    initChat();
    useGlobalStore().loadTranslations = await Qrequest(chatQWeb.loadTranslations);
});
onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
    if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
        chatQWeb[key].disconnect(responseAIFunObj[key]);
    }
    }
    document.removeEventListener("keydown", handleKeyDown);
});
</script>

<style lang="scss" scoped>
$scaleValue: var(--scale); // 缩放比例
$scaleValue1: var(--scale1); // 缩放比例
::selection {
    color: inherit;
}
video ::selection {
    background: none;
}
.main-content {
    position: relative;
    display: flex;
    flex-direction: column;
    height: 100vh;
    width: 100vw;
    overflow: hidden;
    color: var(--uosai-color-shortcut);
    background-color: var(--main-content-background-color);
    font-size: 1.14rem;
    .logo {
    width: 100%;
    height: 155px;
    text-align: center;
    margin-top: 35px;
    img {
        width: 96px;
        height: 96px;
        user-select: none;
    }
    div {
        font-size: 1.42rem;
        font-weight: 600;
        font-style: normal;
        margin-top: 10px;
        color: var(--uosai-color-title);
    }
    }
    .close {
    width: 12px;
    height: 12px;
    position: absolute;
    right: 7px;
    top: 7px;
    padding: 12px;
    &:hover {
        cursor: pointer;
    }
    }
    .play {
    width: 36px;
    height: 36px;
    position: absolute;
    bottom: 7.86vh;
    left: 12.75vw;
    padding: 5px;
    fill: var(--uosai-color-clear);
    color: var(--uosai-color-clear);
    &:not(.disabledDig):hover {
        cursor: pointer;
        fill: var(--uosai-color-clear-hover);
        color: var(--uosai-color-clear-hover);
    }
    &:not(.disabledDig):active {
        fill: var(--activityColor);
        color: var(--activityColor);
    }
    z-index: 20;
    &.disabledDig {
        fill: var(--uosai-color-inputcontent-placeholder);
        color: var(--uosai-color-inputcontent-placeholder);
    }
    }
    .animate-box {
    position: absolute;
    width: 300px;
    height: 300px;
    text-align: center;
    z-index: 1;
    left: 50%;
    transform: translateX(-50%);
    top: 40%;
    user-select: none;
    &:hover {
        cursor: pointer;
    }
    .listen {
        position: absolute;
        display: flex;
        justify-content: center;
        width: 100%;
        height: auto;
        left: 0;
        img {
        width: 100%;
        height: auto;
        }
    }
    video {
        position: absolute;
        width: 100%;
        height: auto;
        z-index: 1;
        left: 0;
    }
    }
    .scale {
    position: absolute;
    width: 300px;
    left: 0;
    animation: scale 50ms linear;
    animation-fill-mode: forwards;
    }
    .scale1 {
    position: absolute;
    width: 300px;
    left: 0;
    animation: scale1 50ms linear;
    animation-fill-mode: forwards;
    // animation: scale1 50ms ease-in-out infinite;
    }
    .rotate {
    transform-origin: center center; /* 设置旋转中心为圆心 */
    animation: rotate 6s linear infinite;
    }
    .active-box {
    width: 100%;
    margin: 0 auto 6.12vh;
    left: 0;
    text-align: center;
    font-size: 1.42rem;
    z-index: 10;
    .pr {
        position: relative;
    }
    .active-con {
        width: calc(100% - 60px);
        margin-left: 30px;

        .dian {
        letter-spacing: 1.5px;
        margin-left: 0px;
        display: inline-block;
        }
        pre {
        word-break: break-all;
        white-space: pre-wrap;
        line-height: 25px;
        width: 80vw;
        margin: auto;
        font-size: 1rem;
        }
        .errDisplayInfo{
        font-size: 1rem;
        }
        .go-config {
        font-size: 1rem;
        color: var(--activityColor);
        cursor: pointer;
        text-decoration: none;
        white-space: nowrap; /* 不换行 */
        }
        svg {
        width: 10px;
        height: 14px;
        margin-right: 8px;
        }
        .fz-14 {
        font-size: 1rem;
        margin-top: 5px;
        }
    }
    .handel {
        position: relative;
        width: 70px;
        height: 70px;
        .svgBg {
        width: 70px;
        height: 70px;
        background: var(--activityColor);
        border-radius: 50%;
        display: flex;
        justify-content: center;
        align-items: center;
        }
        .svgBg-hover {
        width: 70px;
        height: 70px;
        position: absolute;
        top: 0;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 50%;
        display: none;
        }
        .icon-dig-play {
        width: 18px;
        height: 20px;
        }
        .icon-dig-stop {
        width: 16px;
        height: 16px;
        }
        margin: 14.6vh auto 0;
    }
    }
    .tip {
        position: absolute;
        bottom: 8px;
        left: 0;
        right: 0;
        color: var(--uosai-bottom-tip-color);
        font-size: 0.71rem;
        font-weight: 500;
        font-style: normal;
        text-align: center;
        user-select: none;
    }
    .disabledDig {
    cursor: not-allowed;
    }
    :deep(.icon-change-normal) {
    fill: var(--activityColor) !important;
    }
}
@keyframes scale {
    0% {
    transform: scale(1); /*开始为原始大小*/
    }
    100% {
    transform: scale($scaleValue);
    }
}
@keyframes scale1 {
    0% {
    transform: scale(1); /*开始为原始大小*/
    }
    100% {
    transform: scale($scaleValue1);
    }
    // 100% {
    //   transform: scale(1);
    // }
}
@keyframes rotate {
    0% {
    transform: rotate(0deg);
    }
    50% {
    transform: rotate(180deg);
    }
    100% {
    transform: rotate(360deg);
    }
}
.handel:not(.disabledDig):hover {
    cursor: pointer;
}
.handel:not(.disabledDig):hover > .svgBg-hover {
    display: block;
}
.handel:not(.disabledDig):active > .svgBg-hover {
    background: rgba(255, 255, 255, 0.05);
    display: block;
}
.handel:not(.disabledDig):active .svg-icon {
    opacity: 0.6;
}
.handel.disabledDig {
    .svgBg {
    opacity: 0.4;
    }
}
.dark {
    .handel:not(.disabledDig):hover > .svg-icon {
    display: block;
    background: rgba(255, 255, 255, 0.1);
    }
    .handel:not(.disabledDig):active > .svgBg-hover {
    background: rgba(255, 255, 255, 0.05);
    display: block;
    }
}
</style>
