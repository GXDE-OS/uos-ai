<template>
    <div class="chat-bubble" v-if="item">
        <!-- 用户侧气泡 -->
        <div :class="`question item`" v-show="showQuestion" :style="{'min-width': 'auto', 'margin-top' : props.historyIndex === 0 ? '15px' : '0'}">
            <pre><PromptTag v-show="showPromptTag" class="prompt-tag"
                :promptTag="promptTag" 
                />{{ questionDisplayContent }}</pre>             
        </div>
        <!-- 问题操作按钮 -->
        <div class="question-actions">
            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="store.loadTranslations['Copy']">
                <div class="action-btn" @click="questionAction('copy')" :class="{ disabled: showStop || recording }">
                    <svgIcon icon="question-copy" />
                </div>
            </el-tooltip>
            <div class="divider"></div>
            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="store.loadTranslations['Re-edit']">
                <div class="action-btn" @click="questionAction('reEdit')" :class="{ disabled: showStop || recording }">
                    <svgIcon icon="question-edit" />
                </div>
            </el-tooltip>
        </div>
        <!-- 文档内容 -->
        <div v-for="(file, index) in fileList" :key="index" >
            <DocumentParsing ref="documentParsingRef" class="documentParsing"
                v-if="file.type === store.DocParsingFileType.Doc" :fileInfo="file" :isDarkMode="isDarkMode"/> 
            <div v-else-if="file.type === store.DocParsingFileType.Image" class="image-container" @click="openImage(file.filePath)">
                <img class="image-file" :src="fileImageUrls[file.index]" alt="" />
            </div>
        </div>
        <!-- 左右切换按钮 -->
        <template v-if="props.item.answers && props.item.answers.length > 1">
            <div class="switch-btn" :class="{ disabled: showStop || recording }">
                <div class="prev btn" :class="{ disabled: activeIndex === 0 || recording }" @click="handleSwitch('prev')">
                    <svgIcon icon="arrow_left" />
                </div>
                <div style="user-select: none;">{{ activeIndex + 1 }}/{{ item.answers.length }}</div>
                <div class="next btn" :class="{ disabled: activeIndex + 1 === item.answers.length || recording }"
                    @click="handleSwitch('next')">
                    <svgIcon icon="arrow_right" />
                </div>
            </div>
        </template>
        <!-- 应答册气泡 -->
        <div :class="`answers item`" :style="{ minWidth: answerNowDisplayContent === '' || item.role === 'user' ? 'auto' : '200px' }">
            <!-- 知识库搜索状态显示 -->
            <div class="think" v-show="KnowledgeSearchStatus" style="margin-bottom: 6px;display: flex;width: 100%;">
                <div class="think-title" v-show="!isKnowledgeSearchComplete">
                    <div>{{ store.loadTranslations['Searching'] }}</div>
                    <div class="loading" v-show="!isKnowledgeSearchComplete" style="margin-left: 6px;"></div>
                </div>
                <div class="think-title" v-show="isKnowledgeSearchComplete">
                    <div>{{ knowledgeSearchCompleteContent }}</div>
                </div>
            </div>
            <!-- 思考状态 -->
            <div class="think" v-show="isShowThink">
                <div class="think-title" @click="showThinkContent">
                    <div>{{ thinkTitle }}</div>
                    <div class="loading" v-show="!isThinkComplete" style="margin-left: 6px;"></div>
                    <div v-show="isThinkContentFound" style="margin-left: 6px;display: flex;">
                        <SvgIcon :icon="isShowThinkContent ? 'arrow_up' : 'arrow_down'" style="fill: var(--uosai-think-title-svg-color);"/>
                    </div>
                </div>
                <div class="think-content" v-show="isShowThinkContent">
                    <div class="think-text" v-html="thinkContent" ></div>
                </div>
            </div>
            <!-- 正在加载 -->
            <div class="loading" v-if="answerNowDisplayContent === '' && showStop && isLast && item.answers[activeIndex].displayContent === '' && ((KnowledgeSearchStatus && isKnowledgeSearchComplete) || !KnowledgeSearchStatus)"></div>
            <!-- 文生图显示 -->
            <ImgBubble :isPPTShow="showPPTBtn" :isPosterShow="showPosterBtn" @handleShowTip="emit('handleShowTip', store.loadTranslations['Copied successfully'])" :disabled="recording || !!playAudioID"
                :content="answerNowDisplayContent" @clickOnPPT="downloadPPT" @clickOnEdit="downloadPoster" v-else-if="Array.isArray(answerNowDisplayContent)" :isWindowMode="props.isWindowMode"/>
            <!-- 文本显示 -->
            <div v-else  v-for="(item, index) in allDisplayContent" :key="index">
                <!-- 正文 -->
                <pre class="answerNowDisplayContent" :isDarkMode="isDarkMode" v-if="item.chatType == store.ChatAction.ChatTextPlain" v-html="item.content" ref="target"></pre> 
                <!-- 工具使用状态 -->
                <ToolUseItem v-if="item.chatType == store.ChatAction.ChatToolUse" :toolUseItem="item" :errCode="props.item.answers[activeIndex].errCode" @copyToolUseItem="copyToolUseItem"/>
            </div>
            <!-- <pre class="answerNowDisplayContent" :isDarkMode="isDarkMode"  v-html="answerNowDisplayContent" v-else ref="target"></pre>  -->
            <!-- 联网搜索 -->
            <pre class="answerNowDisplayContent"  v-show="item.answers[activeIndex].errCode == 10001" :isDarkMode="isDarkMode" ref="target">
                {{ store.loadTranslations['Search complete.'] }}
                <br>
                <span class="go-config" @click="openUrl"> {{ store.loadTranslations['Click to view results'] }}></span> 
            </pre> 
            <!-- 错误处理 -->
            <span class="go-config" v-show="item.answers[activeIndex].errCode <= -9000 && configMsg != 'Not Need Config'" @click="goConfig">{{ configMsg }}></span>  
            <span class="go-config" v-show="item.answers[activeIndex].errCode == -9002 && !store.IsGotFreeCredits" @click="goGetFreeCredits"> &nbsp;&nbsp;&nbsp;{{ store.loadTranslations['Claim Free Credits'] }}></span> 
            <!-- 原文预览 -->
            <div v-show="showPreViewList" class="PreViewList">
                <div class="PreViewListLabel">
                    <span>{{store.loadTranslations['Reference']}}</span>
                </div>
                <div class="PreViewListContent">
                    <Preview v-show="showPreViewList" v-for="(preView, index) in preViewList" :key="index"
                    :preView="preView"
                    :index="index"/>
                </div>
            </div>
            <!-- PPT操作 -->
            <div v-show="showPPTBtn" class="pptBtn">
                <button @click="editPPT(item.answers[activeIndex].displayContent)" v-show=!showPPTPicture class="operate-btn">编辑大纲</button>
                <button @click="genePPT(item.answers[activeIndex].displayContent)" v-show=!showPPTPicture  class="operate-btn">生成PPT</button>
                <button @click="downloadPPT" v-show=showPPTPicture class="operate-btn">下载和编辑</button>
           </div>
           <!-- 海报操作 -->
           <div v-show="showPosterBtn" class="posterBtn">
                <button @click="editPoster(item.answers[activeIndex].displayContent)" v-show=!showPosterPicture class="operate-btn">编辑内容</button>
                <button @click="genePoster(item.answers[activeIndex].displayContent)" v-show=!showPosterPicture class="operate-btn">生成海报</button>
           </div>
            <!-- 应答气泡底部显示 -->
            <div class="play-audio" v-show="answerNowDisplayContent !== '' || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)" >
                <!-- 模型名称和图标 -->
                <div class="model" v-if="item.answers && item.answers[activeIndex] && item.answers[activeIndex].llmName">
                    <img :src="`file://${item.answers[activeIndex].llmIcon}`" alt="">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="item.answers[activeIndex].llmName">
                        <div class="name">{{ item.answers[activeIndex].llmName }}
                        </div>
                    </el-tooltip>
                </div>
                <!-- 播放和复制按钮 -->
                <template v-if="showPlay && (answerNowDisplayContent !== '' || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)">
                    <div class="play-animation" :style="{ visibility: isActive ? 'visible' : 'hidden' }">
                        <svgIcon :icon="inconName" />
                    </div>
                    <svgIcon v-if="netState&&hasOutput" :class="{ disabled: showStop || recording || !netState }" style=""
                        v-show="!isActive" icon="play" @click="!showStop && !recording && netState && hasOutput&& playTextAudio()" />
                    <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="!hasOutput?store.loadTranslations['The sound output device is not detected, please check and try again!']:!netState ? store.loadTranslations['Voice broadcast is temporarily unavailable, please check the network!'] : ''">
                        <svgIcon :class="{ disabled: showStop || recording || !netState||!hasOutput }" style="" v-show="!isActive"
                            icon="play" @click="!showStop && !recording && netState && hasOutput&&playTextAudio()" />
                    </el-tooltip>
                    <svgIcon v-show="isActive" icon="stop" @click="stopPlayTextAudio" />
                </template>
                <svgIcon icon="copy-bubble" v-show="showPlay && !showCopy && (answerNowDisplayContent !== '' || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)"
                    class="copy-btn" @click="copy(copyContent)" />
                <!-- 分割线 -->
                 <div v-show="showLikeOrDislike && showPlay && (answerNowDisplayContent !== '' || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)" style="display: flex;">
                    <div class="split-Icon"></div>
                    <div class="like-dislike">
                        <svgIcon :icon="likeIcon" :class="{ active: likeOrDislike.likeOrNot === 1 }" :style="{marginRight:'10px'}"  @click="clickLike" />
                        <svgIcon :icon="disLikeIcon" :class="{ active: likeOrDislike.likeOrNot === 2 }" @click="clickDislike" />
                    </div>
                 </div>
                
            </div> 
        </div>
        <!-- 重试按钮 -->
        <div v-if="!disabledRetry" :class="{ disabled: disabledRetry || recording, 'enable-backdrop-filter': enableBackdropFilter }" class="retry-btn" v-show="showRetry"
            @click="!disabledRetry && !recording && retryRequest()">
            <svgIcon icon="again" />
            {{store.loadTranslations['Regenerate']}}
        </div>
        <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="disabledRetry ? store.loadTranslations['Answer each question up to 5 times'] : ''">
            <div :class="{ disabledbtn: disabledRetry || recording, 'enable-backdrop-filter': enableBackdropFilter }" class="retry-btn" v-show="showRetry"
                @click="!disabledRetry && !recording && retryRequest()">
                <div class="retry-content">
                    <svgIcon icon="again" />
                    {{store.loadTranslations['Regenerate']}}
                </div>
            </div>
        </el-tooltip>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import { useIntervalFn } from '@vueuse/core'
