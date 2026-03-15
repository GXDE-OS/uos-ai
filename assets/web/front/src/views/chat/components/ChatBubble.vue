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
        <FileGroup
            v-if="fileList.length > 0"
            :fileList="fileList"
            :isDarkMode="isDarkMode"
            :isWindowMode="props.isWindowMode"
            @openImage="openImage"
        />
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
        <!-- 任务进度组件 -->
        <TaskProgress v-if="showTaskProgress" style="margin-bottom: 20px;" :task="taskProgressData" :isDarkMode="isDarkMode" @copyToolUseItem="copyToolUseItem" />
        <!-- 应答册气泡 -->
        <div :class="`answers item`" v-show="showAnswer" :style="{ minWidth: answerNowDisplayContent === '' || item.role === 'user' ? 'auto' : '200px', maxWidth: changeMaxWidth ? 'calc(100% - 30px)' : '87.5%', width: changeMaxWidth ? 'calc(100% - 20px)' : 'fit-content' }">
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
            <div class="loading" v-if="showLoading"></div>
            <!-- 文生图显示 -->
            <ImgBubble :isPPTShow="showPPTBtn" :isPosterShow="showPosterBtn" @handleShowTip="emit('handleShowTip', store.loadTranslations['Copied successfully'])" :disabled="recording || !!playAudioID"
                :content="answerNowDisplayContent" @clickOnPPT="downloadPPT" @clickOnEdit="downloadPoster" v-else-if="Array.isArray(answerNowDisplayContent)" :isWindowMode="props.isWindowMode"/>
            <!-- 文本显示 -->
            <div v-else v-for="(item, index) in allDisplayContent" :key="index">
                <!-- 正文 -->
                <pre class="answerNowDisplayContent" :isDarkMode="isDarkMode" v-if="item.chatType == store.ChatAction.ChatTextPlain" v-html="item.content" ref="target"
                    @mouseleave.stop="hideReference"
                    @mousemove.stop="handleReferenceMouseMove"></pre>
                <!-- 引用卡片 -->
                <div v-if="showReferenceCard" class="reference-card-container" ref="referenceCardContainer"
                    :style="{
                        position: 'absolute',
                        left: referencePosition.x + 'px',
                        top: referencePosition.y + 'px',
                        transform: referenceCardTransform, // 动态控制卡片的垂直位置转换
                        zIndex: 1000
                    }">
                    <ReferenceCard
                        :index="currentReferenceIndex"
                        :content="references"
                        @mouseenter="keepReferenceCardVisible"
                        @mouseleave="hideReference" />
                </div>
                <!-- 工具使用状态 -->
                <ToolUseItem v-if="item.chatType == store.ChatAction.ChatToolUse" :toolUseItem="item" :errCode="props.item.answers[activeIndex].errCode" @copyToolUseItem="copyToolUseItem"/>
                <!-- 大纲 -->
                <div v-if="item.chatType == store.ChatAction.ChatOutline" class="outline-container">
                    <Outline :show="item.chatType == store.ChatAction.ChatOutline" :Title="item.content.title" :Paragraphs="item.content.content" @updateOutline="updateOutline" :disabled="!props.isLast"/>
                </div>
                <!-- 文档卡片 -->
                <ContentCard v-if="item.chatType == store.ChatAction.ChatDocCard" :id="item.content.id" :title="item.content.title" :content="item.content.content" @openMarkdownEditor="openMarkdownEditor"/>
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
            <div class="play-audio" v-show="showBottomBtn || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)" >
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
                <template v-if="showPlay && (showBottomBtn || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)">
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
                <svgIcon icon="copy-bubble" v-show="showPlay && !showCopy && (showBottomBtn || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)"
                    class="copy-btn" @click="copy(copyContent)" />
                <!-- 分割线 -->
                 <div v-show="showLikeOrDislike && showPlay && (showBottomBtn || (answerNowDisplayContent === '' && item.answers[activeIndex].errCode == 10001)) && !Array.isArray(answerNowDisplayContent)" style="display: flex;">
                    <div class="split-Icon"></div>
                    <div class="like-dislike">
                        <svgIcon :icon="likeIcon" :class="{ active: likeOrDislike.likeOrNot === 1 }" :style="{marginRight:'10px'}"  @click="clickLike" />
                        <svgIcon :icon="disLikeIcon" :class="{ active: likeOrDislike.likeOrNot === 2 }" @click="clickDislike" />
                    </div>
                 </div>
                
            </div> 
        </div>
        <!-- 猜你想要 -->
        <GuessYouWant :show="guessYouWantLists.length > 0 && showRetry" :GuessYouWantLists="guessYouWantLists" @guessYouWantClick="handleGuessYouWantClick"/>
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
        <!-- 基于大纲生成内容按钮容器 -->
        <div v-if="showGenContentBtn" class="gen-content-btn-container" ref="genContentBtnContainerRef">
            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                :show-after="1000" :offset="2" :content="store.loadTranslations['Outline to Docs']">
                <div class="base-outline-gen-content-btn" @click="handleGenContent"
                    :class="{ 'disabled': (recording || !props.isLast || outlineIsEmpty) }"
                    :style="{ transform: `translateY(${genContentBtnOffset}px)` }">
                    <svgIcon icon="generate"/>
                    <div class="base-outline-gen-content-btn-text">
                        {{ store.loadTranslations['Outline to Docs']}}
                    </div>
                </div>
            </el-tooltip>
        </div>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import { useIntervalFn } from '@vueuse/core'