import ImgBubble from './ImgBubble.vue'
import DocumentParsing from "./DocumentParsing.vue"
import PromptTag from "./PromptTag.vue";
import Preview from "./Preview.vue";
import ToolUseItem from './ToolUseItem.vue';
import _, { forEach } from "lodash";
import { computed, ref } from "vue";
import { conformsTo } from "lodash";

//代码格式化
import { Marked } from 'marked';  
import { markedHighlight } from "marked-highlight"
import hljs from 'highlight.js'
import 'highlight.js/styles/atom-one-dark.css' 
import 'highlight.js/styles/atom-one-light.css'

//是否为深色主题 
const isDarkMode = computed(() => {
    document.getElementsByTagName('html')[0].dataset.codeTheme = store.chatQWeb.themeType == 2 ? 'atom-one-dark' : 'atom-one-light';
    return props.isDarkMode
})

//声明markdown格式化内容
const marked = new Marked(
    markedHighlight({
        langPrefix: 'hljs language-',
        highlight(code, lang) {
            const language = hljs.getLanguage(lang) ? lang : 'shell'
            return hljs.highlight(code, { language }).value
        }
    })
)

// 正则表达式匹配大多数 URL
const urlRegex = /https?:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-z]{2,6}\b(?:[-a-zA-Z0-9@:%_\+.~#?&//=]*)|(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-z]{2,6}\b(?:[-a-zA-Z0-9@:%_\+.~#?&//=]*)/gi;

// 创建一个新的 renderer 实例
const renderer = new marked.Renderer();

// 重写 link 方法
renderer.link = function(tokens) {
    // 使用正则表达式的 exec 方法循环提取所有 URL
    const urls = [];
    const hrefs = [];
    let match;
    while ((match = urlRegex.exec(tokens.href)) !== null) {
        urls.push(match[0].replace("http://",""));
        hrefs.push(match[0].replace("http://","https://"));
    }
    
    if(urls.length == 0){
        return `<a class='marked-url' href="${tokens.href}">${tokens.text}</a>`;
    }

    //URL的位置
    const urlIndex = tokens.text.indexOf(urls[0])
    // 获取关键字前的字符串
    const beforeKeyword = tokens.text.substring(0, urlIndex);
    // 获取关键字后的字符串
    const afterKeyword = tokens.text.substring(urlIndex + urls[0].length);

    return `<span>${beforeKeyword}</span><a class='marked-url' href="${hrefs[0]}">${urls[0]}</a><span>${afterKeyword}</span>`;
};

// 设置 marked 的选项，使用自定义的 renderer
marked.setOptions({
  renderer: renderer
});

// 自定义 marked 的选项
const options = {
  gfm: true, // 启用 GitHub Flavored Markdown
  pedantic: false, // 关闭严格模式
  sanitize: false, // 关闭自动转义
  smartLists: true, // 启用智能列表
  smartypants: false, // 关闭 SmartyPants
  highlight: (code, lang) => {
    // 这里可以自定义代码高亮
    return `<pre><code class="${lang}">${code}</code></pre>`;
  },
  headerIds: true, // 启用自动生成的 id 作为标题的锚点
  headerAnchors: true, // 启用自动生成的 id 作为标题的锚点
  // 其他自定义选项...
};

const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()
const props = defineProps(['item', 'isLast', 'showStop', 'playAudioID', 'recording', 'netState','hasOutput', 'currentAssistant','isKnowledgeBaseExist', 'isEmbeddingPluginsExist', 'showFile', 'fileNameText','filePath', 'imgBase64', 'isLLMExist', 'isDarkMode','isWindowMode','historyIndex'])    //'isKnowledgeBaseExist', 'isEmbeddingPluginsExist'
const emit = defineEmits(['handleShowTip', 'hideTooltip', 'getFreeCredits', 'retryRequest', 'update:playAudioID', 'updateLikeOrDislike'])
const activeIndex = ref(0)
const documentParsingRef = ref()
const disabledRetry = computed(() => props.item.answers && props.item.answers.length === 5)

// 判断是否启用高级CSS特性（如backdrop-filter等）
const enableBackdropFilter = computed(() => store.IsEnableAdvancedCssFeatures)
const errcodeArr = [-9000, -9001, -9003, -9002, -9004, -9100]
const showCopy = computed(() => {
    return errcodeArr.includes(_.last(props.item.answers).errCode)
})

const configMsg = computed ( () => {
    if(props.item.answers[activeIndex.value].errCode <= -9000){
        return JSON.parse(props.item.answers[activeIndex.value].errInfo).info
    }
    return ""
})

const goConfig = async(event) => {
    eval(JSON.parse(props.item.answers[activeIndex.value].errInfo).exec)
    return
}


const goGetFreeCredits = async(event) => {
    emit('getFreeCredits', true)
}

const openUrl = async(event) => {
    let  displayContentObj = []
    try {
        displayContentObj = JSON.parse(props.item.answers[activeIndex.value].displayContent)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }
    const content = displayContentObj.find(obj => obj.chatType === store.ChatAction.ChatTextPlain).content;
    await Qrequest(chatQWeb.openUrl, content)
    return
}

const showRetry = computed(() => {
    return !props.showStop && props.isLast && !errcodeArr.includes(_.last(props.item.answers).errCode) && !showPPTPicture.value && !showPosterPicture.value && _.last(props.item.answers).errCode !== 10001
})
const showPlay = computed(() => {
    if (props.isLast && props.showStop) return false
    return true
})

const copy = (value) => {
    let oInput = document.createElement('textarea');
    oInput.value = value
    if (props.item.answers[activeIndex.value].errCode == 10001) {  //联网搜索
        oInput.value = store.loadTranslations['Search complete.'] + store.loadTranslations['Click to view results']
    }
    
    Qrequest(chatQWeb.copyReplyText, oInput.value);

    emit('handleShowTip', store.loadTranslations['Copied successfully'])
}

const editPPT = (value) => {
    let displayContentObj 
    try {
        displayContentObj = JSON.parse(value)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }
    const content = displayContentObj.find(obj => obj.chatType === store.ChatAction.ChatTextPlain).content;
    Qrequest(chatQWeb.editPPT, content)
}

const genePPT = (value) => {
    let displayContentObj 
    try {
        displayContentObj = JSON.parse(value)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }
    const content = displayContentObj.find(obj => obj.chatType === store.ChatAction.ChatTextPlain).content;
    Qrequest(chatQWeb.sendPPTOutline, content)
}

const editPoster = (value) => {
    let displayContentObj 
    try {
        displayContentObj = JSON.parse(value)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }
    const content = displayContentObj.find(obj => obj.chatType === store.ChatAction.ChatTextPlain).content;
    Qrequest(chatQWeb.editPoster, content)
}

const genePoster = (value) => {
    let displayContentObj 
    try {
        displayContentObj = JSON.parse(value)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }
    const content = displayContentObj.find(obj => obj.chatType === store.ChatAction.ChatTextPlain).content;
    Qrequest(chatQWeb.genePoster, content)
}

const downloadPPT = () => {
    const ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.PictureId) {
            Qrequest(chatQWeb.downloadPPT, element.idValue)
        }
    }
}

const downloadPoster = (posterIndex) => {
    const ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.PictureId) {
            Qrequest(chatQWeb.downloadPoster,element.idValue[posterIndex]);
            break;
        }
    }
}

const handleSwitch = (type) => {
    if (props.showStop || props.recording) return
    if (type === 'prev' && activeIndex.value > 0) activeIndex.value = activeIndex.value - 1
    if (type === 'next' && activeIndex.value < props.item.answers.length - 1) activeIndex.value = activeIndex.value + 1
}

let index = 1
const inconName = ref('yinpin-1')
const { pause, resume, isActive } = useIntervalFn(() => {
    index++;
    if (index > 9) index = 1
    inconName.value = 'yinpin-' + index
}, 300, { immediate: false })

const playTextAudio = async () => {
    const { displayContent, reqId } = props.item.answers[activeIndex.value]
    let displayContentObj = []
    try {
        displayContentObj = JSON.parse(displayContent)
    } catch (error) {
        displayContentObj = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }

    // 新增：拼接所有chatType为ChatTextPlain的content
    let playText = ''
    displayContentObj.forEach(obj => {
        if (obj.chatType === store.ChatAction.ChatTextPlain) {
            playText += obj.content
        }
    })

    if (props.item.answers[activeIndex.value].errCode == 10001) {  //联网搜索
        playText = store.loadTranslations['Search complete.'] + store.loadTranslations['Click to view results']
    }
    
    const res = await Qrequest(chatQWeb.playTextAudio, reqId, playText, true)
    if(res){
        emit('update:playAudioID', reqId)
        resume()
    }
}
const stopPlayTextAudio = async () => {
    await Qrequest(chatQWeb.stopPlayTextAudio)
    emit('update:playAudioID', '')
    pause()
}
watch(() => props.item.answers, (newValue) => {
    if (Array.isArray(newValue)) {
        activeIndex.value = newValue.length - 1
    }
}, { deep: true, immediate: true })
watch(() => props.playAudioID, (newValue) => {
    if (!props.item.answers) return
    const { reqId } = props.item.answers[activeIndex.value]
    if (newValue !== reqId) pause()
})
watch(() => props.currentAssistant, (newValue) => {
    //console.log("props.currentAssistant changed: ", newValue);

    stopPlayTextAudio();
});