import ImgBubble from './ImgBubble.vue'
import FileGroup from "./FileGroup.vue"
import PromptTag from "./PromptTag.vue";
import Preview from "./Preview.vue";
import ToolUseItem from './ToolUseItem.vue';
import TaskProgress from './TaskProgress.vue'; // 添加TaskProgress组件导入
import _, { forEach, max } from "lodash";
import { computed, ref, onMounted, onUnmounted, nextTick, watch } from "vue";
import { conformsTo } from "lodash";

//代码格式化
import { Marked } from 'marked';  
import { markedHighlight } from "marked-highlight"
import hljs from 'highlight.js'
import 'highlight.js/styles/atom-one-dark.css' 
import 'highlight.js/styles/atom-one-light.css'

// 大纲组件
import Outline from "./Outline/Outline.vue";
import ContentCard from "./Outline/ContentCard.vue";
// 猜你想要组件
import GuessYouWant from "./Outline/GuessYouWant.vue";
// 引用卡片组件
import ReferenceCard from "./Outline/ReferenceCard.vue";



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
const emit = defineEmits(['handleShowTip', 'hideTooltip', 'getFreeCredits', 'retryRequest', 'update:playAudioID', 'updateLikeOrDislike', 'updateOutline', 'updateActiveIndex', 'genContentFromOutline', 'openMarkdownEditor'])
const activeIndex = ref(0)
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
    emit("updateActiveIndex", activeIndex.value)
    Qrequest(chatQWeb.updateAnswresActiveIndex , activeIndex.value);
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
    let isPlayOutline = false
    displayContentObj.forEach(obj => {
        if (obj.chatType === store.ChatAction.ChatTextPlain) {
            playText += obj.content
        }
        if (obj.chatType === store.ChatAction.ChatOutline) {
            playText += outlineCopy.value
            isPlayOutline = true
        }
    })

    if (props.item.answers[activeIndex.value].errCode == 10001) {  //联网搜索
        playText = store.loadTranslations['Search complete.'] + store.loadTranslations['Click to view results']
    }
    
    const res = await Qrequest(chatQWeb.playTextAudio, reqId, playText, true, isPlayOutline)
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
        Qrequest(chatQWeb.updateAnswresActiveIndex , activeIndex.value);
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
const guessYouWantLists = ref([])
const referenceRegex = /<reference>(.*?)<\/reference>/gs  // 匹配引用部分的正则表达式，添加s flag使.匹配换行
const references = ref([])  // 所有引用部分的数组
const showAnswer = ref(true)  // 是否显示应答册气泡
const answerNowDisplayContent = ref("")
// 添加任务进度相关的响应式变量
const showTaskProgress = ref(false);
const taskProgressData = ref({});
const isShowThink = ref(false)  //是否显示思考部分
const isShowThinkContent = ref(true)  //是否显示思考内容
const thinkStart = ref("<think>\n\n")
const thinkEnd = ref("\n\n</think>\n\n")
const regexThink = /<think>\n\n(.*?)\n\n<\/think>\n\n/s;
const thinkTitle = ref('')  //思考标题
const thinkContent = ref('')  //思考内容
const isThinkContentFound = ref(false)  //是否找到思考内容
const isThinkComplete = ref(false)  //思考是否完成
// 复制内容
const copyContent = ref("")

//思考部分显示
const showThinkContent = () => {
    if (isShowThinkContent.value) {
        isShowThinkContent.value = false
    }else {
        isShowThinkContent.value = true
    }
}

const changeMaxWidth = computed(() => {
    return allDisplayContent.value.some(item => item.chatType === store.ChatAction.AgentReasonTitle) 
})