/***********************************************************************************
 * new function
 ***********************************************************************************/
//当前用户问题
const showQuestion = ref(false)
const questionDisplayContent = computed(() => {
    if ('question' in props.item){
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return ""
    }
    //history会为无question的记录添加question，要在这里判断
    if ('extention' in props.item.question && props.item.question.extention !== "") {
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return ""
    }
    const ext = JSON.parse(props.item.question.extention)

    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.WordSelectionLable) {
            return element.label + props.item.question.displayContent 
        }
    }

    showQuestion.value = true
    return props.item.question.displayContent
})

const dealOldDisplayContent = (answerIndex) => {
    let ret = []
    
    const { displayContent, chatType, errCode, thinkTime } = answerIndex
    let displayContentNow = displayContent

    if (errCode == 298 && displayContentNow.includes(thinkStart.value) && !displayContentNow.includes(thinkEnd.value)){
        displayContentNow += thinkEnd.value
    }

    const match = displayContentNow.match(regexThink);
    if (match && match[1].length > 0) {
        // 把think部分换成marked
        displayContentNow =  displayContentNow.replace(regexThink, '')

        let chatObj = {
            chatType: store.ChatAction.ChatTextThink,
            content: marked.parse(match[1])
        }

        ret.push(chatObj)
    }

    if (displayContentNow != "") {
        let chatObj = {
            chatType: store.ChatAction.ChatTextPlain,
            content: displayContentNow
        }

        ret.push(chatObj)
    }
    return ret
}

const toolUseContent = ref([])
const allDisplayContent = ref([])  //所有的内容，正文部分被marked处理过
const answerNowDisplayContent = computed(() => {
    allDisplayContent.value = []
    const { displayContent, chatType, errCode, thinkTime } = props.item.answers[activeIndex.value]
    if (chatType === store.ChatAction.ChatText2Image){
        try {
            return JSON.parse(displayContent)
        } catch (error) {
            return displayContent
        }
    }
    toolUseContent.value = []
    if (displayContent === '') {
        return ""
    }

    
    let displayContentNow_ = {}
    try {
        displayContentNow_ = JSON.parse(displayContent)
    } catch (error) {
        displayContentNow_ = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }

    // 在displayContentNow_中找到chatType为store.ChatAction.ChatToolUse的所有item，组成一个新的数组，顺序不变
    toolUseContent.value = displayContentNow_.filter(item => item.chatType === store.ChatAction.ChatToolUse)

    if (displayContentNow_.some(item => item.chatType === store.ChatAction.ChatTextThink)) {  //查找是否有think部分
        isShowThink.value = true
        isThinkContentFound.value = true
        thinkContent.value = displayContentNow_.find(item => item.chatType === store.ChatAction.ChatTextThink).content
        if (errCode == 200) {
            //时间从历史记录中获取
            thinkTitle.value = store.loadTranslations["Deeply thought (%1 seconds)"].replace("%1", thinkTime)
            isThinkComplete.value = true

        } else if (errCode == 298) {
            //如果手动停止，但能找到完整的<think>标签，则显示思考结束+时间，否则显示思考已停止
            if (thinkTime != "-1") {
                thinkTitle.value = store.loadTranslations["Deeply thought (%1 seconds)"].replace("%1", thinkTime)
            } else{
                thinkTitle.value = store.loadTranslations['Thinking has stopped']
            }
            isThinkComplete.value = true
        } else if (errCode == 0) {
            //如果还没停止，且找不到完整的<think>标签，表示正在思考中，
            if (thinkTime != "-1") {
                // thinkTitle.value = await Qrequest(chatQWeb.deepThinkTimeEn, thinkTime)
                thinkTitle.value = store.loadTranslations["Deeply thought (%1 seconds)"].replace("%1", thinkTime)
                isThinkComplete.value = true
            } else{
                thinkTitle.value = store.loadTranslations['Thinking']
                isThinkComplete.value = false
            }        
        }
        thinkContent.value = displayContentNow_.find(item => item.chatType === store.ChatAction.ChatTextThink).content
        thinkContent.value = marked.parse(thinkContent.value)
    }else {
        isShowThink.value = false
        isThinkContentFound.value = false
    }

    // think样式
    if (thinkContent.value.includes('<p>')) {  // 第一段不加margin-top
        thinkContent.value = thinkContent.value.replace(/(?!^)(<p>)/g, "<p style=\"margin-top: 10px;\">")
    } 
   
    if (errCode === 10001) {
        //不走marked的封装
        return ""
    }

    // 新显示逻辑
    allDisplayContent.value = displayContentNow_;
    copyContent.value = ""  //清空复制内容
    let markedHtml = ""
    for (let index = 0; index < allDisplayContent.value.length; index++) {
        if (allDisplayContent.value[index].chatType == store.ChatAction.ChatTextPlain) {
            copyContent.value += allDisplayContent.value[index].content  //填充复制内容
            markedHtml = marked.parse(allDisplayContent.value[index].content)  //把正文部分处理为marked格式

            if (props.item.answers[activeIndex.value].assistantId === 'Poster Assistant') {
                let filteredContent = allDisplayContent.value[index].content.replace(/ID：\d+<br>/, '');//过滤掉ID
                markedHtml = marked.parse(filteredContent)
            }

            // 正文样式
            if (markedHtml.includes('<table>')) {  //table加自定义样式
                markedHtml = markedHtml.replace(/<table>/g, "<table class=\"marked-table\">")
            } 
            if (markedHtml.includes('<pre><code class=\"')){//code添加上下边距，代码背景样式用hljs
                markedHtml = markedHtml.replace(/<pre><code class=\"/g, '<pre style=\"margin-top:20px;margin-bottom: 20px;\"><code class=\"hljs ')
            }

            if (markedHtml.includes('<h1>')){//h1
                markedHtml = markedHtml.replace(/<h1>/g, "<h1 style=\"line-height:45px;\">")
            }

            if (markedHtml.includes('<h2>')){//h2
                markedHtml = markedHtml.replace(/<h2>/g, "<h2 style=\"line-height:45px;\">")
            }

            if (markedHtml.includes('<think>')) {  //think加自定义样式 dark#A6A6A6 light#8B8B8B
                markedHtml = markedHtml.replace(/<think>/g, "<think style=\"color:var(--uosai-color-think);\">")
            } 
            
            if (markedHtml.includes('</think></p>')) {  //think加自定义样式
                markedHtml = markedHtml.replace(/<\/think><\/p>/g, "</p></think>")
            }

            if (markedHtml.includes('<img')) {  //img标签添加最大宽度限制，高度等比缩放
                markedHtml = markedHtml.replace(/<img([^>]*?)>/g, '<img$1 style="max-width: 100%; height: auto;">')
            } 

            // 修改content
            allDisplayContent.value[index].content = markedHtml
        }
    }

    return markedHtml
})

// 复制内容
const copyContent = ref("")

//思考部分显示
const thinkStart = ref("<think>\n\n")
const thinkEnd = ref("\n\n</think>\n\n")
const regexThink = /<think>\n\n(.*?)\n\n<\/think>\n\n/s;
const isShowThink = ref(false)  //是否显示思考部分
const isShowThinkContent = ref(true)  //是否显示思考内容
const thinkTitle = ref('')  //思考标题
const thinkContent = ref('')  //思考内容
const isThinkContentFound = ref(false)  //是否找到思考内容
const isThinkComplete = ref(false)  //思考是否完成
const showThinkContent = () => {
    if (isShowThinkContent.value) {
        isShowThinkContent.value = false
    }else {
        isShowThinkContent.value = true
    }
}

const openImage = (filePath) => {
    Qrequest(chatQWeb.previewImageForPath,filePath)
}

const fileList = computed(() => {
    if ('question' in props.item){
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return []
    }
    if ('extention' in props.item.question && props.item.question.extention !== "") {
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return []
    }
    const ext = JSON.parse(props.item.question.extention)

    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.DocSummary) {
            let fileList = []
            // 循环element.files，将每个文件的metaInfo添加到fileList中
            for (let index = 0; index < element.files.length; index++) {
                const extfile = element.files[index];
                let file = {
                    type: extfile.type,  // 文件类型
                    isExist: false,  
                    index: extfile.index,
                    filePath: extfile.metaInfo.docPath,  // 文件路径
                    fileNameText: extfile.metaInfo.docName,  // 文件名
                    imgBase64:extfile.metaInfo.iconData,  // 文件图标
                    docContent: extfile.content,  // 文件内容
                    isEnabledMouthOver: false,  // 是否启用hover事件
                    isShowParsingStatus: false,  // 是否显示解析状态
                    isParsingStatusEnd: true,  // 是否解析结束
                    parsingStatusText: ""  // 解析状态文本
                }
                
                // 如果是图片类型，检查文件是否存在
                if (file.type === store.DocParsingFileType.Image) {
                    checkFileExist(file)
                }

                fileList.push(file)
            }
            return fileList
        }
    }
    return []
})