// 更新大纲
const outlineCopy = ref("")
const outlineIsEmpty = ref(false)
const updateOutline = (newTitle, newParagraph) => {
    if (newTitle === "" && newParagraph.length === 0) {
        outlineIsEmpty.value = true
    } else {
        outlineIsEmpty.value = false
    }
    emit('updateOutline', activeIndex.value, newTitle, newParagraph)
    // 大纲更新后重新计算按钮位置
    nextTick(() => {
        calculateBtnPosition()
    })
}

// 显示生成内容按钮
const showGenContentBtn = computed(() => {
    // 只在最后一条消息且是AI写作助手时显示
    return props.isLast &&
           props.currentAssistant?.type === store.AssistantType.AI_WRITING_ASSISTANT &&
           allDisplayContent.value.some(item => item.chatType === store.ChatAction.ChatOutline)
})

// 处理生成内容按钮点击
const handleGenContent = () => {
    if (props.recording || !props.isLast || outlineIsEmpty.value) return
    emit('genContentFromOutline')
}

// 按钮容器引用和偏移量
const genContentBtnContainerRef = ref(null)
const genContentBtnOffset = ref(0)

// 计算按钮位置，根据滚动情况调整偏移量
const calculateBtnPosition = () => {
    if (!showGenContentBtn.value || !genContentBtnContainerRef.value) return

    const chatHistory = document.getElementById('chatHistory')
    if (!chatHistory) return

    const chatHistoryRect = chatHistory.getBoundingClientRect()
    const containerRect = genContentBtnContainerRef.value.getBoundingClientRect()

    const buttonHeight = 36 // 按钮高度
    const bottomMargin = 1 // 距离视口底部的最小间距
    const viewportBottom = chatHistoryRect.bottom

    // 容器顶部相对于可视区域的位置
    const containerTop = containerRect.top

    // 按钮在自然位置（容器顶部）时的底部位置
    const naturalBottom = containerTop + buttonHeight

    // 计算需要的偏移量
    if (naturalBottom + bottomMargin > viewportBottom) {
        // 按钮在自然位置会超出可视区域，需要向上偏移
        const overflow = naturalBottom + bottomMargin - viewportBottom
        genContentBtnOffset.value = -overflow
    } else if (containerTop < chatHistoryRect.top) {
        // 容器顶部在可视区域之上，按钮需要向下偏移以保持在可视区域顶部附近
        const minTop = chatHistoryRect.top + bottomMargin
        const targetTop = Math.max(minTop, viewportBottom - buttonHeight - bottomMargin)
        genContentBtnOffset.value = targetTop - containerTop
    } else {
        // 容器在可视区域内，按钮保持在容器顶部（无偏移）
        genContentBtnOffset.value = 0
    }
}

// 当显示状态改变时重新计算位置
watch(showGenContentBtn, (newVal) => {
    if (newVal) {
        nextTick(() => {
            calculateBtnPosition()
        })
    }
})

// 监听大纲内容变化（如展开/折叠、编辑、添加/删除等），重新计算按钮位置
watch(() => allDisplayContent.value, (newVal, oldVal) => {
    if (showGenContentBtn.value) {
        // 检查是否有大纲类型的内容
        const hasOutline = newVal.some(item => item.chatType === store.ChatAction.ChatOutline)
        if (hasOutline) {
            // 检查大纲内容是否确实发生了变化
            const oldOutline = oldVal?.find(item => item.chatType === store.ChatAction.ChatOutline)?.content
            const newOutline = newVal.find(item => item.chatType === store.ChatAction.ChatOutline)?.content
            if (JSON.stringify(oldOutline) !== JSON.stringify(newOutline)) {
                // 大纲内容变化，重新计算按钮位置
                nextTick(() => {
                    calculateBtnPosition()
                })
            }
        }
    }
}, { deep: true })

// 猜你想要点击
const handleGuessYouWantClick = (item) => {
    emit('guessYouWantClick', item)
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

    // 查找WritingResource扩展信息(用于AI写作智能体)
    for (let index = 0; index < ext.length; index++) {
        const element = ext[index];
        if (element.type == store.ExtentionType.WritingResource) {
            return element.files
        }
    }

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

                fileList.push(file)
            }
            return fileList
        }
    }
    return []
})

const retryRequest = async () => {
    isShowThink.value = false
    isThinkContentFound.value = false
    isKnowledgeSearchComplete.value = false
    showAnswer.value = true
    // 以下状态重置
    allDisplayContent.value = []
    guessYouWantLists.value = []
    references.value = []
    showTaskProgress.value = false
    answerNowDisplayContent.value = ""
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

const likeOrDislikeUpdateHistory = ref(false)

watch(() => props.item.anwsers, (newValue) => {
    if (likeOrDislikeUpdateHistory.value) {
        likeOrDislikeUpdateHistory.value = false
        return
    }
    if (Array.isArray(newValue)) {
        activeIndex.value = newValue.length - 1
        Qrequest(chatQWeb.updateAnswresActiveIndex , activeIndex.value);
        
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

// 滚动事件处理器
let scrollHandler = null
// ResizeObserver 用于监听大纲容器尺寸变化
let outlineResizeObserver = null

onMounted(async () => {
    likeOrDislikeUpdateHistory.value = false

    // 初始化滚动监听
    const chatHistory = document.getElementById('chatHistory')
    if (chatHistory) {
        scrollHandler = () => {
            calculateBtnPosition()
        }
        chatHistory.addEventListener('scroll', scrollHandler)
    }

    // 使用 ResizeObserver 监听大纲容器尺寸变化
    outlineResizeObserver = new ResizeObserver(() => {
        if (showGenContentBtn.value) {
            calculateBtnPosition()
        }
    })

    // 监听大纲容器的尺寸变化
    nextTick(() => {
        const outlineContainers = document.querySelectorAll('.outline-container')
        outlineContainers.forEach(container => {
            outlineResizeObserver.observe(container)
        })
    })

    // 初始计算按钮位置
    nextTick(() => {
        calculateBtnPosition()
    })
})

onUnmounted(() => {
    // 清理滚动监听
    const chatHistory = document.getElementById('chatHistory')
    if (chatHistory && scrollHandler) {
        chatHistory.removeEventListener('scroll', scrollHandler)
    }

    // 清理 ResizeObserver
    if (outlineResizeObserver) {
        outlineResizeObserver.disconnect()
    }
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
            Qrequest(chatQWeb.updateAnswresActiveIndex , activeIndex.value);
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
            Qrequest(chatQWeb.updateAnswresActiveIndex , activeIndex.value);
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
// 引用卡片显示控制
const showReferenceCard = ref(false);
const currentReferenceIndex = ref(0);
const referencePosition = ref({ x: 0, y: 0 });
const referenceCardContainer = ref([]); // 多个元素使用ref，会自动变为数组
const referenceCardTransform = ref('translateY(-100%)'); // 默认：将卡片定位在引用上方
let hideTimeout = null;

// 获取.chat-history容器的DOM元素
const getChatHistoryContainer = () => {
    return document.querySelector('.chat-history');
};

const handleReferenceMouseMove = (e) => {
    // 检查鼠标是否悬停在引用元素上
    const target = e.target.closest('.reference-sup');
    if (target) {
        // 鼠标在引用标记上，显示并更新卡片
        const refIndex = parseInt(target.getAttribute('data-ref-index'));
        currentReferenceIndex.value = refIndex;
        showReferenceCard.value = true;
        
        // 假设ReferenceCard有固定的尺寸: 360px 宽度，140px 高度
        // (来自ReferenceCard.vue: width: 360px; height: 140px;)
        const cardWidth = 360;
        const cardHeight = 140;
        
        // 1. 计算X轴位置
        let x = target.offsetLeft;  // 与引用元素的左边缘对齐
        
        // 检查卡片是否会超出容器的右边界
        // 获取最近的容器元素(.answers)来计算宽度
        const answersContainer = target.closest('.answers');
        if (answersContainer) {
            const containerWidth = answersContainer.offsetWidth;
            if (x + cardWidth > containerWidth) {
                x = containerWidth - cardWidth - 12; // 将卡片的右边缘与容器的右边缘对齐，距离右边界12px
            }
        }

        // 2. 计算Y轴位置和变换效果
        // 获取.chat-history容器
        const chatHistoryContainer = getChatHistoryContainer();
        
        if (chatHistoryContainer) {
            // 获取目标元素在.chat-history容器内的位置
            const chatHistoryRect = chatHistoryContainer.getBoundingClientRect();
            const targetRect = target.getBoundingClientRect();
            
            // 计算目标元素在.chat-history容器内的相对top位置
            const targetTopInChatHistory = targetRect.top - chatHistoryRect.top + chatHistoryContainer.scrollTop;
            
            // 检查卡片定位在引用上方时是否会超出chat-history容器
            const spaceAboveInChatHistory = targetTopInChatHistory - 5;
            
            if (spaceAboveInChatHistory >= cardHeight) {
                // 上方有足够的空间，将卡片定位在引用上方5px处，并使用 translateY(-100%)
                referencePosition.value = {
                    x,
                    y: target.offsetTop - 5
                };
                referenceCardTransform.value = 'translateY(-100%)';
            } else {
                // 上方没有足够的空间，将卡片定位在引用下方5px处，不使用变换
                referencePosition.value = {
                    x,
                    y: target.offsetTop + target.offsetHeight 
                };
                referenceCardTransform.value = 'translateY(0)';
            }
        } else {
            // 如果没有找到.chat-history容器，使用默认逻辑
            const spaceAboveCard = target.offsetTop - 5;
            if (spaceAboveCard >= cardHeight) {
                referencePosition.value = { x, y: target.offsetTop - 5 };
                referenceCardTransform.value = 'translateY(-100%)';
            } else {
                referencePosition.value = { x, y: target.offsetTop + target.offsetHeight };
                referenceCardTransform.value = 'translateY(0)';
            }
        }
        // 清除任何隐藏计时器
        if (hideTimeout) {
            clearTimeout(hideTimeout);
            hideTimeout = null;
        }
    } else {
        // 鼠标不在引用标记上，检查是否在卡片容器上
        if (showReferenceCard.value) {
            // 找到当前可见的卡片容器（offsetParent为null表示不可见）
            const visibleCardElements = referenceCardContainer.value.filter(el => el && el.offsetParent !== null);
            const cardElement = visibleCardElements.length > 0 ? visibleCardElements[0] : null;
            
            if (cardElement) {
                const cardRect = cardElement.getBoundingClientRect();
                const mouseX = e.clientX;
                const mouseY = e.clientY;
                
                // 检查鼠标是否在卡片容器内，增加10像素的缓冲区域
                const buffer = 10;
                const isInsideCard =
                    mouseX >= cardRect.left - buffer &&
                    mouseX <= cardRect.right + buffer &&
                    mouseY >= cardRect.top - buffer &&
                    mouseY <= cardRect.bottom + buffer;
                
                // 如果不在卡片容器内，开始隐藏计时器
                if (!isInsideCard) {
                    hideReference();
                }
            }
        }
    }
};


const showReference = (index) => {
    // This is called from ReferenceCard to keep it visible when hovering over the card
    currentReferenceIndex.value = index;
    showReferenceCard.value = true;
};


const hideReference = () => {
    // Delay hiding the card to give time for the mouse to move from reference to card
    hideTimeout = setTimeout(() => {
        showReferenceCard.value = false;
    }, 50);
};

// Update ReferenceCard mouse enter to clear hide timeout
const keepReferenceCardVisible = () => {
    if (hideTimeout) {
        clearTimeout(hideTimeout);
        hideTimeout = null;
    }
    // 确保卡片保持可见状态
    showReferenceCard.value = true;
};

const showLoading = computed(() => {
    return answerNowDisplayContent.value === '' && props.showStop && props.isLast && props.item.answers[activeIndex.value].displayContent === '' && ((KnowledgeSearchStatus.value && isKnowledgeSearchComplete.value) || !KnowledgeSearchStatus.value)
})
watch (() => props.item, (newValue, oldValue) => {
    dealAnswerContent()
}, {deep: true, immediate: true})

watch(() => props.item.answers[activeIndex.value], (newValue) => {
    dealAnswerContent()
}, {deep: true, immediate: true})


function dealAnswerContent() {
    allDisplayContent.value = []
    guessYouWantLists.value = []
    references.value = []
    outlineCopy.value = ""
    showTaskProgress.value = false
    answerNowDisplayContent.value = ""
    isShowThink.value = false
    isThinkContentFound.value = false
    isKnowledgeSearchComplete.value = false
    const {
        displayContent = '',
        chatType       = '',
        errCode        = 0,
        thinkTime      = 0
    } = props.item?.answers?.[activeIndex.value] ?? {}; // 如果props.item.answers[activeIndex.value]为undefined，则使用默认值

    if (chatType === store.ChatAction.ChatText2Image){
        try {
            answerNowDisplayContent.value = JSON.parse(displayContent)
            return
        } catch (error) {
            answerNowDisplayContent.value = displayContent
            return
        }
    }
    
    if (displayContent === '') {
        answerNowDisplayContent.value = ""
        return
    }

    let displayContentNow_ = {}
    try {
        displayContentNow_ = JSON.parse(displayContent)
    } catch (error) {
        displayContentNow_ = dealOldDisplayContent(props.item.answers[activeIndex.value])
    }

    // 在displayContentNow_中找到chatType为store.ChatAction.ChatToolUse的所有item，组成一个新的数组，顺序不变
    toolUseContent.value = displayContentNow_.filter(item => item.chatType === store.ChatAction.ChatToolUse)

    // 检查是否有AgentReasonTitle、AgentReasoning或AgentAction类型的内容，如果有则更新TaskProgress数据
    const hasAgentContent = displayContentNow_.some(item =>
        item.chatType === store.ChatAction.AgentReasonTitle ||
        item.chatType === store.ChatAction.AgentReasoning ||
        item.chatType === store.ChatAction.AgentAction
    );

    if (hasAgentContent) {
        // 过滤出AgentReasoning和AgentAction类型的内容
        const agentContent = displayContentNow_.filter(item =>
            item.chatType === store.ChatAction.AgentReasoning ||
            item.chatType === store.ChatAction.AgentAction
        );

         // 格式化AgentReasoning和AgentAction数据，确保与默认值格式一致
        const formattedAgentContent = agentContent.map(item => {
            if (item.chatType === store.ChatAction.AgentReasoning) {
                // AgentReasoning类型只需要chatType和content字段
                return {
                    chatType: item.chatType,
                    content: item.content,
                };
            } else if (item.chatType === store.ChatAction.AgentAction) {
                // AgentAction类型需要确保包含所有必要字段
                return {
                    chatType: item.chatType,
                    content: item.content,
                    index: item.index,
                    name: item.name || "",
                    params: item.params || "",
                    result: item.result || "",
                    status: item.status !== undefined ? item.status : (errCode === 0 ? 0 : 1)
                };
            }
            return item;
        });
        
        // 找到最后一个chatType为store.ChatAction.AgentReasonTitle的item，将其content赋给formattedAgentName
        const agentReasonTitleItems = displayContentNow_.filter(item =>
            item.chatType === store.ChatAction.AgentReasonTitle
        );
        
        let formattedAgentName = errCode === 0 ? store.loadTranslations["Collecting and analyzing data"] : store.loadTranslations["Data collection and analysis completed"];
        let formattedAgentFailed = 0
        let formattedAgentActive = errCode === 0
        let formattedAgentCompleted = errCode === 200
        if (agentReasonTitleItems.length > 0) {
            const lastAgentReasonTitle = agentReasonTitleItems[agentReasonTitleItems.length - 1];
            if (lastAgentReasonTitle.content ===  "" && agentReasonTitleItems.length > 1) {
                // 从后往前遍历数组，找到第一个不为空的content
                for (let i = agentReasonTitleItems.length - 2; i >= 0; i--) {
                    if (agentReasonTitleItems[i].content && agentReasonTitleItems[i].content !== "") {
                        formattedAgentName = agentReasonTitleItems[i].content;
                            break;
                    }
                 }
            } else {
                formattedAgentName = lastAgentReasonTitle.content;
            }
            formattedAgentFailed = lastAgentReasonTitle.status === store.TitleStatus.Failed;
            formattedAgentActive = lastAgentReasonTitle.status === store.TitleStatus.InProgress;
            formattedAgentCompleted = lastAgentReasonTitle.status === store.TitleStatus.Completed;
        } 
        // 更新TaskProgress数据
        taskProgressData.value = {
            name: formattedAgentName,
            isActive: formattedAgentActive, // 如果还在进行中则显示为活跃状态
            isCompleted: formattedAgentCompleted, // 如果已完成则显示为完成状态
            isFailed: formattedAgentFailed,
            displayContent: formattedAgentContent,
            errCode: errCode
        };
        
        // 确保显示TaskProgress组件
        showTaskProgress.value = true;
    }

    // 检查是否所有item的chatType都是Agent相关类型，如果是则返回空字符串
    const allAgentTypes = [store.ChatAction.AgentReasonTitle, store.ChatAction.AgentReasoning, store.ChatAction.AgentAction];
    const onlyHasAgentTypes = displayContentNow_.every(item => allAgentTypes.includes(item.chatType));
    if (onlyHasAgentTypes) {
        showAnswer.value = false;
    } else {
        // 过滤掉allAgentTypes相关的元素
        const filteredContent = displayContentNow_.filter(item => !allAgentTypes.includes(item.chatType));
        
        // 检查剩下的元素是否只有ChatTextPlain一种类型
        const onlyHasChatTextPlain = filteredContent.every(item => item.chatType === store.ChatAction.ChatTextPlain);
        
        if (onlyHasChatTextPlain) {
            // 检查ChatTextPlain类型的content是否为空字符串
            const chatTextPlainItems = filteredContent.filter(item => item.chatType === store.ChatAction.ChatTextPlain);
            const hasEmptyContent = chatTextPlainItems.some(item => item.content === '');
            
            if (hasEmptyContent && chatTextPlainItems.length === filteredContent.length) {
                showAnswer.value = false;
            } else {
                showAnswer.value = true;
            }
        } else {
            showAnswer.value = true;
        }
    }

    if (displayContentNow_.some(item => item.chatType === store.ChatAction.ChatTextThink)) {  //查找是否有think部分
        isShowThink.value = true
        isThinkContentFound.value = true
        thinkContent.value = displayContentNow_.find(item => item.chatType === store.ChatAction.ChatTextThink).content
        if (errCode == 200) {
            let thinkTimeTmp = thinkTime
            const thinkTimeNum = Number(thinkTimeTmp)
            if (thinkTimeNum < 0 || isNaN(thinkTimeNum)) {
                thinkTimeTmp = "1"
            }
            //时间从历史记录中获取
            thinkTitle.value = store.loadTranslations["Deeply thought (%1 seconds)"].replace("%1", thinkTimeTmp)
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
        answerNowDisplayContent.value = ""
        return
    }

    // 新显示逻辑
    allDisplayContent.value = displayContentNow_;
    copyContent.value = ""  //清空复制内容
    let markedHtml = ""
    for (let index = 0; index < allDisplayContent.value.length; index++) {
        // 跳过AgentReasonTitle、AgentReasoning和AgentAction类型的内容，这些内容由TaskProgress组件处理
        if (allDisplayContent.value[index].chatType === store.ChatAction.AgentReasonTitle ||
            allDisplayContent.value[index].chatType === store.ChatAction.AgentReasoning ||
            allDisplayContent.value[index].chatType === store.ChatAction.AgentAction) {
            continue;
        }

        if (allDisplayContent.value[index].chatType == store.ChatAction.ChatTextPlain) {
            // 去除引用部分后再添加到复制内容
            copyContent.value += allDisplayContent.value[index].content.replace(referenceRegex, '');  //填充复制内容

            /**
             * 处理引用部分
             */
            // 1.找到引用部分，替换为①②③...
            let content = allDisplayContent.value[index].content;
            // const referencesList = [];
            
            // 使用正则表达式匹配所有引用
            let match;
            let counter = 1;
            
            // 重置正则表达式的lastIndex
            referenceRegex.lastIndex = 0;
            
            while ((match = referenceRegex.exec(content)) !== null) {
                // 将引用内容添加到数组
                references.value.push({
                    index: counter,
                    content: match[1]
                });

                
                
                counter++;
            }

            // 将所有引用替换为对应的序号
            // ${String.fromCharCode(0x2460 + referencesList.findIndex(r => r.content === p1))}
            content = content.replace(referenceRegex, (match, p1) => {
                const refIndex = references.value.findIndex(r => r.content === p1) + 1;
                
                return `<sup class="reference-sup" data-ref-index="${refIndex}"
                            style="cursor: pointer; color: var(--uosai-color-flat-btn-icon);">
                        <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="16" height="17">
                            <path  transform="matrix(0.707107 -0.707107 0.707107 0.707107 0.0251265 11.1317)" d="M11.5 9.094947e-13C13.432997 9.094947e-13 15 1.5670034 15 3.5C15 5.4329967 13.432997 7 11.5 7L9.5 7C7.5670033 7 6 5.4329967 6 3.5C6 3.2238576 6.2238574 3 6.5 3C6.7761426 3 7 3.2238576 7 3.5C7 4.880712 8.1192884 6 9.5 6L11.5 6C12.880712 6 14 4.880712 14 3.5C14 2.1192882 12.880712 1 11.5 1L9.5 1C9.2238579 1 9 0.77614236 9 0.5C9 0.22385763 9.2238579 9.094947e-13 9.5 9.094947e-13L11.5 9.094947e-13ZM5.5 0C7.4329967 0 9 1.5670034 9 3.5C9 3.7761424 8.7761421 4 8.5 4C8.2238579 4 8 3.7761424 8 3.5C8 2.1192882 6.880712 1 5.5 1L3.5 1C2.1192882 1 1 2.1192882 1 3.5C1 4.880712 2.1192882 6 3.5 6L5.5 6C5.7761426 6 6 6.2238574 6 6.5C6 6.7761426 5.7761426 7 5.5 7L3.5 7C1.5670034 7 -4.5474735e-13 5.4329967 -4.5474735e-13 3.5C-4.5474735e-13 1.5670034 1.5670034 0 3.5 0L5.5 0Z"/>
                        </svg>
                            <path fill-rule="evenodd" d="M4 9H3V8h1v1zm0-3H3v1h1V6zm0-2H3v1h1V4zm0-2H3v1h1V2zm8 8h-1v-1h1v1zm-2-2H9v1h1v-1zm-2-2H7v1h1V6zm-2-2H5v1h1V4zm6-2v1h1V2h-1zm-8 8V7h1v3H2zm0-4V3h1v3H2zm10 6v-3h1v3h-1zm-2-4H9v3h1V7zm-2-4H7v3h1V3zm2 0h1v3h-1V3zm0 8h1v3h-1v-3zM4 2v1h1V2H4zm10 10v1h1v-1h-1zM2 12v1h1v-1H2zm0-4v1h1V8H2zm12 4v1h1v-1h-1zm-10 2h1v1H2v-1zm0-6h1v1H2V6z"/>
                        </svg>
                    </sup>`;
            });
            markedHtml = marked.parse(content)  //把正文部分处理为marked格式


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

        if (allDisplayContent.value[index].chatType === store.ChatAction.ChatOutline) {
            try {
                allDisplayContent.value[index].content = JSON.parse(allDisplayContent.value[index].content)
                // 判断content是否为空对象或者不包含title和content字段
                const keys = Object.keys(allDisplayContent.value[index].content)
                if (keys.length === 0 || !keys.includes('title') || !keys.includes('content')) {
                    outlineIsEmpty.value = true
                    allDisplayContent.value[index].content = {
                        title: "",
                        content: []
                    }
                }
            } catch (error) {
                // content不是json格式，或者json格式错误，都设置为空对象
                outlineIsEmpty.value = true
                allDisplayContent.value[index].content = {
                    title: "",
                    content: []
                }
            }
            
            // 构建大纲文本内容
            let outlineText = ""
            const outlineData = allDisplayContent.value[index].content.content
            
            outlineCopy.value = "# " + allDisplayContent.value[index].content.title + "\n"
            // 遍历外部数组中的每个元素
            for (const item of outlineData) {
                // 添加外部标题（二级标题）
                outlineText += "## " + item.title + "\n"
                
                // 遍历内部content数组，添加子标题（三级标题）
                if (item.content && Array.isArray(item.content)) {
                    for (const subItem of item.content) {
                        if (subItem.title) {
                            outlineText += "### " + subItem.title + "\n"
                        }
                    }
                }
                outlineText += "\n" // 在每个外部元素之间添加空行
            }
            
            outlineCopy.value += outlineText
            copyContent.value += outlineCopy.value
        }

        // 猜你想要
        if (allDisplayContent.value[index].chatType === store.ChatAction.ChatGuessYouWant) {
            guessYouWantLists.value = allDisplayContent.value[index].content
        }
    }

    answerNowDisplayContent.value = markedHtml
}

const showBottomBtn = computed(() => {
    let show = false
    for (let index = 0; index < allDisplayContent.value.length; index++) {
        if ((allDisplayContent.value[index].content !== '' && allDisplayContent.value[index].chatType === store.ChatAction.ChatTextPlain) 
        || (allDisplayContent.value[index].chatType === store.ChatAction.ChatOutline) 
        || (allDisplayContent.value[index].chatType === store.ChatAction.ChatDocCard)) {
            show = true
        }
    }
    return show
})

const openMarkdownEditor = (mdEditorContent) => {
    emit('openMarkdownEditor', mdEditorContent)
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
    
    // 引用样式
    .reference-sup {
        cursor: pointer;
        color: var(--uosai-color-flat-btn-icon);
        margin-bottom: 1px;
        position: relative;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        vertical-align: middle;
        width: 20px;
        height: 20px;
        border-radius: 4px;

        svg {
            width: 16px;
            height: 16px;
            fill: var(--uosai-color-flat-btn-icon);
        }

        &:hover {
            width: 20px;
            height: 20px;
            background-color: var(--uosai-color-flat-btn-bg-hover);
        }
    }
    
    // 引用卡片容器
    .reference-card-container {
        position: absolute;
        top: 5px;
        transform: translateY(-100%);
        z-index: 1000;
    }

    // 大纲容器
    .outline-container {
        width: 100%;
    }

    // 基于大纲生成内容按钮容器
    .gen-content-btn-container {
        position: relative;
        width: 100%;
        height: 46px; // 按钮高度(36px) + 上下间距(10px)
        margin-top: 10px;
        margin-bottom: 5px;
        display: flex;
        justify-content: center;
    }

    // 基于大纲生成内容按钮
    .base-outline-gen-content-btn {
        position: absolute;
        top: 0;
        height: 36px;
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        box-shadow: 0 4px 6px 0 var(--baseOutlineGenContentboxShadow);
        background-color: var(--activityColor);
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
        padding: 0 10px;
        width: fit-content;
        max-width: 288px;

        svg {
            width: 16px;
            height: 16px;
            min-width: 16px;
            margin-right: 5px;
            flex-shrink: 0;
            fill: var(--uosai-color-baseOutlineGenContentBtn-icon);
        }

        .base-outline-gen-content-btn-text {
            font-size: 1rem;
            line-height: 1.2;
            color: var(--uosai-color-baseOutlineGenContentBtn-text);
            font-weight: 500;
            padding: 0 7px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }

        &:not(.disabled):hover {
            background-color: var(--activityColorHover);
        }

        &:not(.disabled):active {
            background-color: var(--activityColor);
        }

        &.disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
    }
</style>