// 检查文件是否存在
const fileImageUrls = ref({})
const checkFileExist = async (file) => {
    try {
        file.isExist = await Qrequest(chatQWeb.isFileExist, file.filePath)
        fileImageUrls.value[file.index] = file.isExist ? 'file://' + file.filePath : isDarkMode.value ? 'icons/icon-image-lost-dark.svg' : 'icons/icon-image-lost-light.svg'
    } catch (error) {
        file.isExist = false
    }
}

watch(() => isDarkMode.value, (newValue) => {
    for (const file of fileList.value) {
        if (file.type === store.DocParsingFileType.Image) {
            checkFileExist(file)
        }
    }
}, { immediate: true })

const retryRequest = async () => {
    isShowThink.value = false
    isThinkContentFound.value = false
    isKnowledgeSearchComplete.value = false
    emit('retryRequest')
}

const KnowledgeSearchStatus = computed(() => {  // 是否显示知识库搜索
    return props.item.answers[activeIndex.value].knowledgeSearchStatus  && props.item.answers[activeIndex.value].errCode >= 0
})
const isKnowledgeSearchComplete = ref(false)  // 知识库搜索是否完成
const knowledgeSearchCompleteContent = ref('')  // 知识库搜索完成内容
//预览列表
const preViewList = computed(() => {
    let ext
    try{
        ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    }
    catch(e){
        return JSON.parse("[]")
    }
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.PerView) {
            let searchTime = Math.round(element.searchTime) 
            knowledgeSearchCompleteContent.value = store.loadTranslations["%1 reference documents have been obtained (%2s)"].replace("%1", element.sources.length).replace("%2", searchTime === 0 ? 1 : searchTime)
            isKnowledgeSearchComplete.value = true
            return element.sources
        }
    }
    return JSON.parse("[]")
})

const showPreViewList = computed(() => {
    if(props.showStop && props.isLast){
        return false
    }
    if (preViewList.value.length > 0 && props.item.answers[activeIndex.value].errCode >= 0) return true
    return false
})

const showPPTBtn = computed(() => {
    if(props.showStop && props.isLast){
        return false
    }
    if (props.item.answers[activeIndex.value].assistantId !== 'PPT Assistant') return false
    if (props.item.answers[activeIndex.value].chatType === store.ChatAction.ChatText2Image) return true
    const { displayContent } = props.item.answers[activeIndex.value]
    if (!displayContent.includes("PPT大纲为")) return false
    if (props.item.answers[activeIndex.value].errCode === 200) return true
    return false
})

const showPPTPicture = computed(() => {

    if (showPPTBtn.value && props.item.answers[activeIndex.value].chatType === store.ChatAction.ChatText2Image) {
        return true
    }
    return false;
})

const showPosterBtn = computed(() => {
    if(props.showStop && props.isLast){
        return false
    }
    if (props.item.answers[activeIndex.value].assistantId !== 'Poster Assistant') return false
    if (props.item.answers[activeIndex.value].chatType === store.ChatAction.ChatText2Image) return true
    const { displayContent } = props.item.answers[activeIndex.value]
    if (!displayContent.includes("根据您的描述，为您准备的海报内容如下")) return false
    if (props.item.answers[activeIndex.value].errCode === 200) return true
    return false
})

const showPosterPicture = computed(() => {
    if (showPosterBtn.value && props.item.answers[activeIndex.value].chatType === store.ChatAction.ChatText2Image) {
        return true
    }
    return false;
})

//指令系统

const showPromptTag = computed(() => {
    if ('question' in props.item){
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return false
    }
    if ('extention' in props.item.question && props.item.question.extention !== "") {
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return false
    }
    const ext = JSON.parse(props.item.question.extention)
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.PromptTag) {
            return true
        }
    }
    return false
})
const promptInfo = ref({})
const promptTag = computed(() => {
    if ('question' in props.item){
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return ""
    }
    if ('extention' in props.item.question && props.item.question.extention !== "") {
        showQuestion.value = true
    } else {
        showQuestion.value = false
        return ""
    }
    const ext = JSON.parse(props.item.question.extention)
    promptInfo.value = {}
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.PromptTag) {
            promptInfo.value = element
            return element.tagName
        }
    }
    return ""
})

// 用户输入可复制及二次更改
const questionAction = async (type) => {
    if (props.showStop || props.recording) return  // 回答中或录音中不可操作
    const userInput = {
        questionDisplayContent: questionDisplayContent.value,
        promptInfo: promptInfo.value,
        fileList: fileList.value,
    }
    emit('questionAction', userInput, type)
}

watch(() => props.item, (newValue) => {
})

const likeOrDislikeUpdateHistory = ref(false)

watch(() => props.item.anwsers, (newValue) => {
    if (likeOrDislikeUpdateHistory.value) {
        likeOrDislikeUpdateHistory.value = false
        return
    }
    if (Array.isArray(newValue)) {
        activeIndex.value = newValue.length - 1
    }
}, { deep: true, immediate: true })

const showLikeOrDislike = computed(() => {
    let ext = []
    try{
        ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    }
    catch(e){
        ext = []
    }
    var exists = ext.some(obj => obj.hasOwnProperty("likeOrNot"));  // 检查数组中是否存在某个属性
    return exists
})

const likeOrDislike = computed(() => {
    let ext
    try{
        ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    }
    catch(e){
        return {
            type: store.ExtentionType.LikeOrNot,
            likeOrNot: store.LikeOrNot.EMPTY,
        }
    }
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.LikeOrNot) {
            return element
        }
    }
    return {
        type: store.ExtentionType.LikeOrNot,
        likeOrNot: store.LikeOrNot.EMPTY,
    }
})

onMounted(async () => {
    likeOrDislikeUpdateHistory.value = false
})


/**
 * 点赞、踩、空：1、2，3
 */
 const clickLike = () => {
    let cur = likeOrDislike.value.likeOrNot
    if (cur === store.LikeOrNot.LIKE) {
        cur = store.LikeOrNot.EMPTY
    } else {
        cur = store.LikeOrNot.LIKE
    }

    likeOrDislikeUpdateHistory.value = true
    likeOrDislike.value.likeOrNot = cur

    let ext = []
    try{
        ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    }
    catch(e){
        ext = []
    }

    let rate = {}
    var exists = ext.some(obj => obj.hasOwnProperty("likeOrNot"));  // 检查数组中是否存在某个属性
    if (exists) {
        for (let index = 0; index < ext.length; index++) {
            const element = ext[index];
            if (element.type == store.ExtentionType.LikeOrNot) {
                ext[index].likeOrNot = cur
                rate = ext[index]
                break
            }
        }
    } else {
        ext.push(JSON.parse(JSON.stringify(likeOrDislike.value)))  //不能直接push
    }

    let curActiveIndex = activeIndex.value

    props.item.answers[activeIndex.value].extention = JSON.stringify(ext)

    //修改日志
    emit('updateLikeOrDislike')

    //存数据库
    Qrequest(chatQWeb.rateAnwser, props.historyIndex, activeIndex.value, cur, JSON.stringify(rate))

    nextTick(() => {
        if (curActiveIndex != activeIndex.value) {
            activeIndex.value = curActiveIndex
        }
    })

}

const clickDislike = () => {
    let cur = likeOrDislike.value.likeOrNot
    if (cur === store.LikeOrNot.DISLIKE) {
        cur = store.LikeOrNot.EMPTY
    } else {
        cur = store.LikeOrNot.DISLIKE
    }
    
    likeOrDislikeUpdateHistory.value = true
    likeOrDislike.value.likeOrNot = cur

    let ext = []
    try{
        ext = JSON.parse(props.item.answers[activeIndex.value].extention)
    }
    catch(e){
        ext = []
    }

    let rate = {}
    var exists = ext.some(obj => obj.hasOwnProperty("likeOrNot"));  // 检查数组中是否存在某个属性
    if (exists) {
        for (let index = 0; index < ext.length; index++) {
            const element = ext[index];
            if (element.type == store.ExtentionType.LikeOrNot) {
                ext[index].likeOrNot = cur
                rate = ext[index]
                break
            }
        }
    } else {
        ext.push(JSON.parse(JSON.stringify(likeOrDislike.value)))  //不能直接push
    }

    let curActiveIndex = activeIndex.value

    props.item.answers[activeIndex.value].extention = JSON.stringify(ext)
    emit('updateLikeOrDislike')
    
    //存数据库
    Qrequest(chatQWeb.rateAnwser, props.historyIndex, activeIndex.value, cur, JSON.stringify(rate))

    nextTick(() => {
        if (curActiveIndex != activeIndex.value) {
            activeIndex.value = curActiveIndex
        }
    })
}

//点赞踩状态
const likeIcon = computed(() => {
    if(likeOrDislike.value.likeOrNot == 1){
        return "like-checked"
    }
    if (isDarkMode.value) {
        return "like-dark"
    }
    return "like"
})

const disLikeIcon = computed(() => {
    if(likeOrDislike.value.likeOrNot == 2){
        return "stamp-checked"
    }
    if (isDarkMode.value) {
        return "stamp-dark"
    }
    return "stamp"
})

// 工具调用部分
const copyToolUseItem = (toolCoptContent) => {
    Qrequest(chatQWeb.copyReplyText, toolCoptContent);
}

</script>


<style lang="scss">
    //代码块跟随系统主题切换样式
	@use "sass:meta";

	html[data-code-theme="atom-one-dark"] {
		@include meta.load-css("highlight.js/styles/atom-one-dark.css");
	}
	html[data-code-theme="atom-one-light"] {
		@include meta.load-css("highlight.js/styles/atom-one-light.css");
	}
	
	//自定义table样式
    .marked-table {
        width: 100%;
        border-collapse: collapse;
        margin-top: 20px;
        margin-bottom: 20px;
    }

    .marked-table th, .marked-table td {
        border: 1px solid #ccc;
        padding: 8px;
        text-align:left;
    }

    .marked-table th {
        background-color: #f0f0f0;
        font-weight: bold;
    }

    .marked-url {
        font-size: 0.93rem;
        color: var(--activityColor);
        cursor: pointer;
    }
</style>

<style lang="scss" >
.chat-bubble {
    .item {
        padding: 10px 15px;
        font-size: 0.93rem;
        font-weight: 500;
        font-style: normal;
        max-width: 87.5%;
        width: fit-content;
        position: relative;
        border-radius: 8px;

        pre {
            word-break: break-all;
            white-space: pre-wrap;
            line-height: 25px;
            font-size: 0.93rem;
            font-weight: 500;
            font-style: normal;
            font-family: var(--font-family);
        }

        // .copy-btn {
        //     position: absolute;
        //     bottom: -20px;
        //     right: 15px;
        //     z-index: 9999;
        //     border-radius: 8px;
        //     border: 1px solid rgba(0, 0, 0, 0.05);
        //     box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
        //     background-color: rgba(247, 247, 247);
        //     width: 36px;
        //     height: 36px;
        //     cursor: pointer;
        //     display: flex;
        //     align-items: center;
        //     justify-content: center;
        //     opacity: 0;
        //     transition-property: opacity;
        //     transition-duration: 1s;

        //     svg {
        //         fill: #000;
        //     }
        // }

        // &:hover {
        //     .copy-btn {
        //         opacity: 1;
        //     }
        // }
        .operate-btn {
            min-width: 90px;
            height: 30px;
            background-color: var(--activityColor);
            color: white; /* 白色文字 */
            border: none; /* 无边框 */
            margin-right: 5px;
            margin-top: 20px;
            margin-bottom: 10px;
            border-radius: 8px; /* 四个圆角 */
            cursor: pointer; /* 鼠标悬停时显示指针 */
            transition: background-color 0.3s; /* 背景色渐变效果 */
            outline: none; 
            font-size: 1rem;
            display: inline-flex; /* 设置为弹性盒子 */
            justify-content: center; /* 水平居中 */
            align-items: center; /* 垂直居中 */
        }

        .operate-btn:hover {
            background-color: var(--activityColorHover);
         }

        .play-audio {
            margin-top: 10px;
            display: flex;
            align-items: center;
            width: 100%;

            .icon-play,
            .icon-stop,
            .icon-copy-bubble {
                fill: var(--uosai-color-clear);
                color: var(--uosai-color-clear);
                min-width: 14px;
                max-width: 14px;
                width: 14px;
                height: 14px;
                cursor: pointer;

                &:not(.disabled):hover {
                    fill: var(--uosai-color-clear-hover);
                    color: var(--uosai-color-clear-hover);
                }

                &:not(.disabled):active {
                    fill: var(--activityColor);
                    color: var(--activityColor);
                }
            }

            .play-animation {
                width: 72px;
                height: 20px;
                margin-left: auto;
                margin-right: 10px;

                svg {
                    width: 72px;
                    height: 20px;
                    fill: var(--activityColor);
                    opacity: 1;
                }
            }

            .icon-copy-bubble {
                margin-left: 11px;
            }

            .model {
                color: var(--uosai-color-model-name);
                font-size: 0.85rem;
                font-weight: 500;
                font-style: normal;
                user-select: none;
                display: flex;
                align-items: center;
                width: 50%;

                .name {
                    max-width: 90%;
                    white-space: nowrap;
                    overflow: hidden;
                    text-overflow: ellipsis;
                    width: fit-content;
                }

                img {
                    width: 14px;
                    height: 14px;
                    margin-right: 7px;
                }
            }

            .split-Icon {
                width: 1px;
                height: 12px;
                margin-left: 8px;
                margin-right: 8px;
                background-color: var(--uosai-color-split-Icon-bg);
            }
            .like-dislike {
                display: flex;
                align-items: center;
                // cursor: pointer;

                svg {
                    fill: var(--uosai-color-clear);
                    color: var(--uosai-color-clear);
                    width: 14px;
                    height: 14px;
                    // margin-left: 7px;
                    cursor: pointer;

                    &:not(.disabled):hover {
                        fill: var(--uosai-color-clear-hover);
                        color: var(--uosai-color-clear-hover);
                    }

                    &:not(.disabled):active {
                        fill: var(--activityColor);
                        color: var(--activityColor);
                    }
                }
            }
        }

        .PreViewList{
            margin-top: 20px;
            margin-bottom: 20px;

            .PreViewListLabel{
                display: flex;
                align-items: center;
                font-size: 0.93rem;
                color: var(--uosai-color-Reference);
            }

            .PreViewListContent{
                position: relative;
                margin-top: 10px;

                &::before {
                    // 伪元素，用于绘制竖线
                    content: ""; /* 伪元素需要 content 属性 */
                    position: absolute;
                    left: 0;
                    // top: -5px; /* 向上偏移 5 像素 */
                    width: 2px; /* 竖线的宽度 */
                    background-color: var(--uosai-think-content-side-color-bg); /* 竖线的颜色 */
                    // height: calc(100% + 10px); /* 竖线比文本高度多出 10 像素（上下各 5 像素） */
                    height: 100%;
                }
            }
        }

    }

    .question {
        background-color: var(--activityColor);
        color: rgba(255, 255, 255, 1);
        margin-bottom: 8px;
        margin-left: auto;
        box-shadow: 0px 4px 6px var(--borderColor);

        .prompt-tag{
            margin-right: 10px; 
            display: inline-flex;
            line-height: normal;
            align-content: center;
            text-align: center;
            color: rgba(255, 255, 255, 1); 
            background-color: rgba(255, 255, 255, 0.25);
        }

        ::selection {
            background-color: rgba(255, 255, 255, 0.3);
        }
    }

    .question-actions {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        margin-bottom: 10px;
        margin-left: auto;
        
        .action-btn {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            user-select: none;

            svg {
                width: 16px;
                height: 16px;
                fill: var(--uosai-color-question-action-svg);
            }

            &:not(.disabled):hover {
                width: 20px;
                height: 20px;
                border-radius: 4px;
                background-color: var(--uosai-color-question-action-btn-hover);
                svg {
                    fill: var(--uosai-color-question-action-svg);
                }
            }
            &:not(.disabled):active {
                svg {
                    fill: var(--activityColor);
                }
            }
        }

        .divider {
            width: 1px;
            height: 15px;
            margin: 0 4px;
            background-color: var(--uosai-color-question-action-divider);
        }
    }

    .answers {
        border: 1px solid rgba(0, 0, 0, 0.05);
        box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.08);
        background-color: var(--uosai-color-assistant-bg);
        color: var(--uosai-color-shortcut);
        margin-bottom: 20px;
        margin-right: auto;
        min-width: 200px;

        .think {
            display: block;
            // background-color: #f3f5fc;
            // padding: 8px 12px;
            // box-sizing: border-box;
            // border-radius: 8px;
            width: 100%;

            .think-title {
                // margin-left: 5px;
                display: flex;
                align-items: center;
                user-select: none;
                color: var(--uosai-think-title-color);
            }

            .think-content {
                display: flex;
                margin: 10px 0px;

                // 伪元素竖线
                position: relative; /* 为伪元素提供定位上下文 */
                padding-left: 8px; /* 为竖线留出空间 */

                .think-text {
                    color: var(--uosai-think-content-color);
                    height: calc(100%);
                    width: 100%;
                    line-height: 1.5rem;
                    white-space: normal;
                    word-wrap: break-word;
                }
            }

            .think-content::before {
                // 伪元素，用于绘制竖线
                content: ""; /* 伪元素需要 content 属性 */
                position: absolute;
                left: 0;
                // top: -5px; /* 向上偏移 5 像素 */
                width: 2px; /* 竖线的宽度 */
                background-color: var(--uosai-think-content-side-color-bg); /* 竖线的颜色 */
                // height: calc(100% + 10px); /* 竖线比文本高度多出 10 像素（上下各 5 像素） */
                height: 100%;
            }

        }

        .answerNowDisplayContent{
            white-space: normal; /*css-3*/
            word-wrap: break-word; /*InternetExplorer5.5+*/ 
        }
    }

    .loading {
        width: 22px;
        height: 6px;
        animation-name: loadingChange;
        animation-duration: 1.5s;
        animation-iteration-count: infinite;
    }

    .go-config {
        font-size: 0.93rem;
        color: var(--activityColor);
        cursor: pointer;
    }

    @keyframes loadingChange {
        0% {
            background: url(../../../svg/1-loading.svg) no-repeat 0px center;
        }

        50% {
            background: url(../../../svg/2-loading.svg) no-repeat 0px center;

        }

        100% {
            background: url(../../../svg/3-loading.svg) no-repeat 0px center;

        }
    }

    @keyframes loadingChangedark {
        0% {
            background: url(../../../svg/1-loading-dark.svg) no-repeat 0px center;
        }

        50% {
            background: url(../../../svg/2-loading-dark.svg) no-repeat 0px center;

        }

        100% {
            background: url(../../../svg/3-loading-dark.svg) no-repeat 0px center;

        }
    }

    .documentParsing{
        display: flex;
        top: 0px;
        left: 0px;
        margin-top: 0px;
        margin-bottom: 20px;
        margin-left: auto;
        margin-right: 0px;
        min-width: auto;
        
    }

    .image-container {
        /* 基本布局属性 */
        margin-top: 0px;
        margin-bottom: 20px;
        margin-left: auto;
        margin-right: 0px;
        display: block;
        cursor: pointer;
        
        /* 视窗尺寸约束 - 固定容器大小范围 */
        min-width: 105px;
        max-width: 320px;
        min-height: 124px;
        max-height: 320px;
        
        /* 容器设置 */
        position: relative;
        overflow: hidden;
        border-radius: 8px;
        
        /* 让容器根据内容自适应，但受min/max约束 */
        width: max-content;
        height: max-content;
    }

    .image-file {
        /* 图片基本设置 */
        display: block;
        border-radius: 8px;
        
        /* 核心：图片适应策略 */
        object-fit: cover;
        object-position: top left;
        
        /* 图片尺寸设置 - 关键逻辑 */
        /* 对于超大图片：max-width/max-height限制尺寸，object-fit: cover进行裁剪 */
        /* 对于太小图片：min-width/min-height强制最小尺寸，object-fit: cover进行放大 */
        min-width: 105px;
        min-height: 124px;
        max-width: 320px;
        max-height: 320px;
        
        /* 让图片填充容器 */
        width: 100%;
        height: 100%;
    }

    .retry-btn {
        padding: 6px 15px;
        border-radius: 8px;
        box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.05);
        // border: 1px solid rgba(0, 0, 0, 0.05);
        background-color: var(--uosai-bg-retry);
        width: fit-content;
        color: var(--uosai-color-retry-text);
        font-size: 0.93rem;
        font-weight: 500;
        font-style: normal;
        cursor: pointer;
        user-select: none;
        // margin-top: -10px;
        display: flex;
        align-items: center;
        margin-left: 1px;
        margin-bottom: 5px;
        &:not(.disabledbtn):hover {
            // border: 1px solid rgba(0, 0, 0, 0.05);
            background-color: var(--uosai-bg-retry-hover);
        }

        &:not(.disabledbtn):active {
            box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.1);
            background-color: var(--uosai-bg-retry-active);
            color: var(--activityColor);
            svg {
                fill: var(--activityColor);
            }
        }

        svg {
            fill: var(--uosai-color-retry-text);
            width: 12px;
            height: 14px;
            margin-right: 6px;
        }
        .retry-content {
            display: flex;
            align-items: center;
        }
        &.disabledbtn .retry-content {
            opacity: 0.4;
        }
        &.disabledbtn {
            background-color: var(--uosai-bg-retry);
            border-color: var(--uosai-border-color-retry);
            cursor: not-allowed;
        }
        
        // QT6特定样式
        &.enable-backdrop-filter {
            background-color: var(--uosai-bg-retry-qt6);
            &:not(.disabledbtn):hover {
                background-color: var(--uosai-bg-retry-hover-qt6);
            }
            &:not(.disabledbtn):active {
                color: var(--activityColor);
                background-color: var(--uosai-bg-retry-active-qt6);
                svg {
                    fill: var(--activityColor);
                }
            }
            &.disabledbtn {
                background-color: var(--uosai-bg-retry-qt6);
                border-color: var(--uosai-border-color-retry-qt6);
            }
        }
    }

    .switch-btn {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        color: var(--uosai-color-shortcut);
        font-size: 0.85rem;
        margin-bottom: 9px;
        height: 16px;
        letter-spacing: 2px;
        line-height: 12px;
        padding-right: 10px;

        .btn {
            width: 16px;
            height: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            border-radius: 3px;

            &.disabled {
                opacity: 0.4;
            }

            &:not(.disabled):hover {
                background-color: var(--uosai-color-modelbtn-bg);
            }

            &:not(.disabled):active {
                background-color: var(--uosai-color-modelbtn-hover);

                svg {
                    fill: var(--activityColor);
                }
            }
        }

        .prev {
            margin-right: 5px;
        }

        .next {
            margin-left: 5px;
        }

        svg {
            width: 5px;
            height: 8px;
            fill: var(--uosai-color-shortcut);
        }
    }
}

.dark {
    .loading {
        animation-name: loadingChangedark !important;
    }
    .retry-btn{
        box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.15);
    }

    .marked-table th, .marked-table td {
        border: 1px solid #B6B6B6;
    }
    .marked-table th {
        background-color: #2c2c2c;
    }
}

//加粗处理
strong {
	    font-weight: 800; 
    }
</style>