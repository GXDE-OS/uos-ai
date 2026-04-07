<template>
    <div class="main-content"
        v-show="!showMarkdownEditor"
        @dragstart="handleDragStart"
        @dragenter.prevent="handleDragEnter"
        @dragover.prevent="handleDragOver"
        @mouseup="handleMainMouseUp">
        <div class="inner-dropzone"  v-show="isDragging" @dragleave.prevent="handleDragLeave">
            <div class="inner-dropzone-content" >
                <div class="inner-dropzone-icon">
                    <img :src="isDarkMode? 'icons/upload-dark.svg':'icons/upload-light.svg'" alt=""   class="inner-dropzone-icon-icon"/>
                    <!-- <svgIcon :icon="isDarkMode? 'icons/upload-dark.svg':'icons/upload-light.svg'" class="inner-dropzone-icon-icon"/> -->
                </div>
                <div class="inner-dropzone-text"> <span class="inner-dropzone-text-span">{{(store.loadTranslations['Drag files here to add them.'])}}</span></div>
                <div class="inner-dropzone-text-suffix"> <span class="inner-dropzone-text-suffix-span" id="innerDropzoneTextSuffix">{{(store.loadTranslations['You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.'])}}</span></div>
            </div>
        </div>
        <!-- 历史会话列表 -->
        <ConversionList class="conversion-list-wrapper"
            :historyList="historyList"
            :isDarkMode="isDarkMode"
            :show="showConversionList"
            @close="closeHistoryList"
            @selectHistoryItem="selectHistoryItem"
            @showAlertDialog="showAlertDialog"
            @showClearHistoryAlertDialog="showClearHistoryAlertDialog"/>
        <WelcomePage ref="welcomePageRef"
            v-model:question="question"
            :recording="recording"
            :historyLength="history.length"
            :currentAssistant="currentAssistant"
            :isKnowledgeBaseExist="isKnowledgeBaseExist"
            :isEmbeddingPluginsExist="isEmbeddingPluginsExist"
            :isLLMExist="isLLMExist"
            :currentAccount="currentAccount"
            :assistantList="assistantList"
            :shortcutList="shortcutList"
            @update:currentAssistant="updateCurrentAssistant"
            @showAssistantList="showAssistantList"
            v-show="history.length === 0 && store.ConversationModeStatus === store.ConversionMode.Normal"
            :class="{'disabled': isUIDisabledForPopup}" />
        <!-- 隐私会话欢迎页 -->
        <PrivateWelcomePage :class="{'disabled': isUIDisabledForPopup}" v-show="history.length === 0 && store.ConversationModeStatus === store.ConversionMode.Private" :isDarkMode="isDarkMode" />
        <div class="chat-history" :class="{'disabled': isUIDisabledForPopup}" v-show="history.length > 0" @mousewheel="handleHistoryScroll">
            <custom-scrollbar class="history-scrollbar" id="chatHistory" ref="chatHistoryScrollbarRef"
                :autoHideDelay="2000" :thumbWidth="6"
                :wrapperStyle="{height: '100%'}" :style="{ width: '100%', height: '100%'}" >
                <div class="bubble-div">
                    <!-- 重写聊天气泡 -->
                    <ChatBubble v-for="(item, index) in history" :key="index"
                        v-model:playAudioID="playAudioID"
                        :historyIndex="index"
                        :item="item" :showStop="showStop"
                        :isLast="history.length === index + 1"
                        :recording="recording"
                        :netState="netState"
                        :hasOutput="hasOutput"
                        :isKnowledgeBaseExist="isKnowledgeBaseExist"
                        :isEmbeddingPluginsExist="isEmbeddingPluginsExist"
                        @handleShowTip="handleShowTip"
                        @hideTooltip="hideTooltip"
                        @getFreeCredits="getFreeCredits"
                        @retryRequest="retryRequest"
                        @updateLikeOrDislike="updateLikeOrDislike"
                        @questionAction="questionAction"
                        @updateOutline="updateOutline"
                        @updateActiveIndex="updateActiveIndex"
                        @guessYouWantClick="handleGuessYouWantClick"
                        @genContentFromOutline="clickBaseOutlineGenContent"
                        @openMarkdownEditor="openMarkdownEditor"
                        :currentAssistant="currentAssistant"
                        :isLLMExist="isLLMExist"
                        :isDarkMode="isDarkMode"
                        :isWindowMode="isWindowMode"
                    />
                </div>
            </custom-scrollbar>
        </div>
        <div class="chat-bottom">
            <div class="handle-tip" :class="{'disabled': isUIDisabledForPopup}">
                <div class="tip-item-msg" v-show="showTopTip" :class="{ 'advanced-features': store.IsEnableAdvancedCssFeatures }" :style="{'backdrop-filter': store.IsEnableAdvancedCssFeatures  ? isDarkMode ? 'blur(30px) ': 'blur(20px) '  : 'none'}">
                    {{ topTipMsg }}
                </div>
                <div class="tip-item" v-show="showCount" v-if="store.loadTranslations['Stop recording after %1 seconds']">
                    {{ store.loadTranslations['Stop recording after %1 seconds'].replace('%1', countDown) }}
                </div>
                <div class="top-stop tip-item" v-show="showStop"  @click="stopRequest">
                    <svgIcon icon="stop" /> {{ store.loadTranslations['Stop'] }}
                </div>
            </div>
            <div class="top-returnBtnOut" :class="{'disabled': isUIDisabledForPopup}">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="store.loadTranslations['Back to bottom']">
                    <div class="top-returnBtn" @click="returnBottom" v-show="showReturnBottom && history.length > 0">
                        <SvgIcon icon="combobox-arrow"/>
                    </div>
                </el-tooltip>
            </div>
            <div class="top" v-show="!showPropmptList">
                <!-- 创建新会话记录按钮 -->
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="store.loadTranslations['New Conversation']">
                    <div class="add-new-conversation" ref="conversationModeIconRef"
                        :class="{'disabled': disabled || recording || isUIDisabledForPopup}"
                        :style="{ 'margin-right': '6px'}"
                        @click="selectConversionMode(store.ConversionMode.Normal)">
                        <SvgIcon icon="new-conversation" style="width: 20px;height: 20px;"/>
                        <div class="conversation-mode-icon" @click="changeConversationMode">
                            <SvgIcon icon="conversation-mode"  style="width: 6px;height: 3.5px;"/>
                        </div>
                    </div>
                </el-tooltip>
                <!-- 会话模式选择组件 -->
                <ConversionMode :class="{'disabled': isUIDisabledForPopup}"
                    :show="showConversionMode"
                    @close="closeConversionMode"
                    @selectMode="selectConversionMode"
                />
                <!-- 历史记录按钮 -->
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="store.loadTranslations['History']">
                    <div class="prompt-btn" :class="{'disabled': (recording || disabled || isUIDisabledForPopup)}"
                         :style="{ 'margin-right': ((currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && (isShowPromptBtn || isEnableMcp)) ? '6px' : 'auto' }"
                        @click="openHistoryList">
                        <SvgIcon icon="open-history" />
                    </div>
                </el-tooltip>
                <!-- 知识库开关按钮 -->
                <el-tooltip popper-class="uos-tooltip knowledge-base-switch-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="knowledgeBaseSwitchHoverContent"
                    :class="{ 'disabled': !(recording || disabled || isUIDisabledForPopup) }">
                    <div class="prompt-btn" @click="clickKnowledgeBaseIcon"
                       :style="{ 'margin-right': ((currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && (isShowPromptBtn || isEnableMcp)) ? '6px' : 'auto' }"
                        :class="{ 'disabled': (recording || disabled || isUIDisabledForPopup) }"
                        v-show="(currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && isEnableKnowledgeBase">
                        <SvgIcon icon="personal-knowledge-icon" :style="{'margin-left': '1px', 'fill': store.IsOpenKnowledgeBase ?  'var(--activityColor)' : 'var(--uosai-color-flat-btn-icon)'}"/>
                    </div>
                </el-tooltip>
                <!-- 指令按钮 -->
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="instructionHoverContent"
                    :class="{ 'disabled': !(recording || disabled || isUIDisabledForPopup) }">
                    <div class="prompt-btn" @click="selectPrompt"
                        :style="{ 'margin-right': ((currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && isShowPromptBtn && isEnableMcp) ? '6px' : 'auto' }"
                        :class="{ 'disabled': (recording || disabled || isUIDisabledForPopup) }"
                        v-show="(currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && isShowPromptBtn">
                        <SvgIcon icon="prompt"/>
                    </div>
                </el-tooltip>
                <!-- mcp开关按钮 -->
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="mcpHoverContent"
                    :class="{ 'disabled': !(recording || disabled || isUIDisabledForPopup) }">
                    <div class="prompt-btn mcp-switch" @click="clickMcpServerIcon"
                        ref="mcpServerIconRef"
                        style="margin-right: auto;"
                        :class="{ 'disabled': (recording || disabled || isUIDisabledForPopup) }"
                        v-show="(currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && isEnableMcp">
                        <SvgIcon icon="mcp-switch" :style="{'fill': store.IsOpenMcpServer && store.IsInstallUOSAiAgent?  'var(--activityColor)' : 'var(--uosai-color-flat-btn-icon)', 'width': '20px', 'height': '20px'}"/>
                        <div class="setmcp-icon" @click="clickMcpSetting">
                            <SvgIcon icon="conversation-mode"  style="width: 6px;height: 3.5px;" :style = "{'fill': store.IsOpenMcpServer && store.IsInstallUOSAiAgent?  'var(--activityColor)' : 'var(--uosai-color-flat-btn-icon)'}"/>
                        </div>
                    </div>
                </el-tooltip>
                <!-- mcp设置组件 -->
                <McpSetting :class="{'disabled': isUIDisabledForPopup}"
                    :target="mcpServerIconRef"
                    :show="showMcpSetting"
                    @clickMcpIcon="clickMcpServerIcon"
                    @close="closeMcpSetting"
                />
                <!-- 切换助手模型下拉框 -->
                <SwitchModel ref="switchModel" class="switchModel"
                    v-model:currentAccount="currentAccount" v-model:accountList="accountList"
                    v-model:assistantList="assistantList" v-model:currentAssistant="currentAssistant"
                    :showGuide="showGuide"
                    :guideActiveItemId="guideActiveAssistantId"
                    :disabled="disabled || recording"  @update:currentAssistant="updateCurAssistant" @update:currentAccountChanged="currentAccountChanged"/>
            </div>
            <div class="input-content" @mouseup="handleMouseUp" :class="{ 'foucs': isFocus, 'private-mode': store.ConversationModeStatus === store.ConversionMode.Private, 'disabled': isUIDisabledForPopup }" :style="{'background-color': store.ConversationModeStatus === store.ConversionMode.Private ? 'var(--activityColorPrivateModeInputBackgroundColor)' : 'var(--uosai-color-inputcontent-bg)'}">
                <!-- 指令列表 -->
                <PromptList ref="promptListRef" class="propmpt-list"
                    :showPropmptList="showPropmptList"
                    :searchQuery="searchQuery"
                    :promptInfos="promptInfos"
                    @sigHidePromptLists="sigHidePromptLists"
                    @sigSelectOnePrompt="sigSelectOnePrompt"/>
                <div class="input-content-all" :style="{'border-radius': showPropmptList ? '0px 0px 8px 8px' : '8px', }">
                    <!-- 功能按钮列表 -->
                    <FunctionButtons
                        ref="functionButtonsRef"
                        :functionList="functionList"
                        :isWindowMode="isWindowMode"
                        :currentAssistant="currentAssistant"
                        v-show="currentAssistant.type === store.AssistantType.AI_TEXT_PROCESSING_ASSISTANT"
                        @selectFunction="handleFunctionSelect"
                        @clearFunction="handleFunctionClear"
                    />
                    <!-- 挂起来的标签部分 -->
                    <div class="input-tag">
                        <!-- 文档总结 -->
                         <div class="document-parsing" :style="{'display': isWindowMode ? 'flex' : 'block'}">
                            <!-- AI写作助手场景下的特殊处理 -->
                            <template v-if="currentAssistant.type === store.AssistantType.AI_WRITING_ASSISTANT">
                                <!-- 本地素材按钮 -->
                                <div v-if="materialFiles.length > 0" class="local-material-btn-wrapper">
                                    <div class="local-material-btn" @click="showLocalMaterialsList = !showLocalMaterialsList" ref="localMaterialBtnRef">
                                        <SvgIcon icon="local-materials" />
                                        <span class="local-material-text">{{ store.loadTranslations['Local Materials'] }} ({{ materialFiles.length }})</span>
                                        <SvgIcon icon="arrow_down" style="width: 8px; height: 8px; margin-left: 8px; margin-right: 8px;"/>
                                    </div>

                                    <!-- 本地素材列表 - 确保 localMaterialBtnRef 存在后再显示 -->
                                    <LocalMaterialsList
                                        v-if="showLocalMaterialsList && localMaterialBtnRef"
                                        ref="localMaterialsListRef"
                                        :materialList="materialFiles"
                                        :isDarkMode="isDarkMode"
                                        :targetBtnRef="localMaterialBtnRef"
                                        @close="showLocalMaterialsList = false"
                                        @delete="handleDeleteMaterialFile"
                                    />
                                </div>
                                <!-- 仅显示大纲文件 -->
                                <div v-for="file in outlineFiles" :key="file.index">
                                    <DocumentParsing class="document-parsing-item" :style="{'max-width': isWindowMode ? '317px' : 'calc(100% - 20px)'}"
                                        @sigDeleteFile="sigDeleteFile" :fileInfo="file" :isWindowMode="isWindowMode" :isDarkMode="isDarkMode"/>
                                </div>
                            </template>
                            
                            <!-- 其他助手类型的原有逻辑 -->
                            <template v-else>
                                <div v-for="file in inputFileList" :key="file.index">
                                    <DocumentParsing class="document-parsing-item" :style="{'max-width': isWindowMode ? '317px' : 'calc(100% - 20px)'}"
                                        @sigDeleteFile="sigDeleteFile" :fileInfo="file" :isWindowMode="isWindowMode" :isDarkMode="isDarkMode"/>
                                </div>
                            </template>
                         </div>
                        <!-- 指令标签 -->
                        <PromptTag v-show="showPromptTag" :promptTag="promptTag"
                            style="margin-top: 8px;margin-left: 10px;"/>
                        <!-- mcp开启标志 -->
                        <PromptTag v-show="(currentAssistant.type === store.AssistantType.UOS_AI || currentAssistant.id === 'PPT Assistant') && store.IsOpenMcpServer" 
                            :promptTag="'MCP & Skills'"
                            :isMcpTag="true"
                            style="margin-top: 8px;margin-left: 10px;height: 24px;line-height: 24px;"/>
                    </div>
                    <el-input v-model="question"
                        ref="questionInput"
                        resize="none"
                        :autosize="{ minRows: 4, maxRows: 10 }"
                        type="textarea"
                        :placeholder="placeHolder"
                        @focus="handleInputFocus"
                        @blur="handleInputNoFocus"
                        @keydown.enter.native="handeleEnter"
                        @input="handleInput"
                        @keydown.up="highlightPrevious"
                        @keydown.down="highlightNext"
                        @keydown.left="handleKeyLeft"
                        @keydown.right="handleKeyRight"
                        @keydown.enter="handleEnterKeyDown"
                        @keydown.backspace="handleBackspace"
                        @mousedown="handleMouseDown"
                        @mouseup="handleInputMouseUp"
                        @paste="handlePaste"
                        :disabled="recording || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist ||!isEmbeddingPluginsExist)&& history.length === 0)
                        || ((currentAssistant.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||currentAssistant.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT) && !isEmbeddingPluginsExist && history.length === 0)
                        || showGuide" />
                    <div class="bottom">
                        <div style="margin-left: 10px; display: flex;">
                            <ModelSwitch
                                    v-model:currentAccount="currentAccount"
                                    v-model:accountList="accountList"
                                    :disabled="disabled || recording"
                                    :isWindowMode="isWindowMode"
                                    @update:currentAccountChanged="currentAccountChanged"
                                    :style="{'margin-right': isWindowMode ? '10px' : '6px'}"/>
                            <!-- 深度思考和联网搜索 -->
                            <div style="display: flex;">
                                <!-- 深度思考开关 -->
                                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                                :show-after="1000" :offset="2"
                                :content="store.loadTranslations['DeepThink']">
                                    <div v-show="showDeepThink" class="deep-think-btn" @click="deepThinkClick" :style="{'color': store.IsDeepThink ? 'var(--activityColor)' : 'var(--uosai-think-search-color)', 'width' : deepThinkWidth}">
                                        <SvgIcon icon="deep-think" :style="{'fill': store.IsDeepThink ?  'var(--activityColor)' : 'var(--uosai-think-search-color)', 'margin-right': isWindowMode ? '5px' : '0px'}"/>
                                        <span v-show="isWindowMode">{{ store.loadTranslations['DeepThink'] }}</span>
                                    </div>
                                </el-tooltip>
                                <!-- 联网搜索开关 -->
                                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                                :show-after="1000" :offset="2"
                                :content="store.loadTranslations['Search']">
                                    <div v-show="showSearchOnline" class="deep-think-btn" @click="searchOnlineClick" :style="{'color': store.IsSearchOnline? 'var(--activityColor)' : 'var(--uosai-think-search-color)', 'width' : onlineSearchWidth, 'margin-left': (showDeepThink && isWindowMode) ? '10px' : (showDeepThink ? '6px' : '0px')}">
                                        <SvgIcon icon="search-online" :style="{'fill': store.IsSearchOnline ?  'var(--activityColor)' : 'var(--uosai-think-search-color)', 'margin-right': isWindowMode ? '5px' : '0px'}"/>
                                        <span v-show="isWindowMode">{{ store.loadTranslations['Search'] }}</span>
                                    </div>
                                </el-tooltip>
                            </div>
                        </div>
                        <div class="send-btn">
                            <!-- 截图按钮 -->
                            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                                :show-after="1000" :offset="2"
                                :content="screenShotTooltipContent"
                                :class="{ 'disabled': !(inputFileList.length === 3 || recording || disabled || (currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT)) && currentAssistant.type != store.AssistantType.AI_WRITING_ASSISTANT}">
                                <div class="file-btn btn"
                                    ref="screenShotIconRef"
                                    v-show="currentAssistant.type != store.AssistantType.PLUGIN_ASSISTANT && currentAssistant.type != store.AssistantType.AI_WRITING_ASSISTANT && isEnableScreenshot > -1"
                                    :class="{ 'disabled': (inputFileList.length === 3 || recording || disabled || (currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT) || screenShotToolDisable) && currentAssistant.type != store.AssistantType.AI_WRITING_ASSISTANT}"
                                :style="{'margin-right': isWindowMode ? '15px' : '10px'}"
                                    @click="screenshot"
                                    @mouseenter="handleScreenShotIconHover"
                                    @mouseleave="handleScreenShotIconLeave">
                                    <SvgIcon icon="screenshot" style="width: 12px;height: 12px;"/>
                                </div>
                            </el-tooltip>
                            <!-- 文档总结按钮 -->
                            <FileButtonWithMenu
                                ref="fileButtonWithMenuRef"
                                :disabled="disabled || recording"
                                :isWindowMode="isWindowMode"
                                :inputFileListLength="inputFileList.length"
                                :isShowGenContentBtn="isShowGenContentBtn"
                                :currentAssistant="currentAssistant"
                                :materialFilesLength="materialFiles.length"
                                :outlineFilesLength="outlineFiles.length"
                                v-show="(currentAssistant.type != store.AssistantType.PLUGIN_ASSISTANT || currentAssistant.id == 'PPT Assistant')"
                                @selectFile="selectFile"
                                @selectFiles="handleSelectFiles"
                            />
                            <!-- 语音输入按钮 -->
                            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                                :show-after="1000" :offset="2"
                                :content="netState ? !isAudioInput ? store.loadTranslations['Please connect the microphone and try again'] : store.loadTranslations['Voice input'] : store.loadTranslations['Voice input is temporarily unavailable, please check the network!']">
                                <div class="voice-btn btn"
                                    :class="{ recording, disabled: !isAudioInput || showStop || !netState ||
                                        (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist ||!isEmbeddingPluginsExist) && history.length === 0)
                                        || ((currentAssistant.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||currentAssistant.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT) && !isEmbeddingPluginsExist && history.length === 0 )
                                        , notalking: audioLevel === 0 }"
                                    :style="{'margin-right': isWindowMode ? '15px' : '10px'}"
                                    @click="handleRecorder">
                                    <SvgIcon icon="voice" />
                                </div>
                            </el-tooltip>
                            <!-- 发送按钮 -->
                            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                                :show-after="1000" :offset="2"
                                :disabled="sendBtnContent === ''"
                                :content="sendBtnContent">
                                <div class="send btn" :class="{ 'disabled': (question.trim().length === 0  && !isFileInInput) || recording || disabled
                                    || (currentAssistant.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist ||!isEmbeddingPluginsExist) && history.length === 0)
                                    || ((currentAssistant.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||currentAssistant.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT)&& !isEmbeddingPluginsExist  && history.length === 0)
                                    || (isFileInInput && !isAllParsingStatusEnd) || (isFileInInput && isAllParsingStatusEnd && !isFileParsingSuccess)}"
                                    @click="sendQuestion">
                                    <SvgIcon icon="send" style="user-select: none;"/>
                                </div>
                            </el-tooltip>

                        </div>
                    </div>

                </div>
            </div>
            <!-- 提示信息 -->
             <div class="bottom-tip">
                <div class="tip">
                    {{ store.loadTranslations[`The content generated by AI is for reference only, please pay attention to the accuracy of the information.`] }}
                </div>
                <ShortcutTip v-show="isWindowMode && currentShortcutList.length > 0" :currentShortcutList="currentShortcutList"/>
             </div>
            
        </div>
        <!-- 引导界面 -->
        <GuideOverlay
            :steps="guideConfig.steps"
            :guide-type="guideConfig.guideType"
            :visible="showGuide"
            :isWindowMode="isWindowMode"
            :isDarkMode="isDarkMode"
            @close="onGuideClose"
            />
    </div>
    <div class="markdown-editor-container" v-show="showMarkdownEditor">
        <!-- Typora编辑器 -->
        <MarkdownEditor
            v-model="mdEditorContent"
            :isWindowMode="isWindowMode"
            :isDarkMode="isDarkMode"
            :currentShortcutList="currentShortcutList"
            @closeMarkdownEditor="closeMarkdownEditor"
            @save="saveMarkdownContent"/>
    </div>
</template>

<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import _ from "lodash";
import ModelSwitch from "./components/ModelSwitch.vue";
import SwitchModel from "./components/SwitchModel.vue";
import WelcomePage from "./components/welcomePage.vue";
import ChatBubble from "./components/ChatBubble.vue";
import PromptList from "./components/PromptList.vue";
import PromptTag from "./components/PromptTag.vue";
import GuideOverlay from "./components/GuideOverlay.vue";
import ConversionList from "./components/ConversionList.vue"
import ConversionMode from "./components/ConversionMode.vue"
import McpSetting from "./components/McpSetting.vue";
import InputFilesMenu from "./components/InputFilesMenu.vue";
import FileButtonWithMenu from "./components/FileButtonWithMenu.vue";
import PrivateWelcomePage from "./components/PrivateWelcomePage.vue";
import FunctionButtons from "./components/FunctionButtons.vue";
import ShortcutTip from "./components/ShortcutTip.vue";
import CustomScrollbar from 'custom-vue-scrollbar';
import 'custom-vue-scrollbar/dist/style.css';
import { ref, computed, nextTick } from "vue";
import { useRouter } from "vue-router";
import LocalMaterialsList from "./components/LocalMaterialsList.vue";   
import MarkdownEditor from "./components/Outline/MarkdownEditor.vue";

//*******************************************************************************
/**
 * 拖拽蒙版js
 */
import DocumentParsing from "./components/DocumentParsing.vue"
import SvgIcon from "@/components/svgIcon/svgIcon.vue";
const isDragging = ref(false); // 控制蒙版显示的变量
const isFileInInput = ref(false)  //输入框是否存在文件
const isFileParsingSuccess = ref(false)  //文件解析状态成功
const isAllParsingStatusEnd = ref(false)  //所有文件解析状态结束
const isDarkMode = ref(false)  //是否为深色主题
const inputFileList = ref([]) // 输入框中文件列表
const extensionFileList = ref([]) // 扩展文件列表,解析成功待发送

// 文件索引计数器，用于生成唯一的文件标识
let fileIndexCounter = 0

// 生成唯一文件索引
const generateFileIndex = () => {
    return (++fileIndexCounter).toString()
}

// 素材文件列表 - 返回完整的文件对象数组，而不只是 filePath
const materialFiles = computed(() => {
    return inputFileList.value
        .filter(f => f.fileCategory === store.DocFileCategory.LocalMaterial)
})

// 文件大纲
const fileOutline = computed(() => {
    const outline = inputFileList.value
        .find(f => f.fileCategory === store.DocFileCategory.FileOutline)
    return outline?.filePath || ""
})


function handleDragEnter(event) {
    event.preventDefault(); // 阻止默认事件
    event.stopPropagation();

    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    if(showGuide.value) return  //引导页不处理拖拽
    if(isShowGenContentBtn.value) return  //如果当前是生成内容按钮，不允许选择文件
    // if(isFileInInput.value) return  //文件存在
    if(currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT && currentAssistant.id != 'PPT Assistant') return  //非UOS AI助手
    const messageElement = document.getElementById('innerDropzoneTextSuffix');
    if(currentAssistant.value.id === 'PPT Assistant') {
       messageElement.textContent = "最多只能添加一个文件，文件类型：docx"
    } else {
       messageElement.textContent = store.loadTranslations['You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.']
    }
    isDragging.value = true;
}


function handleDragOver(event) {
    event.preventDefault(); // 阻止默认事件

    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    if(showGuide.value) return  //引导页不处理拖拽
    if(isShowGenContentBtn.value) return  //如果当前是生成内容按钮，不允许选择文件
    // if(isFileInInput.value) return  //文件存在
    if(currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT && currentAssistant.id != 'PPT Assistant') return  //非UOS AI助手
    const messageElement = document.getElementById('innerDropzoneTextSuffix');
    if(currentAssistant.value.id === 'PPT Assistant') {
       messageElement.textContent = "最多只能添加一个文件，文件类型：docx"
    } else {
       messageElement.textContent = store.loadTranslations['You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.']
    }
    isDragging.value = true;
}

function handleDragLeave(event) {
    event.stopPropagation();
    isDragging.value = false;
}

function handleDragLeaveChild(event) {
    event.stopPropagation();
    event.preventDefault(); // 阻止默认事件
}

function handleDragEnterChild(event) {
    event.preventDefault(); // 阻止默认事件
    event.stopPropagation();
}

/**
 * 文档显示图标js
 */

//*******************************************************************************

const router = useRouter();
const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()
const store = useGlobalStore()
const instance = getCurrentInstance()
const question = ref('')
const isFocus = ref(false)
const showTopTip = ref(false)
// showStop控制 底部的清除icon、发送按钮、语音图标、模型切换、状态切换按钮、<>置灰，播放按钮置灰，设置不置灰，可正常点击
const showStop = ref(false)
const topTipMsg = ref('')
const accountList = ref([])
const assistantList = ref([])
const currentAccount = ref('')
const currentAssistant = ref('')
const playAudioID = ref('')
const isKnowledgeBaseExist = ref(false)
const isEmbeddingPluginsExist = ref(false)
const isLLMExist = ref(false)
const disabled = computed(() => {
    return showStop.value
})
const isAudioInput = ref(false)
const isWindowMode = ref(true);
const isChineseLanguage = ref(false);

const isEnableMcp = ref(false);
const currentShortcutList = ref([])
const initChat = async () => {
    const resAccount = await Qrequest(chatQWeb.queryLLMAccountList)
    const resCurAccountID = await Qrequest(chatQWeb.currentLLMAccountId)
    const resAssistant = await Qrequest(chatQWeb.queryAssistantList)
    const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
    isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable)
    hasOutput.value = await Qrequest(chatQWeb.isAudioOutputAvailable)
    netState.value = await Qrequest(chatQWeb.isNetworkAvailable)
    isKnowledgeBaseExist.value = await Qrequest(chatQWeb.isKnowledgeBaseExist)
    isEmbeddingPluginsExist.value = await Qrequest(chatQWeb.isEmbeddingPluginsExist)

    isChineseLanguage.value = await Qrequest(chatQWeb.isChineseLanguage)

    // 获取当前对话信息
    await getNowConversationInfo(resCurAssistantID)

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    //新版查询历史记录
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)

    // 判断选中的历史记录最后一条是否有大纲
    if (history.value.length > 0) {
        const displayContentLast = JSON.parse(_.last(_.last(history.value).answers).displayContent)
        if (displayContentLast.find(item => item.chatType === store.ChatAction.ChatOutline)) {
            isLastHasOutline.value = true
        } else {
            isLastHasOutline.value = false
        }
    }
    

    //查询当前系统主题
    const theme = await Qrequest(chatQWeb.getThemeType)
    sigThemeChanged(theme)

    //查询窗口模式
    isWindowMode.value = await Qrequest(chatQWeb.isWindowMode);

    //查询当前后端统一背景色
    const backgroundColor = await Qrequest(chatQWeb.getMainContentbackgroundColor);
    sigMainContentBackgroundColor(backgroundColor)

    // 查询是否编译在QT6上
    store.IsEnableAdvancedCssFeatures = await Qrequest(chatQWeb.isEnableAdvancedCssFeatures);

    console.log("assistant list: ", resAssistant);
    console.log("current assistant id: ", resCurAssistantID);
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

    // 判断当前是否已领取免费模型
    const account = accountList.value.find(item => item.model === store.Uos_Free)
    if (account) {
        isGotDeepSeekUosFree.value = true
    } else {
        isGotDeepSeekUosFree.value = false
    }

    //查询指令列表
    currentAccountChanged(currentAccount)

    //查询是否启用mcp
    isEnableMcp.value = await Qrequest(chatQWeb.isEnableMcp);
    store.IsEnableMcp = isEnableMcp.value

    //查询是否启用截图
    isEnableScreenshot.value = await Qrequest(chatQWeb.isEnableScreenshot);
    screenShotTooltipContent.value = store.loadTranslations['Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported.']

    //查询是否启用知识库
    isEnableKnowledgeBase.value = await Qrequest(chatQWeb.isEnableKnowledgebase);

    // 查询是否为简体中文
    store.IsSimplifiedChinese = await Qrequest(chatQWeb.isSimplifiedChinese);

    await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件

    //查询当前快捷键
    const shortcutList = await Qrequest(chatQWeb.getCurrentShortcut);
    // 解析快捷键字符串，如 "<Shift><Control><Alt><Super>A" 转换为 ["Shift", "Control", "Alt", "Super", "A"]
    if (shortcutList && typeof shortcutList === 'string') {
        // 移除尖括号并分割成数组
        const keys = shortcutList.replace(/[<>]/g, ' ').trim().split(/\s+/);
        currentShortcutList.value = keys.filter(key => key.length > 0);

        // 将Control转换为Ctrl
        currentShortcutList.value = currentShortcutList.value.map(key => key === 'Control' ? 'Ctrl' : key);
    } else {
        currentShortcutList.value = [];
    }

    //渲染完成，通知后端
    Qrequest(chatQWeb.webViewLoadFinished)
    Qrequest(chatQWeb.chatInitFinished)

    isMouseScroll.value = false  //初始化滚轮状态
    showReturnBottom.value = false  //初始化返回底部按钮状态
    nextTick(() => handelScrol())

    // 初始化知识库，指令，mcp的hover文案
    knowledgeBaseSwitchHoverContent.value = store.loadTranslations["After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base."]
    mcpHoverContent.value = "MCP & Skills"
    instructionHoverContent.value = store.loadTranslations["Instruction"]

    // 解决输入框重新计算行高导致页面跳动问题
    question.value = ""
    setTimeout(() => {
        question.value = ""
        if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
            question.value = store.loadTranslations["I am [Enter Identity/Position]. Please help me write a [Report/Article/Outline/WeChat Official Account Post/Notice/Research Report/Work Summary/Speech] on [Enter Theme], with a length of about [1000 words]. The content requirements are [Enter Requirements/Content Focus/Content Style, etc.]."];
            Qrequest(chatQWeb.setDigitalImageDisable, true);  // 禁用数字形象
        } else {
            Qrequest(chatQWeb.setDigitalImageDisable, false);  // 启用数字形象
        }
    }, 5);

    // 判断数字形象是否被禁用
    const isActiveChatFromDigitalImage = await Qrequest(chatQWeb.isActiveChatFromDigitalImage);
    if (isActiveChatFromDigitalImage) {
        handleShowTip(store.loadTranslations['Digital Human Unavailable'])
    }
}

const isUIDisabledForPopup = computed(() => {
  return showConversionList.value || showGuide.value
})

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

const talkID = ref('')
const waitSend = ref(false)
//修改历史记录结构体
const history = ref([])
const isShowFile = ref(false)
const sendBtnContent = computed(() => {
    if (isFileInInput.value && isAllParsingStatusEnd.value && !isFileParsingSuccess.value) {
        return store.loadTranslations['Please delete the abnormal file and send it again']
    }
    return ''
})
const sendQuestion = async (status) => {
//*****************************************************************************************************************
    if ((question.value.trim().length === 0 && !isFileInInput.value) || disabled.value) return
    if (currentAssistant.value.type == store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isEmbeddingPluginsExist.value || !isKnowledgeBaseExist.value) && history.value.length === 0) return
    if (currentAssistant.value.type == store.AssistantType.DEEPIN_SYSTEM_ASSISTANT && !isEmbeddingPluginsExist.value && history.value.length === 0) return
    if (currentAssistant.value.type == store.AssistantType.UOS_SYSTEM_ASSISTANT && !isEmbeddingPluginsExist.value && history.value.length === 0) return
    if (isFileInInput.value && !isAllParsingStatusEnd.value) return  //所有文件解析状态结束
    if (isFileInInput.value && isAllParsingStatusEnd.value && !isFileParsingSuccess.value) {
        handleShowTip(store.loadTranslations['Please delete the abnormal file and send it again'])
        return  //文件存在但解析失败
    }

    // 录音中点击发送按钮,不允许发送
    if (recording.value) {
        return
    }
    await Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
//*****************************************************************************************************************

    /**
     * TODO:
     * 1.发送问题：只传question结构体
     * 2.应答：只回复answer结构体
     * 3.维护历史记录结构体
     */


    /**
     * 后期改接口
     */
     if(isFileInInput.value && isFileParsingSuccess.value && question.value.trim().length === 0){
        if (showPromptTag.value ) {
            question.value = ''
        } else if (currentAssistant.value.id === 'PPT Assistant') {
            question.value = '根据文档生成PPT。'
        } else {
            switch(currentAssistant.value.type){
                case store.AssistantType.UOS_AI:
                case store.AssistantType.UOS_SYSTEM_ASSISTANT:
                case store.AssistantType.DEEPIN_SYSTEM_ASSISTANT:
                case store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT:
                    question.value = docParsingPlaceHolder.value
                    break
                case store.AssistantType.AI_WRITING_ASSISTANT:  //写作
                    question.value = store.loadTranslations["Write an article based on the following document:"]
                    break
                case store.AssistantType.AI_TEXT_PROCESSING_ASSISTANT:  //文本处理
                    {
                        const idx = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type).index
                        if (idx === -1) {
                            question.value = store.loadTranslations['Summarize the key content of the file.']
                        }
                    }
                    break
                case store.AssistantType.AI_TRANSLATION_ASSISTANT:  //翻译
                    question.value = store.loadTranslations["Translate the following document into English:"]
                    break
                default:
                    question.value = ''
                    break
            }
        }
    }
    let sendQuestionAccount = currentAccount.value
    const { id, model, icon, displayname } = sendQuestionAccount
    const extention = []
    const ques = {
        displayContent:question.value,
        reqId:"0",
        chatType:0,
        isRetry:false,
        onlineSearch: store.IsSearchOnline,
	    extention:JSON.stringify(extention)
    }
    // ques添加openThink字段和onlineSearch字段
    // 深度思考开关显示时才添加openThink字段
    if (showDeepThink.value) {
        ques.openThink = store.IsDeepThink
    } else {
        ques.openThink = false
    }

    // 联网搜索开关显示时才添加onlineSearch字段
    if (showSearchOnline.value) {
        ques.onlineSearch = store.IsSearchOnline
    } else {
        ques.onlineSearch = false
    }

    if(currentAssistant.value.type == store.AssistantType.AI_WRITING_ASSISTANT && (fileOutline.value !== "" || materialFiles.value.length > 0)) {
        isShowFile.value = false 
        isFileInInput.value = false
        isFileParsingSuccess.value = false
        isAllParsingStatusEnd.value = false

        let ret = await Qrequest(chatQWeb.showAllowUploadFilesAlert, model, displayname, showSearchOnline.value && store.IsSearchOnline)
        if (!ret) {
            return
        }

        // 合并大纲文件和素材文件到一个扁平数组
        let writingFiles = [
            ...outlineFiles.value,
            ...materialFiles.value
        ]

        // 如果有文件，添加到扩展信息中
        extention.push({
            type: store.ExtentionType.WritingResource,
            template_path: fileOutline.value,
            file_paths: materialFiles.value.map(f => f.filePath),
            files: writingFiles
        })

        ques.extention = JSON.stringify(extention)
    } else if(isFileInInput.value && isFileParsingSuccess.value){
        isShowFile.value = false
        isFileInInput.value = false
        isFileParsingSuccess.value = false
        isAllParsingStatusEnd.value = false

        extention.push({
            type: store.ExtentionType.DocSummary,
            files: extensionFileList.value
        })
        
        ques.extention = JSON.stringify(extention)
    }
    
    //标签存在
    if(showPromptTag.value){
        extention.push({
            type: store.ExtentionType.PromptTag,
            tagType:promptInfo.value.tagType,
            tagName:promptInfo.value.tagName,
            content:promptInfo.value.content,
            description:promptInfo.value.content
        })
        ques.extention = JSON.stringify(extention)
        showPromptTag.value = false
    }

    // MCP开关 uos ai智能体才支持 且uos ai智能体已安装
    if (store.IsOpenMcpServer && currentAssistant.value.type == store.AssistantType.UOS_AI && store.IsInstallUOSAiAgent) {
        extention.push({
            type: store.ExtentionType.McpBtnStatus,
            McpServers: true
        })
        ques.extention = JSON.stringify(extention)
    }


    //标签列表存在
    if (showPropmptList.value) {
        showPropmptList.value = false
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

    // 写作智能体
    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        // 发送问题后功能按钮取消选中
        const found = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type)
        if (found) {
            found.index = -1
            functionButtonsRef.value.resetActiveIndex()
        }

        // 扩展添加大纲
        if (isShowGenContentBtn.value && status === store.SendArg.SendDocFileOutline) {
            extention.push({
                type: store.ExtentionType.Outline,
                Outline: OutlineContentEx.value  
            })
            ques.extention = JSON.stringify(extention)
        }
    }

    // 文本处理智能体
    if (currentAssistant.value.type === store.AssistantType.AI_TEXT_PROCESSING_ASSISTANT) {
        const idx = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type).index
        if (idx !== -1) {
            if (ques.displayContent !== ""){
                ques.displayContent = "[" + functionList.value[idx].Name + "]\n " + ques.displayContent
            }else{
                ques.displayContent = "[" + functionList.value[idx].Name + "]"
            }
        }
    }

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
                assistantName : currentAssistant.value.displayname,
                errCode:0,
                errInfo:{},  //补充json
                chatType:0,  //问答、文生图、FunctionCall
                isRetry:false,
                extention:"[]",
                thinkTime: "-1",
                openThink: false,
                onlineSearch: false,
                knowledgeSearchStatus: (store.IsOpenKnowledgeBase && currentAssistant.value.type === store.AssistantType.UOS_AI) ? true : false,  //是否显示知识库搜索
            }
        ]
     })

    showStop.value = true
    question.value = ''
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

    talkID.value = JSON.parse(quesResp).reqId

    isMouseScroll.value = false  //初始化滚轮状态
    showReturnBottom.value = false  //初始化返回底部按钮状态
    handelScrol()

    answerDisplayMsg.value = []  // 初始化answerDisplayMsg

    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))


    // 文件发送完成后，清空inputFileList.value
    inputFileList.value = []
    extensionFileList.value = []
    await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件

    isLastHasOutline.value = false

}

const sigWordWizardAsk = async (askQuestion) => {
    if(showGuide.value) return  //引导页不处理问一问对话
//*****************************************************************************************************************
    if (askQuestion.trim().length === 0) return

//*****************************************************************************************************************

    /**
     * TODO:
     * 1.发送问题：只传question结构体
     * 2.应答：只回复answer结构体
     * 3.维护历史记录结构体
     */


    // const { id, model, icon, displayname } = currentAccount.value
    const extention = []
    const ques = {
        displayContent:askQuestion,
        reqId:"0",
        chatType:0,
        isRetry:false,
        onlineSearch: store.IsSearchOnline,
	    extention:JSON.stringify(extention)
    }
    // ques添加openThink字段和onlineSearch字段
    // 深度思考开关显示时才添加openThink字段
    if (showDeepThink.value) {
        ques.openThink = store.IsDeepThink
    } else {
        ques.openThink = false
    }

    // 联网搜索开关显示时才添加onlineSearch字段
    if (showSearchOnline.value) {
        ques.onlineSearch = store.IsSearchOnline
    } else {
        ques.onlineSearch = false
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
                assistantName : currentAssistant.value.displayname,
                errCode:0,
                errInfo:{},  //补充json
                chatType:0,  //问答、文生图、FunctionCall
                isRetry:false,
                extention:"[]",
                thinkTime: "-1",
                openThink: false,
                onlineSearch: false
            }
        ]
     })

    showStop.value = true
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

    talkID.value = JSON.parse(quesResp).reqId

    isMouseScroll.value = false  //初始化滚轮状态
    showReturnBottom.value = false  //初始化返回底部按钮状态
    handelScrol()

    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))
}

const sigGetFreeCreditsResult = (isSuccess, msg) => {
    store.IsGotFreeCredits = isSuccess
}

const sigIsGotFreeCredits = (isGot) => {
    store.IsGotFreeCredits = isGot
}

const sigActiveChatFromDigitalImage = () => {
    handleShowTip(store.loadTranslations['Digital Human Unavailable'])
}

const sendEmptyAnswers = async () => {
    let sendQuestionAccount = currentAccount.value
    const { id, model, icon, displayname } = sendQuestionAccount
    const extention = []

    const reqId = _.last(_.last(history.value).answers).reqId

    history.value.push({
        // question:ques,
        answers:[
            {
                displayContent:"",
                content:"",
                displayHash:"",
                reqId:"0",
                llmIcon: icon ? icon : '',
                llmName: displayname ? displayname : '',
                llmId: id ? id : '',
                llmModel: model,
                assistantId: currentAssistant.value.id ? currentAssistant.value.id : '',
                assistantName : currentAssistant.value.displayname,
                errCode:0,
                errInfo:{},  //补充json
                chatType:0,  //问答、文生图、FunctionCall
                isRetry:false,
                extention:"[]",
                thinkTime: "-1",
                openThink: store.IsDeepThink,
                onlineSearch: store.IsSearchOnline
            }
        ]
     })

    showStop.value = true
    question.value = ''

    try{
        JSON.parse(quesResp)
    }
    catch(e){
        return
    }

    _.last(_.last(history.value).answers).reqId = reqId

    talkID.value = reqId
}

// TODO: 截图按钮
const screenShotIconRef = ref(null)
const isEnableScreenshot = ref(-1)
const screenshot = async () => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    if(currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT) return  //非UOS AI助手
    if(inputFileList.value.length === 3) return  //最多支持3个文件
    await Qrequest(chatQWeb.startScreenshot)
}

const screenShotTooltipContent = ref('')
const screenShotToolDisable = ref(false)
const handleScreenShotIconHover = async () => {
    isEnableScreenshot.value = await Qrequest(chatQWeb.isEnableScreenshot);
    screenShotTooltipContent.value = store.loadTranslations['Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported.']
    if(isEnableScreenshot.value === 2) {
        screenShotTooltipContent.value = store.loadTranslations['Cannot be used during screen recording']
        screenShotToolDisable.value = true
    }
}

const handleScreenShotIconLeave = async () => {
    screenShotToolDisable.value = false
}

const selectFile = async (event) => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    if(currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT && currentAssistant.id != 'PPT Assistant') return  //非UOS AI助手
    if(isShowGenContentBtn.value) return  //如果当前是生成内容按钮，不允许选择文件

    if(inputFileList.value.length === 3) return  //最多支持3个文件
    await Qrequest(chatQWeb.onDocSummarySelect)
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


const topTipTimer = ref(null)
const hideTooltip = async () => {
    // 清除之前的定时器
    if (topTipTimer.value) {
        clearTimeout(topTipTimer.value)
        topTipTimer.value = null
    }

    // 先隐藏之前的 tip
    showTopTip.value = false
}
const handleShowTip = (msg) => {
    // 清除之前的定时器
    if (topTipTimer.value) {
        clearTimeout(topTipTimer.value)
        topTipTimer.value = null
    }

    // 先隐藏之前的 tip
    showTopTip.value = false

    // 短暂延迟后显示新的 tip，确保过渡平滑
    setTimeout(() => {
        topTipMsg.value = msg
        showTopTip.value = true

        // 设置新的定时器
        topTipTimer.value = setTimeout(() => {
            showTopTip.value = false
            topTipTimer.value = null
        }, 3000)
    }, 50)
}

const updateCurAssistant = async (assistant) => {
    console.log("current assistant id changed: ", assistant.id);

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    store.ConversationModeStatus = store.ConversionMode.Normal
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    await getNowConversationInfo(assistant.id)
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)
    question.value = ''

    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        question.value = store.loadTranslations["I am [Enter Identity/Position]. Please help me write a [Report/Article/Outline/WeChat Official Account Post/Notice/Research Report/Work Summary/Speech] on [Enter Theme], with a length of about [1000 words]. The content requirements are [Enter Requirements/Content Focus/Content Style, etc.]."];
        Qrequest(chatQWeb.setDigitalImageDisable, true);  // 禁用数字形象
    } else {
        Qrequest(chatQWeb.setDigitalImageDisable, false);  // 启用数字形象
    }

    // 判断选中的历史记录最后一条是否有大纲
    try {
        const displayContentLast = JSON.parse(_.last(_.last(history.value).answers).displayContent)
        if (displayContentLast.find(item => item.chatType === store.ChatAction.ChatOutline)) {
            isLastHasOutline.value = true
        } else {
            isLastHasOutline.value = false
        }
    } catch (error) {
        
    }

    //切换助手，聊天窗口存在文件则删除
    if(isFileInInput.value){
        isShowFile.value = false
        isFileInInput.value = false
        isFileParsingSuccess.value = false
        isAllParsingStatusEnd.value = false

        inputFileList.value = []
        extensionFileList.value = []
        await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
    }

    //切换助手，聊天窗口存在标签则删除
    if (showPromptTag.value) {
        showPromptTag.value = false
        showPropmptList.value = false
    }

    /**
     * TODO:
     * 查询当前助手指令列表
     */
     currentAccountChanged()

    //查询功能按钮list
    const functionListStr = await Qrequest(chatQWeb.getAssistantFunctions, assistant.type)
    if (functionListStr !== "") {
        functionList.value = JSON.parse(functionListStr)
    }else{
        functionList.value = []
    }

     isMouseScroll.value = false  //初始化滚轮状态
     showReturnBottom.value = false  //初始化返回底部按钮状态
     nextTick(() => handelScrol())

     nextTick(() => {
        questionInput.value.focus()  //页面加载完成，自动聚焦到输入框
    }, 5)

    answerDisplayMsg.value = []  // 初始化answerDisplayMsg

}

const isShowPromptBtn = ref(false)
const currentAccountChanged  = async (accent) => {
    // mcp开启下切换到非官方模型，tips提示.
    // 开启mcp状态下或当前为运维智能体，模型变化，上一个模型为官方模型，当前不为官方模型
    if (store.IsOpenMcpServer && accent && accent.model != store.Uos_Free && store.LastModel == store.Uos_Free) {
        handleShowTip(store.loadTranslations["It is recommended to use the official model \"Intelligent Routing\""])
    }

    // 切换完成，更新上一个模型
    if (accent) {
        store.LastModel = accent.model
    }

    /**
     * TODO:
     * 查询当前助手指令列表
     */
     const getInstList_ = await Qrequest(chatQWeb.getInstList)
    try {
        promptInfos.value = JSON.parse(getInstList_)
    } catch (error) {
        isShowPromptBtn.value = false
        showPropmptList.value = false
        showPromptTag.value = false
        return
    }

    if (promptInfos.value.length == 0) {
        isShowPromptBtn.value = false
        showPropmptList.value = false
        showPromptTag.value = false
        return
    }
    isShowPromptBtn.value = true

    //当前模型不支持当前指令，清空
    let promptNameLists = []
    promptInfos.value.forEach(element => {
        promptNameLists.push(element.tagName)
    })

    if (showPromptTag.value && !promptNameLists.includes(promptTag.value)) {
        showPromptTag.value = false
    }

    // mcp开启下切换到非官方模型，tips提示.
    // 开启mcp状态下，模型变化，上一个模型为官方模型，当前不为官方模型
    if (store.IsOpenMcpServer && accent && accent.model != store.Uos_Free && store.LastModel == store.Uos_Free) {
        handleShowTip(store.loadTranslations["It is recommended to use the official model \"Intelligent Routing\""])
    }

    // 切换完成，更新上一个模型
    if (accent) {
        store.LastModel = accent.model
    }

    
}
//*********************************************
//提示词相关
const showPropmptList = ref(false)
const promptListRef=ref()
const searchQuery = ref('')
const showPromptTag = ref(false)
const promptTag = ref("")
const promptInfos = ref([])
const isInputPrompt = ref(false)
const handleInput = async (value) => {
    // 引导页不处理输入
    if(showGuide.value) {
        return
    }

    const selectionStart = questionInput.value.$el.querySelector('textarea').selectionStart
    searchQuery.value = ''
    if (value.startsWith('/') || value.startsWith('、')) {
        /**
         * TODO:
         * 查询当前助手指令列表
         */
        const getInstList_ = await Qrequest(chatQWeb.getInstList)
        try {
            promptInfos.value = JSON.parse(getInstList_)
        } catch (error) {
            return
        }

        if (promptInfos.value.length == 0) {
            showPropmptList.value = false;
            return
        }

        showPropmptList.value = true;
        if (value.startsWith('/')) {
            searchQuery.value = value.substr(0,selectionStart).replace('/','')
        }
        if (value.startsWith('、')) {
            searchQuery.value = value.substr(0,selectionStart).replace('、','')
            question.value = question.value.replace('、','/')
            nextTick(() => questionInput.value.$refs.textarea.setSelectionRange(1, 1))  //解决中文状态下修改question导致光标跑到最后的问题

        }

        promptListRef.value.highlightedIndex = 0
        // question.value = question.value.replace('/','、')
        isInputPrompt.value = true
    } else {
      showPropmptList.value = false;
      isInputPrompt.value = false;
    }
};

const sigHidePromptLists = async (value) => {
    showPropmptList.value = false
    if (value && !isInputPrompt.value) {  //非手动输入状态关闭列表时删除/
        question.value = question.value.replace('/','')
    }
    isInputPrompt.value = false
}

const promptInfo = ref({})
const sigSelectOnePrompt = async (promptInfo_) => {
    questionInput.value.focus()
    /**
     * TODO:
     * 聊天框中显示指令
     */

    promptTag.value = promptInfo_.tagName
    promptInfo.value = promptInfo_
    showPromptTag.value = true
    question.value = question.value.replace('/'+searchQuery.value,'')

    // 有指令的情况，清空mcp勾选
    store.IsOpenMcpServer = false

    // 有指令的情况，清空知识库勾选
    store.IsOpenKnowledgeBase = false

    // 修改知识库，指令，mcp的hover文案
    knowledgeBaseSwitchHoverContent.value = store.loadTranslations["Knowledge base unavailable when any command or MCP is selected."]
    mcpHoverContent.value = "MCP & Skills"
    instructionHoverContent.value = store.loadTranslations["Instruction"]

}

const highlightPrevious = (event) => {
    if (showPropmptList.value) {
        promptListRef.value.highlightPrevious()
        nextTick(() => event.target.setSelectionRange(1, 1))
    }
};
const highlightNext = (event) => {
    if (showPropmptList.value) {
        promptListRef.value.highlightNext()
        nextTick(() => event.target.setSelectionRange(1, 1))
    }
};

const handleKeyLeft = async (event) => {
    // handleInput(question.value)
};
const handleKeyRight = (event) => {
    // handleInput(question.value)
};

const handleEnterKeyDown = (event) => {
    if (showPropmptList.value) {
        promptListRef.value.handleEnterKeyDown()
        nextTick(() => event.target.setSelectionRange(0, 0))
    }
};

const handleBackspace = (event) => {
    // Backspace键被按下时执行的逻辑
    // start用于判断光标在第一位
    // end用于判断是否为全选，全选时end != 0
    if (showPromptTag.value
        && questionInput.value.$el.querySelector('textarea').selectionStart == 0
        && questionInput.value.$el.querySelector('textarea').selectionEnd == 0) {
        showPromptTag.value = false
    }
};

const inputSelectStart = ref(0)
const inputSelectEnd = ref(0)
const handleMouseDown = (event) => {
    //获取鼠标按下时选中的文本范围
    inputSelectStart.value = questionInput.value.$el.querySelector('textarea').selectionStart
    inputSelectEnd.value = questionInput.value.$el.querySelector('textarea').selectionEnd
}

const handleInputMouseUp = (event) => {
    //获取鼠标按下时选中的文本范围
    //与鼠标按下时选中的文本范围比较，如果范围相同，则取消选中状态，将光标设置到最后
    if (questionInput.value.$el.querySelector('textarea').selectionStart == inputSelectStart.value
        && questionInput.value.$el.querySelector('textarea').selectionEnd == inputSelectEnd.value) {
        nextTick(() => event.target.setSelectionRange(question.value.length, question.value.length))
    }
    event.preventDefault(); // 阻止默认事件
    event.stopPropagation();

    // 如果正在点击文件上传按钮，不要关闭菜单
    if (fileButtonWithMenuRef.value && !fileButtonWithMenuRef.value.isClickingFileBtn) {
        fileButtonWithMenuRef.value.closeMenu()
    }
}

const handleMouseUp = (event) => {
    questionInput.value.focus()
    event.preventDefault(); // 阻止默认事件
    event.stopPropagation();
};

const handleMainMouseUp = (event) => {
    // 只有当点击不在输入框相关区域时，才将其判定为“失焦”
    if (!event.target.closest('.input-content')) {
        isFocus.value = false;
    }

    if (showPropmptList.value && !isFocus.value) {
        nextTick(() => {
            if (!isInputPrompt.value) {
                sigHidePromptLists(true)
            }else{
                sigHidePromptLists(false)
            }
        })
    }

    if (showConversionMode.value && !isFocus.value) {
        closeConversionMode()
    }

    if (showMcpSetting.value && !isFocus.value) {
        closeMcpSetting()
    }

    if (fileButtonWithMenuRef.value && !isFocus.value) {
        fileButtonWithMenuRef.value.closeMenu()
    }
};

const handleDragStart = (event) => {
    event.preventDefault();
    event.stopPropagation();
}

const handleInputFocus = (event) => {
    isFocus.value = true
    if (showPropmptList.value) {
        nextTick(() =>event.target.setSelectionRange(1, 1))
    }


    if (isAudioInInpit.value) {  //语音录入的文字
        nextTick(() =>event.target.setSelectionRange(question.value.length, question.value.length))
        isAudioInInpit.value = false
        return
    }

    if (showConversionMode.value) {
        closeConversionMode()
    }

    if (showMcpSetting.value) {
        closeMcpSetting()
    }

    // 如果正在点击文件上传按钮，不要关闭菜单
    if (fileButtonWithMenuRef.value && !fileButtonWithMenuRef.value.isClickingFileBtn) {
        fileButtonWithMenuRef.value.closeMenu()
    }
};
const handleInputNoFocus = (event) => {
    // isFocus.value = false
    // if (showPropmptList.value) {
        // nextTick(() => sigHidePromptLists(false))
    // }
};

const selectPrompt = async (event)  => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中

    question.value = "/" + question.value
    // handleInput(question.value)

    /**
     * TODO:
     * 查询当前助手指令列表
     */
     const getInstList_ = await Qrequest(chatQWeb.getInstList)
    try {
        promptInfos.value = JSON.parse(getInstList_)
    } catch (error) {
        showPropmptList.value = false;
        return
    }

    if (promptInfos.value.length == 0) {
        showPropmptList.value = false;
        return
    }

    showPropmptList.value = true;
    searchQuery.value = ''
    promptListRef.value.highlightedIndex = 0
    questionInput.value.focus()

    nextTick(() => {

        if (focus.value) {
            nextTick(() =>event.target.setSelectionRange(1, 1))
        }
    })
};

const mcpServerIconRef = ref(null)
const clickMcpServerIcon = async () => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    questionInput.value.focus()

    if (!store.IsOpenMcpServer) {
        queryMcpServerList()
    } else {
        store.IsOpenMcpServer = !store.IsOpenMcpServer
    }
    
}

const showMcpSetting = ref(false)
const clickMcpSetting = async (event) => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    event.stopPropagation()
    showMcpSetting.value = !showMcpSetting.value
}

const closeMcpSetting = () => {
    showMcpSetting.value = false
}

const queryMcpServerList = async () => {
    store.IsInstallUOSAiAgent = await Qrequest(chatQWeb.isInstallUosAiAgent, store.DefaultAgentName)
    if (store.IsInstallUOSAiAgent) {
        const getThirdPartyMcpAgreement = await Qrequest(chatQWeb.getThirdPartyMcpAgreement)  // 查询是否同意使用三方mcp服务
        if (!getThirdPartyMcpAgreement) {
            Qrequest(chatQWeb.writeVueLog, store.LogLevel.WARN, 'getThirdPartyMcpAgreement: ' + getThirdPartyMcpAgreement)
            return
        }
        store.IsInstallUOSAiAgent = true
        store.IsOpenMcpServer = !store.IsOpenMcpServer
        if (store.IsOpenMcpServer && currentAccount.value.model != store.Uos_Free && !store.IsShowNonOfficialModelTip) {
            store.IsShowNonOfficialModelTip = true
            handleShowTip(store.loadTranslations["It is recommended to use the official model \"Intelligent Routing\""])
        }

        if (store.IsOpenMcpServer) {
            handleMcpCheckChange(true)  // 开启mcp后，清空指令，知识库，还原hover文案
        }
    }else {
        // 未安装UOSAI Agent
        store.IsInstallUOSAiAgent = false
        store.IsOpenMcpServer = false
        await Qrequest(chatQWeb.showInstallUosAIAgentDlg)
    }


}

const handleMcpCheckChange = (isChecked) => {
    // 当MCP被勾选时， 如果输入框有指令，则清空输入框的指令和文字
    if (isChecked && showPromptTag.value) {
        // 清空指令标签
        showPromptTag.value = false
        promptTag.value = ''
        promptInfo.value = {}
        // 清空输入框文字
        question.value = ''
    }

    // 勾选mcp的情况，清空知识库勾选
    store.IsOpenKnowledgeBase = false

    // 修改知识库，指令，mcp的hover文案
    knowledgeBaseSwitchHoverContent.value = store.loadTranslations["Knowledge base unavailable when any command or MCP is selected."]
    mcpHoverContent.value = "MCP & Skills"
    instructionHoverContent.value = store.loadTranslations["Instruction"]
}

const isEnableKnowledgeBase = ref(false)
const knowledgeBaseSwitchHoverContent = ref('')
const instructionHoverContent = ref('')
const mcpHoverContent = ref('')

const clickKnowledgeBaseIcon = (() => {
    // 知识库勾选状态切换
    store.IsOpenKnowledgeBase = !store.IsOpenKnowledgeBase
    knowledgeBaseSwitchHoverContent.value = store.loadTranslations["After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base."]
    if (!store.IsOpenKnowledgeBase) {
        mcpHoverContent.value = "MCP & Skills"
        instructionHoverContent.value = store.loadTranslations["Instruction"]
        return  // 关闭知识库不需要判断插件和知识库是否存在
    }

    // 假设目前知识库环境是完整的，后期添加知识库环境判断
    if (!isEmbeddingPluginsExist.value) {
        // 提示用户安装插件
        store.IsOpenKnowledgeBase = false
        Qrequest(chatQWeb.showKnowledgeBaseErrorDialog, 0)
        return
    }

    if (!isKnowledgeBaseExist.value) {
        // 提示用户配置知识库
        store.IsOpenKnowledgeBase = false
        Qrequest(chatQWeb.showKnowledgeBaseErrorDialog, 1)
        return
    }

    if (store.IsOpenKnowledgeBase) {
        // mcp取消勾选
        store.IsOpenMcpServer = false

        // 清空输入框的指令
        showPromptTag.value = false

        // 修改知识库，指令，mcp的hover文案
        mcpHoverContent.value = store.loadTranslations["MCP is disabled while the knowledge base is active."]
        instructionHoverContent.value = store.loadTranslations["Commands disabled while knowledge base is active."]
    }
})

//输入框暗文
const docParsingPlaceHolder = ref('')
const placeHolder = computed(() => {
    // 开启mcp
    if (store.IsOpenMcpServer && store.IsInstallUOSAiAgent && currentAssistant.value.type === store.AssistantType.UOS_AI) {
        let placeHolder = store.loadTranslations["Enter MCP & Skills Server command, e.g., \"Change system to dark mode for me\""]
        if (currentAccount.value.model !== store.Uos_Free) {
            // 如果当前助手是UOS_AI，且开启了MCP服务器，且当前模型不是智能调度，则提示切换到正式版模型
            placeHolder += "\n* " + store.loadTranslations["For MCP & Skills Server, it is recommended to switch to the official model \"Intelligent Routing\""]
        }
        return placeHolder
    }

    //没有文件或标签
    //没有文件或标签
    if (!isFileInInput.value && !showPromptTag.value) {
        if (currentAssistant.value.id === 'PPT Assistant') {
            return '请输入PPT主题或者内容；'
        }
        if (currentAssistant.value.id === 'ai-writing') {
            // 写作助手：已发送提示词进入内容生成环节
            if (history.value.length > 0) {
                return store.loadTranslations['You can continue to input more requests to optimize or adjust the already generated content.']
            }
            return store.loadTranslations["Please Describe the Content Theme and Requirements for Your Creation."]
        }
        if (currentAssistant.value.id === 'ai-text-processing') {
            return store.loadTranslations["Please Enter the Text You Need to Process and Specify Your Requirements."]
        }
        if (currentAssistant.value.id === 'ai-translation') {
            return store.loadTranslations["Please Enter the Content You Want to Translate and Specify the Target Language. Default Translation is to Chinese."]
        }

        return store.loadTranslations["Enter your question, or enter \"/\" to select a command\n\"Ctrl+Enter\"  to start a new line"]
    }

    //文件和标签同时存在, 或标签单独存在
    if (showPromptTag.value) {
        return promptInfo.value.content
    }

    //文件存在，标签不存在
    if (isFileInInput.value) {
        if (currentAssistant.value.id === 'PPT Assistant') {
            return '根据文档生成PPT。'
        }
        if (currentAssistant.value.id === 'ai-writing' || currentAssistant.value.id === 'ai-text-processing' || currentAssistant.value.id === 'ai-translation') {
            return ''
        }
        return docParsingPlaceHolder.value
    }
});

// 点赞踩变化
const updateLikeOrDislike = () => {
    // Qrequest(chatQWeb.logConversations,  JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))
};

// 用户输入可复制及二次更改
const questionAction = async (userInput, type) => {
    if (type === 'copy') {
        // 调用后端接口，将用户输入复制到剪贴板
        Qrequest(chatQWeb.copyReplyText, userInput.questionDisplayContent);
        handleShowTip(store.loadTranslations['Copied successfully'])
    } else if (type === 'reEdit') {
        let lostFileLists = []
        if (userInput.fileList.length > 0) {
            for (let index = 0; index < userInput.fileList.length; index++) {
                const isExist = await Qrequest(chatQWeb.isFileExist, userInput.fileList[index].filePath)
                if (!isExist) {
                    lostFileLists.push(userInput.fileList[index])
                }
            }
            if (lostFileLists.length > 0) {
                // 提示用户文件不存在
                userInput.fileList = userInput.fileList.filter(item => !lostFileLists.some(lostItem => lostItem.index === item.index))
                let ret = await Qrequest(chatQWeb.showLostFileWarningDlg, JSON.stringify(lostFileLists))  // 后端提示用户文件不存在
                if (!ret) {
                    return
                }
            }
        }

        // 清空输入框
        question.value = ''
        // 清空文件列表
        isShowFile.value = false
        isFileInInput.value = false
        isFileParsingSuccess.value = false
        isAllParsingStatusEnd.value = false
        inputFileList.value = []
        extensionFileList.value = []
        // 清空当前指令标签
        showPromptTag.value = false
        promptInfo.value = {}
        showPromptTag.value = false

        // 文件还原
        if (userInput.fileList.length > 0) {
            if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
                // AI写作助手：根据fileCategory分类还原
                const outlineFilesToRestore = userInput.fileList.filter(f => f.fileCategory === store.DocFileCategory.FileOutline)
                const materialFilesToRestore = userInput.fileList.filter(f => f.fileCategory === store.DocFileCategory.LocalMaterial)

                // 还原大纲文件
                outlineFilesToRestore.forEach(file => {
                    const filesData = JSON.stringify([{
                        filePath: file.filePath,
                        fileIcon: file.imgBase64
                    }])
                    sigDocSummaryForOffice(filesData, store.DocParsingStatusType.Success, store.DocFileCategory.FileOutline)
                })

                // 还原本地素材文件
                if (materialFilesToRestore.length > 0) {
                    const filesData = JSON.stringify(materialFilesToRestore.map(f => ({
                        filePath: f.filePath,
                        fileIcon: f.imgBase64
                    })))
                    sigDocSummaryForOffice(filesData, store.DocParsingStatusType.Success, store.DocFileCategory.LocalMaterial)
                }
            } else {
                // 其他助手：使用原有逻辑
                let existFileLists = userInput.fileList.map(item => item.filePath)
                Qrequest(chatQWeb.editQuestionToFileSummary, existFileLists)
            }
        }

        // 指令还原
        if (Object.keys(userInput.promptInfo).length > 0) {
            let promptInfo = promptInfos.value.find(item => item.tagType === userInput.promptInfo.tagType)
            // 如果promptInfo为空，则提示用户指令不存在
            if (promptInfo) {
                // 还原指令标签
                sigSelectOnePrompt(promptInfo)
                // 清空当前mcp勾选状态
                store.IsOpenMcpServer = false
                // 取消当前知识库勾选
                store.IsOpenKnowledgeBase = false
            }
        }

        // 文字还原
        question.value = userInput.questionDisplayContent
    }
}

// 更新大纲
const OutlineContentEx = ref("")  // 大纲内容扩展， 包含标题和段落
const isLastHasOutline = ref(false)  //是否显示生成内容按钮
const isShowGenContentBtn = computed (() => {
    if (!(isLastHasOutline.value && currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT && history.value.length > 0)) {
        OutlineContentEx.value = ""
    }
    return isLastHasOutline.value && currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT && history.value.length > 0
})

const clickBaseOutlineGenContent = async () => {
    question.value = store.loadTranslations['Outline to Docs']
    await sendQuestion(store.SendArg.SendDocFileOutline)
    isLastHasOutline.value = false
    OutlineContentEx.value = ""
}

const updateOutline = (historyActiveIndex, newTitle, newParagraph) => {
    let answerDisplayMsg = JSON.parse(_.last(history.value).answers[historyActiveIndex].displayContent)
    // 找到answerDisplayMsg中chatType == sotre.ChatType.Outline的元素
    let outlineItem = answerDisplayMsg.find(item => item.chatType === store.ChatAction.ChatOutline)
    
    if (outlineItem) {
        let outlineItemIndex = answerDisplayMsg.indexOf(outlineItem)
        let outlineItemObj = JSON.parse(outlineItem.content)
        
        // 更新标题
        outlineItemObj.title = newTitle
        // 更新段落内容
        outlineItemObj.content = newParagraph
        // 更新大纲内容
        OutlineContentEx.value = JSON.stringify({title: newTitle, content: newParagraph})
        // 最后一个有大纲
        isLastHasOutline.value = true

        // 更新displayContent
        outlineItem.content = JSON.stringify(outlineItemObj)
        
        answerDisplayMsg[outlineItemIndex] = outlineItem
    }
    _.last(history.value).answers[historyActiveIndex].displayContent = JSON.stringify(answerDisplayMsg)
    // 存历史记录
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))
}

// 更新activeIndex
const activeIndex = ref(0)
const updateActiveIndex = (newActiveIndex) => {
    activeIndex.value = newActiveIndex
    try {
        const displayContentLast = JSON.parse(_.last(history.value).answers[newActiveIndex].displayContent)
        if (displayContentLast.find(item => item.chatType === store.ChatAction.ChatOutline)) {
            isLastHasOutline.value = true
        } else {
            isLastHasOutline.value = false
        }
    } catch (error) {
        
    }
}

const handleGuessYouWantClick = (item) => {
    question.value = item
    sendQuestion()
}

// ===============================================================================================
// markdown编辑器
const showMarkdownEditor = ref(false)
const mdEditorContent = ref({})
const openMarkdownEditor = (content) => {
    mdEditorContent.value = content
    showMarkdownEditor.value = true
}

const closeMarkdownEditor = () => {
    showMarkdownEditor.value = false
}

const markdownContent = ref('')
const saveMarkdownContent = async (saveData) => {
    // 保存 markdown 内容到历史记录
    console.log('保存 markdown 内容:', saveData)
    
    const { id, title, content } = saveData
    
    // 遍历历史记录，找到包含该文档的记录并更新
    for (let i = 0; i < history.value.length; i++) {
        const historyItem = history.value[i]
        
        for (let j = 0; j < historyItem.answers.length; j++) {
            const answer = historyItem.answers[j]
            
            try {
                const displayContent = JSON.parse(answer.displayContent)
                
                // 查找 ChatDocCard 类型的内容
                for (let k = 0; k < displayContent.length; k++) {
                    const item = displayContent[k]
                    
                    if (item.chatType === store.ChatAction.ChatDocCard && item.content?.id === id) {
                        // 找到了，更新内容
                        item.content.content = content
                        
                        // 更新 displayContent
                        answer.displayContent = JSON.stringify(displayContent)
                        
                        // 保存到后端
                        await Qrequest(
                            chatQWeb.logCurrentConversations,
                            currentAssistant.value.id,
                            lastConversationInfo.value.conversationId,
                            currentAssistant.value.displayname,
                            JSON.stringify(history.value)
                        )
                        
                        console.log('保存成功:', title)
                        return
                    }
                }
            } catch (error) {
                console.warn('解析 displayContent 失败:', error)
            }
        }
    }
    
    console.warn('未找到对应的文档记录:', id)
}
// ===============================================================================================

// 会话记录添加状态控制
const showConversionList = ref(false);
const historyList = ref(null);
const alertMessage = ref('确定要永久删除此聊天记录吗？');
// const currentConversationId = ref(null);
const createNewConversation= async () => {
    if (disabled.value || recording.value) return
    await Qrequest(chatQWeb.stopPlayTextAudio)  //停止播放

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    await Qrequest(chatQWeb.createNewConversation)
    await getNowConversationInfo(currentAssistant.value.id)  //设置当前助手的最后一个会话信息
    //新版查询历史记录
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)
    const found = store.CurrentAssistantFunctionButtonActiveIndex.find(item => item.assistantId === currentAssistant.value.type)
    if (found) {
        found.index = -1
        functionButtonsRef.value.resetActiveIndex()
    }

    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT && question.value === '') {
        question.value = store.loadTranslations["I am [Enter Identity/Position]. Please help me write a [Report/Article/Outline/WeChat Official Account Post/Notice/Research Report/Work Summary/Speech] on [Enter Theme], with a length of about [1000 words]. The content requirements are [Enter Requirements/Content Focus/Content Style, etc.]."]
        Qrequest(chatQWeb.setDigitalImageDisable, true);  // 禁用数字形象
    } else {
        Qrequest(chatQWeb.setDigitalImageDisable, false);  // 启用数字形象
    }
}

const conversationModeIconRef = ref(null)
// 会话模式选择
const showConversionMode = ref(false);
const changeConversationMode = async (event) => {
    if (disabled.value || recording.value) return  //回答中或录音中
    event.stopPropagation()
    showConversionMode.value = !showConversionMode.value
}

// 关闭会话模式选择
const closeConversionMode = () => {
    showConversionMode.value = false
}

// 选择会话模式
const selectConversionMode = async (mode) => {
    store.ConversationModeStatus = mode

    // 创建新会话
    createNewConversation()

    // 输入框聚焦
    nextTick(() => {
        questionInput.value.focus()
    })
}

const openHistoryList =  async () => {
    if(disabled.value) return  //回答中
    if(recording.value) return  //录音中
    showConversionList.value = true;
    const historyList_ = await  Qrequest(chatQWeb.getConversationHistoryList)
    historyList.value = JSON.parse(historyList_)
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

const closeHistoryList = () => {
    showConversionList.value = false
    Qrequest(chatQWeb.setTitleBarStatus, false)  //设置当前助手的最后一个会话信息
}

const selectHistoryItem = async (item) => {        // 新增助手存在性检查
    if (assistantList.value.some(a => a.id === item.assistantId)) {
        //切换助手和模型
        assistantList.value.forEach(element => {
            if (element.id === item.assistantId) {
                element.active = true
                currentAssistant.value.active = false
                currentAssistant.value = element
            }
        });
    }else{
        //当前助手被删除，弹窗提示
        alertMessage.value = store.loadTranslations['The %1 agent used in this conversation has been deleted'] + "_&_" + store.loadTranslations['This conversation cannot be viewed. To view it, please install the %1 agent and try again.']
        alertMessage.value = alertMessage.value.replace(/%1/g, item.assistantDisplayName)
        await Qrequest(chatQWeb.showWarningDialog, item.assistantId, item.conversationId, alertMessage.value, false, false, false)
        return
    }

    //输入框内容清空
    question.value = ''

    //切换助手，聊天窗口存在文件则删除
    if(isFileInInput.value){
        isShowFile.value = false
        isFileInInput.value = false
        isFileParsingSuccess.value = false
        isAllParsingStatusEnd.value = false

        inputFileList.value = []
        extensionFileList.value = []
        await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
    }

    //切换助手，聊天窗口存在标签则删除
    if (showPromptTag.value) {
        showPromptTag.value = false
        showPropmptList.value = false
    }

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    store.ConversationModeStatus = store.ConversionMode.Normal
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    //新版查询历史记录
    await Qrequest(chatQWeb.setCurrentConversationId, item.assistantId, item.conversationId)  //设置当前助手的最后一个会话信息
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)
    lastConversationInfo.value = item


    // 判断选中的历史记录最后一条是否有大纲
    try {
        const displayContentLast = JSON.parse(_.last(_.last(history.value).answers).displayContent)
        if (displayContentLast.find(item => item.chatType === store.ChatAction.ChatOutline)) {
            isLastHasOutline.value = true
        } else {
            isLastHasOutline.value = false
        }
    } catch (error) {
        
    }

    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        Qrequest(chatQWeb.setDigitalImageDisable, true);  // 禁用数字形象
    } else {
        Qrequest(chatQWeb.setDigitalImageDisable, false);  // 启用数字形象
    }

    if (accountList.value.some(a => a.id === _.last(_.last(history.value).answers).llmId)) {
        accountList.value.forEach(element => {
            if (element.id === _.last(_.last(history.value).answers).llmId) {
                element.active = true
                currentAccount.value.active = false
                currentAccount.value = element
            }
        });
        /**
         * TODO:
         * 查询当前助手指令列表
         */
        await Qrequest(chatQWeb.setCurrentLLMAccountId, currentAccount.value.id)
        currentAccountChanged()
    }else{
        alertMessage.value = store.loadTranslations['The original conversation model has been deleted. We have switched to a new model for you to continue the conversation.']
        await Qrequest(chatQWeb.showWarningDialog, item.assistantId, item.conversationId, alertMessage.value, false, true, false)
        // 当前模型被删除时，自动切换第一个可用模型
        if (accountList.value.length > 0) {
            const firstAccount = accountList.value[0]
            firstAccount.active = true
            currentAccount.value.active = false
            currentAccount.value = firstAccount
        }
        return
    }
    Qrequest(chatQWeb.setTitleBarStatus, false)  //设置当前助手的最后一个会话信息
    showConversionList.value = false
}

const updateCurrentAssistant = (item) => {
    switchModel.value.clickAssistantItem(item)
}

const showAssistantList = () => {
    switchModel.value.clickAssistantSwitch()
}

const showAlertDialog = async (item) => {
    alertMessage.value = store.loadTranslations['Are you sure to delete the conversation? It will be unrecoverable once deleted.']
    await Qrequest(chatQWeb.showWarningDialog, item.assistantId, item.conversationId, alertMessage.value, true, false, false)
}

const showClearHistoryAlertDialog = async () => {
    alertMessage.value = store.loadTranslations['Delete all records?'] + "_&_" + store.loadTranslations['Once deleted, the content cannot be recovered!']
    let ret = await Qrequest(chatQWeb.showWarningDialog, currentAssistant.value.id, lastConversationInfo.value.conversationId, alertMessage.value, false, false, true)
    if (ret) {
        closeHistoryList()
    }
}
//*********************************************

// 写作、文本处理智能体功能按钮
const functionButtonsRef = ref(null)
const functionList = ref([])
const shortcutList = ref([])
const handleFunctionSelect = async (item) => {

    if (history.value.length == 0) {
        //获取随机问题
        const questions = await Qrequest(chatQWeb.getAiFAQByFunction, currentAssistant.value.type, item.Function)
        if (questions != "") {
            shortcutList.value = JSON.parse(questions)
        }
    }

    let quesionModel = await Qrequest(chatQWeb.getFunctionTemplate, currentAssistant.value.type, item.Function, "")
    if (quesionModel !== "") {
        question.value = quesionModel
    }

    nextTick(() => {
        questionInput.value.focus()
    })
}
const handleFunctionClear = async () => {
    question.value = ""
    welcomePageRef.value.getAiFAQ()
}

// 是否领取过免费模型
const isGotDeepSeekUosFree = ref(false)
const guideConfigStack = [
    {
        id: 'new-ai-writing-guide', // Unique ID for the new sequential guide.
        factory: () => {
            let firstStepContent = store.loadTranslations['1.Reference local materials and outlines for more accurate content.'] + '\n' + store.loadTranslations['2.Supports local models, ensuring security and peace of mind.'] + '\n' + store.loadTranslations['3.Traceable sources, reliable data.'] + '\n' + store.loadTranslations['4.Edit while writing, export when satisfied.'];
            const steps = [
                {
                    title: store.loadTranslations['AI Writing Agent Fully Upgraded'],
                    content: firstStepContent,
                    useLaterText: store.loadTranslations['Try Later'],
                    activeText: store.loadTranslations['Try Now'],
                    targetRef: 'none',
                    targets: [],  // Will be set dynamically
                    primaryTarget: null,  // Will be set dynamically
                    isAllowClickOnTarget: false,
                    relativePosition: isWindowMode.value ? { // 窗口模式左边，侧边栏模式上边
                        position: 'none',
                        arrowDirect: 'right',
                        offset: 25
                    } : {},
                    width: 320,
                    onActiveClick: async () => {
                        // 1. 关闭助手列表
                        switchModel.value.showAssistantMenu = false

                        // 2. 切换到写作助手
                        const writingAssistant = assistantList.value.find(
                            item => item.type === store.AssistantType.AI_WRITING_ASSISTANT
                        )
                        if (writingAssistant) {
                            await switchModel.value.clickAssistantItem(writingAssistant)
                            await Qrequest(chatQWeb.setCurrentAssistantId, writingAssistant.id)
                        }

                        // 3. 等待DOM更新
                        await nextTick()

                        // 4. 打开inputFilesMenu
                        // 通过点击按钮来打开菜单
                        if (fileButtonWithMenuRef.value) {
                            fileButtonWithMenuRef.value.btnRef?.click()
                        }

                        // 5. 等待菜单渲染完成
                        await nextTick()

                        // 6. 获取第二步的目标元素
                        const inputFilesIconElement = fileButtonWithMenuRef.value?.btnRef
                        const inputFilesMenuElement = fileButtonWithMenuRef.value?.inputFilesMenuRef?.$el ||
                            (fileButtonWithMenuRef.value?.inputFilesMenuRef && fileButtonWithMenuRef.value.inputFilesMenuRef.querySelector('.input-files-menu'))

                        // 7. 更新第二步的targets
                        if (guideConfig.value.steps[1]) {
                            guideConfig.value.steps[1].targets = [inputFilesIconElement, inputFilesMenuElement].filter(Boolean)
                            guideConfig.value.steps[1].primaryTarget = inputFilesMenuElement
                        }

                        // 8. 清理guideActiveAssistantId，恢复助手列表交互
                        guideActiveAssistantId.value = null
                    }
                },
                {
                    title: store.loadTranslations['AI Writing Agent Fully Upgraded'],
                    content: store.loadTranslations['Generate reports based or outline files for greater accuracy'],
                    activeText: store.loadTranslations['Start Now'],
                    targetRef: 'none',
                    targets: [],  // Will be set dynamically
                    primaryTarget: null,  // Will be set dynamically
                    arrowDisplay: 'none',
                    isAllowClickOnTarget: false,
                    onActiveClick: () => {
                        fileButtonWithMenuRef.value?.closeMenu()
                        onGuideClose()
                    }
                }
            ]

            return {
                guideType: 'independent',
                steps
            };
        }
    },
    {
        id: 'auto-mcp-guide', // Unique ID for the new sequential guide.
        factory: () => {
            const steps = [
                {
                    title: store.loadTranslations['MCP Server Upgrade to Automatic Mode'],
                    content: [
                        { type: 'text', value: store.loadTranslations['MCP Server have been upgraded to automatic mode, allowing you to access all MCP Server with just click '] },
                        { type: 'icon', value: 'mcp-switch' },
                        { type: 'text', value: store.loadTranslations['. This allows you to automate tasks like system setup and file processing with just one click.'] }
                    ],
                    activeText: store.loadTranslations['Next'] ,
                    targetRef: mcpServerIconRef.value,
                    isAllowClickOnTarget: false,
                    onActiveClick: () => {
                        
                    },
                },
                {
                    title: store.loadTranslations['Adding MCP Server has been moved to Settings.'],
                    content: store.loadTranslations['To add more MCP Server, go to Settings > MCP Server.'],
                    activeText: store.loadTranslations['Next'] ,
                    targetRef: "none",
                    isAllowClickOnTarget: false,
                    relativePosition: {
                        arrowDirect: "top",  // 指定箭头方向
                        left: 'auto',
                        right: '10px',
                        top: '10px',
                        bottom: 'auto',
                        titleBarBtnWidth: titleBarBtnWidth.value,
                    },
                    onActiveClick: () => {
                        
                    },
                },
                {
                    title: store.loadTranslations['Complimentary Model Credits'],
                    content: store.loadTranslations['The current system offers the DeepSeek trial account model, which automatically refreshes the free quota at the beginning of each month, allowing you to use it worry-free.'],
                    activeText: isGotDeepSeekUosFree.value? store.loadTranslations['Claim Credits'] : store.loadTranslations['Get a free account'],
                    targetRef: "none",
                    isAllowClickOnTarget: false,
                    relativePosition: {
                        position: 'center',  // 是否居中显示
                        arrowDisplay: 'none',  // 隐藏箭头
                    },
                    onActiveClick: async () => {
                        onGuideClose()
                        //记数据库，不再弹起引导界面
                        Qrequest(chatQWeb.updateUpdateFreeAccountGuideDB, true)
                        if (isGotDeepSeekUosFree.value) {
                            // 领取免费额度
                            await getFreeCredits(false)
                        } else {
                            Qrequest(chatQWeb.launchLLMConfigWindowAndGetFreeDialog)
                        }

                    },
                }
            ];

            AutoMcpSteps.value = steps

            return {
                guideType: 'sequential',
                steps
            };
        }
    },
    {
        id: 'private-guide', // Unique ID for the new sequential guide.
        factory: () => {
            const steps = [
                {
                    title: store.loadTranslations['Add Private Chat'],
                    content: store.loadTranslations['Add [ Private Chat Mode ] - Chats will not be saved.'],
                    activeText: isEnableScreenshot.value > -1 ? store.loadTranslations['Next'] : store.loadTranslations['OK'],
                    targetRef: conversationModeIconRef.value,
                    isAllowClickOnTarget: false,
                    onActiveClick: () => {
                        if (isEnableScreenshot.value === -1) {
                            onGuideClose()
                        }
                    },
                }
            ];

            // 只有当启用截图功能时，才添加第二步
            if (isEnableScreenshot.value > -1) {
                steps.push({
                    title: store.loadTranslations['Add [Screenshot Q&A]'],
                    content: store.loadTranslations['Take a screenshot and send the content to UOS AI. You can also upload an image directly.'],
                    activeText: store.loadTranslations['OK'],
                    targetRef: screenShotIconRef.value,
                    isAllowClickOnTarget: false,
                    onActiveClick: onGuideClose,
                });
            }

            return {
                guideType: 'sequential',
                steps
            };
        }
    },
    {
        id: 'mcp-guide', // Unique ID for the current latest guide.
        factory: () => ({
            guideType: 'independent',
            steps: [
                {
                    title: store.loadTranslations['Add Mcp Server[GuidePage]']?.replace('[GuidePage]', ''),
                    content: store.loadTranslations['Automate multi-file and multi-app tasks with one command using MCP Service. Try it now!'],
                    contentText: store.loadTranslations['First-time users: Install MCP environment \"UOS AI Agent\" via App Store.'],
                    useLaterText: store.loadTranslations['Use later'],
                    activeText: store.loadTranslations['Install Now'],
                    targetRef: mcpServerIconRef.value,
                    isAllowClickOnTarget: false,
                    onActiveClick: () => Qrequest(chatQWeb.installUosAiAgent)
                },
                {
                    title: store.loadTranslations['Enable MCP Server'],
                    content: [
                        { type: 'text', value: store.loadTranslations['After installing the MCP environment \"UOS AI Agent\", click the '] },
                        { type: 'icon', value: 'mcp-switch' },
                        { type: 'text', value: store.loadTranslations[' and select \"uos-mcp\" in the MCP server list.'] }
                    ],
                    contentText: store.loadTranslations['Try saying: \"Change system to dark mode\".'],
                    useLaterText: store.loadTranslations['Use later'],
                    activeText: store.loadTranslations['Try it now'],
                    targetRef: mcpServerIconRef.value,
                    isAllowClickOnTarget: true,
                    onActiveClick: () => clickMcpServerIcon()
                }
            ]
        })
    }
];

// 领取免费额度
const getFreeCredits = async(isShowDlg) => {
    await hideTooltip()

    setTimeout(async () => {
        const ret = await Qrequest(chatQWeb.getFreeCredits, isShowDlg)
    }, 50)
}

// 新增智能体引导界面
const showGuide = ref(false)
const guideConfig = ref({
    steps: [],
    guideType: 'independent'
})

// Guide control state variables
const guideActiveAssistantId = ref(null)

const onGuideClose = (isTryClicked) => {
    // isTryClicked 在新逻辑中不再由 GuideOverlay 直接提供，
    // 因为具体行为已移入 onActiveClick。
    // 这里我们只负责关闭。
    showGuide.value = false
    Qrequest(chatQWeb.setTitleBarStatus, false)  //关闭标题栏遮罩
    //记数据库，不再弹起引导界面
    Qrequest(chatQWeb.updateUpdatePromptDB, true)

    // 清理助手引导相关状态
    if (guideActiveAssistantId.value) {
        guideActiveAssistantId.value = null
    }

    // 关闭inputFilesMenu
    fileButtonWithMenuRef.value?.closeMenu()

    // 把目标元素的层级设置为1
    mcpServerIconRef.value?.style.setProperty('z-index', '1')
    // 把目标元素的pointerEvents属性设置为auto
    mcpServerIconRef.value?.style.setProperty('pointer-events', 'auto')

    nextTick(() => {
        questionInput.value.focus();
    });
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

// 处理可能的多信号叠加情况
// 上次接收到的message对象
const lastMsg = ref({chatType: store.ChatAction.ChatNone})
const processSingleMessage = (msgValue, errCode) => {
    try {
        replyMsg.value = JSON.parse(msgValue);

        // 如果answerDisplayMsg为空，且当前chatType为工具调用且status不为调用中，则直接返回
        if (answerDisplayMsg.value.length == 0 && 
            (replyMsg.value.message.chatType == store.ChatAction.ChatToolUse || 
             replyMsg.value.message.chatType == store.ChatAction.AgentAction) && 
            replyMsg.value.message.status != store.ToolUseStatus.Calling) {
            return true;
        }

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
        if((lastMsg.value.chatType !== replyMsg.value.message.chatType && 
            replyMsg.value.message.chatType !== store.ChatAction.ChatToolUse &&
            replyMsg.value.message.chatType !== store.ChatAction.AgentAction &&
            replyMsg.value.message.chatType !== store.ChatAction.AgentReasonTitle)
        || (replyMsg.value.message.chatType  === store.ChatAction.ChatToolUse && replyMsg.value.message.status === store.ToolUseStatus.Calling)
        || (replyMsg.value.message.chatType  === store.ChatAction.AgentAction && replyMsg.value.message.status === store.ToolUseStatus.Calling)
        || (replyMsg.value.message.chatType  === store.ChatAction.ChatOutline)
        || (replyMsg.value.message.chatType  === store.ChatAction.ChatDocCard)
        ){
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
            if (replyMsg.value.message.chatType  === store.ChatAction.ChatOutline) {
                OutlineContentEx.value = replyMsg.value.message.content
            }
        }

        // 如果发送完成后，有大纲，则显示根据大纲生成内容的按钮
        if (errCode !== 0 && answerDisplayMsg.value.find((item) => item.chatType === store.ChatAction.ChatOutline)){
            isLastHasOutline.value = true
        }

        /**
         * ++的情况
         * 思考内容，上一个也是思考内容
         * 正文内容，上一个也是正文内容
         */
        if (replyMsg.value.message.chatType === lastMsg.value.chatType && 
            replyMsg.value.message.chatType !== store.ChatAction.ChatToolUse &&
            replyMsg.value.message.chatType !== store.ChatAction.AgentAction&&
            replyMsg.value.message.chatType !== store.ChatAction.AgentReasonTitle) {
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
         * 替换的情况 - AgentReasonTitle
         * 当answerDisplayMsg中不存在chatType === store.ChatAction.AgentReasonTitle的message时，
         * 将该message push进answerDisplayMsg，若存在，则替换成replyMsg
         */
        if (replyMsg.value.message.chatType === store.ChatAction.AgentReasonTitle) {
            const existingIndex = answerDisplayMsg.value.findIndex(item =>
                item.chatType === store.ChatAction.AgentReasonTitle
            );
            
            if (existingIndex === -1) {
                // 不存在，push进answerDisplayMsg
                answerDisplayMsg.value.push(replyMsg.value.message);
            } else {
                // 存在，替换成replyMsg
                if (replyMsg.value.message.content !== "") {
                    answerDisplayMsg.value[existingIndex] = replyMsg.value.message;
                } else {
                    answerDisplayMsg.value[existingIndex].status = replyMsg.value.message.status;
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

        /**
         * 修改AgentAction状态的情况
         */
        if (replyMsg.value.message.chatType === store.ChatAction.AgentAction ) {
            for (let index = 0; index < answerDisplayMsg.value.length; index++) {
                // 根据index和name更新AgentAction状态
                if (answerDisplayMsg.value[index].index === replyMsg.value.message.index && answerDisplayMsg.value[index].name === replyMsg.value.message.name) {
                    answerDisplayMsg.value[index] = replyMsg.value.message
                }
            }
        }

        _.last(_.last(history.value).answers).displayContent = JSON.stringify(answerDisplayMsg.value)

        lastMsg.value = replyMsg.value.message

        return true;
    } catch (e) {
        Qrequest(chatQWeb.writeVueLog, store.LogLevel.CRITICAL, "index.vue processSingleMessage error:" + e.message)
        return false;
    }
};

const sigAiReplyStream = async (type, value, status) => {
    if (showStop.value === false) 
        return
    // 初始化大纲状态
    isLastHasOutline.value = false
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
    if (status !== 0) {
        const { icon, displayname } = currentAccount.value
        _.last(_.last(history.value).answers).reqId = _.last(history.value).question.reqId
        // _.last(_.last(history.value).answers).llmIcon = icon
        // _.last(_.last(history.value).answers).llmName = displayname
        // _.last(_.last(history.value).answers).llmId = currentAccount.value.id
        _.last(_.last(history.value).answers).assistantId = currentAssistant.value.id
        _.last(_.last(history.value).answers).assistantName =  currentAssistant.value.displayname,
        _.last(_.last(history.value).answers).errCode = status
        _.last(_.last(history.value).answers).errInfo = JSON.stringify(judgeInstallOrConfig(status, _.last(_.last(history.value).answers).knowledgeSearchStatus))

        if(likeOrNotAgent.includes(currentAssistant.value.id)){
            //填充点赞踩结束时间
            let ext = JSON.parse(_.last(_.last(history.value).answers).extention)
            let answer = {
                answer: _.last(_.last(_.last(history.value).answers).displayContent).content,
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

         //存日志
        //  Qrequest(chatQWeb.logConversations,  JSON.stringify(history.value))
         Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

         /**
         * TODO
         * 发送完成，需要复制应答的answer
         */

        showStop.value = false
        isRetry.value = false
        isThinking.value = false
        answerDisplayMsg.value = []
        lastMsg.value = {chatType: store.ChatAction.ChatNone}
    }

    handelScrol()
}

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
    if(!isLLMExist.value){
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

const isMouseScroll = ref(false)  //是否鼠标滚动
const showReturnBottom = ref(false)  //是否显示返回按钮
const handleHistoryScroll = () => {
    setTimeout(() => {
       if (chatHistory.scrollHeight - chatHistory.clientHeight > chatHistory.scrollTop + 50) {
            isMouseScroll.value = true
            showReturnBottom.value = true
        }else {
            isMouseScroll.value = false
            showReturnBottom.value = false
        }
    }, 20)
}

const returnBottom = () => {
    isMouseScroll.value = false
    showReturnBottom.value = false
    handelScrol()
}

const handelScrol = () => {
    if (isMouseScroll.value) {
        return
    }

    let chatHistory = document.getElementById('chatHistory')
    setTimeout(() => {
        if (chatHistory.scrollHeight > chatHistory.clientHeight) {
            setTimeout(() => {
                chatHistory.scrollTop = chatHistory.scrollHeight;
                showReturnBottom.value = false
            }, 0);
        }else {
            setTimeout(() => {
                showReturnBottom.value = true
            }, 20);
        }
        handleHistoryScroll()
    }, 20);



}

const stopRequest = async () => {
    await Qrequest(chatQWeb.cancelAiRequest, talkID.value)

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

    _.last(_.last(history.value).answers).errCode = 298  //请求被取消

    //点击停止时，任务状态置为failed
    try{
        let displayContent = JSON.parse(_.last(_.last(history.value).answers).displayContent)
        let agentReasonTitleItem  = displayContent.find(item => item.chatType == store.ChatAction.AgentReasonTitle)
        if (agentReasonTitleItem && agentReasonTitleItem.status == store.TitleStatus.InProgress){
            agentReasonTitleItem.status = store.TitleStatus.Failed
        }
        _.last(_.last(history.value).answers).displayContent = JSON.stringify(displayContent)
        }
    catch(e){
        
    }
    

    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

    showStop.value = false
    isRetry.value = false
    isFoundThinkStart.value = false
    isFoundThinkStop.value = false

    isThinking.value = false
    answerDisplayMsg.value = []
    lastMsg.value = {chatType: store.ChatAction.ChatNone}

    nextTick(() => handelScrol())
}

// 重新生成
const isRetry = ref(false)
const retryRequest = async () => {
    answerDisplayMsg.value = []
    lastMsg.value = {chatType: store.ChatAction.ChatNone}
    _.last(history.value).question.isRetry = true
    const _question = history.value[history.value.length - 1].question
    await Qrequest(chatQWeb.stopPlayTextAudio)
    playAudioID.value = ''
    if (recording.value) sigAudioASRError()
    // 重试时，开关性的按钮以当前状态为准
    try {
        let extention = JSON.parse(_question.extention)
        // store.IsOpenMcpServer为false，则删除extention中element.type == store.ExtentionType.McpBtnStatus的元素
        if ( currentAssistant.value.type == store.AssistantType.UOS_AI) {
            if (!store.IsOpenMcpServer || !store.IsInstallUOSAiAgent) {
                extention = extention.filter(element => element.type != store.ExtentionType.McpBtnStatus)
            }else {
                if (extention.filter(element => element.type == store.ExtentionType.McpBtnStatus).length == 0) {
                    // 如果找不到element.type == store.ExtentionType.McpBtnStatus的元素，则添加一个新的元素
                    extention.push({
                        type: store.ExtentionType.McpBtnStatus,
                        McpServers: true
                    })
                } 
            }
        }

        _question.extention = JSON.stringify(extention)
    }catch(e) {

    }
    // ques添加openThink字段和onlineSearch字段
    // 深度思考开关显示时才添加openThink字段
    if (showDeepThink.value) {
        _question.openThink = store.IsDeepThink
    } else {
        _question.openThink = false
    }

    // 联网搜索开关显示时才添加onlineSearch字段
    if (showSearchOnline.value) {
        _question.onlineSearch = store.IsSearchOnline
    } else {
        _question.onlineSearch = false
    }

    let sendQuestionAccount = currentAccount.value
    const { id, model, icon, displayname } = sendQuestionAccount

    _.last(history.value).answers.push(
        {
            displayContent:"",
            content:"",
            displayHash:"",
            reqId:"0",
            llmIcon: icon ? icon : '',
            llmName: displayname ? displayname : '',
            llmModel: model,
            llmId :  id ? id : '',
            assistantId : currentAssistant.value.id,
            assistantName : currentAssistant.value.displayname,
            errCode:0,
            errInfo:JSON.stringify(judgeInstallOrConfig(0, (store.IsOpenKnowledgeBase && currentAssistant.value.type === store.AssistantType.UOS_AI) ? true : false)),  //补充json
            chatType:0,  //问答、文生图、FunctionCall
            isRetry:true,
            extention:"[]",
            thinkTime: "-1",
            openThink: false,
            onlineSearch: false,
            knowledgeSearchStatus: (store.IsOpenKnowledgeBase && currentAssistant.value.type === store.AssistantType.UOS_AI) ? true : false,  //是否显示知识库搜索
        }
    )

    const quesResp = await Qrequest(chatQWeb.sendRequest, id ? id : '', JSON.stringify(_question))
    try{
        JSON.parse(quesResp)
    }catch(e){
        return
    }

    //开始时间
    isFoundThinkStart.value = false
    isFoundThinkStop.value = false

    _.last(history.value).question = JSON.parse(quesResp)
    _.last(_.last(history.value).answers).reqId = JSON.parse(quesResp).reqId

    //初始化like or not
    initLikeOrNotExt()

    talkID.value = JSON.parse(quesResp).reqId
    isRetry.value = true
    showStop.value = true
    // question.value = ''
}

// 是否显示深度思考开关
const showDeepThink = computed(() => {
    return currentAccount.value.model == store.Uos_Free && (!store.IsOpenMcpServer || !store.IsInstallUOSAiAgent) && (currentAssistant.value.type !== store.AssistantType.AI_WRITING_ASSISTANT)
})

// 是否显示联网搜索开关
const showSearchOnline = computed(() => {
    // 写作助手下需要单独显示联网搜索开关
    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        return true
    }

    return currentAccount.value.model == store.Uos_Free && (!store.IsOpenMcpServer || !store.IsInstallUOSAiAgent)
})
// 深度思考
const deepThinkClick = async () => {
    if (store.IsDeepThink) {
        store.IsDeepThink = false
    }else {
        store.IsDeepThink = true
    }
}

// 联网搜索
const searchOnlineClick = async () => {
    if (store.IsSearchOnline) {
        store.IsSearchOnline = false
    }else {
        store.IsSearchOnline = true
    }
}

const deepThinkWidth = computed(() => {
    if (isWindowMode.value) {
        return isChineseLanguage.value ? '88px' : '120px'
    }else{
        return '30px'
    }
})

const onlineSearchWidth = computed(() => {
    if (isWindowMode.value) {
        return isChineseLanguage.value ? '88px' : '78px'
    }else{
        return '30px'
    }
})


const recording = ref(false)
const startQus = ref('')
const endQus = ref('')

const handleRecorder = async () => {

    console.log("handleRecorder:clicked: currentAccount.displayname:", currentAccount.value.displayname);
    console.log(recording.value)
    if (!isAudioInput.value || showStop.value || !netState.value) return
    if (currentAssistant.value.type == store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist.value || !isEmbeddingPluginsExist.value) && history.value.length === 0) return
    if (currentAssistant.value.type == store.AssistantType.UOS_SYSTEM_ASSISTANT && !isEmbeddingPluginsExist.value && history.value.length === 0) return
    if (currentAssistant.value.type == store.AssistantType.DEEPIN_SYSTEM_ASSISTANT && !isEmbeddingPluginsExist.value && history.value.length === 0) return
    if (recording.value) return sigAudioASRError()
    if (!recording.value) {
        isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable)
        await Qrequest(chatQWeb.stopPlayTextAudio)
        if (isAudioInput.value) {
            await Qrequest(chatQWeb.startRecorder, 0)
            const start = questionInput.value.textarea.selectionStart;
            const end = questionInput.value.textarea.selectionEnd;
            startQus.value = questionInput.value.textarea.value.substr(0, start)
            endQus.value = questionInput.value.textarea.value.substr(end)
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

    /**
     * TODO:
     * 查询当前助手指令列表
     */
     currentAccountChanged()
}

const questionInput = ref(null)
const isAudioInInpit = ref(false)
const sigAudioASRStream = (res, isEnd) => {
    if (!(isEnd && res == "") && recording.value) {
        question.value = startQus.value + res + endQus.value
    }
    // console.log('sigAudioASRStream', res, isEnd)
    if (isEnd) {
        sigAudioASRError()

        //添加指令匹配
        handleInput(question.value)

        isAudioInInpit.value = true

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

const sigAudioASRError = async () => {
    countDown.value = 0
    audioLevel.value = 0
    showCount.value = false
    await Qrequest(chatQWeb.stopRecorder)
    recording.value = false
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
    _.last(_.last(history.value).answers).chatType = type
}

// 接受AI图片信息
const sigText2PicFinish = (id, paths) => {
    _.last(_.last(history.value).answers).displayContent = JSON.stringify(paths)

    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

    handelScrol()
    showStop.value = false
    isRetry.value = false
}
// 接受PPT图片信息
const sigPPTCreateSuccess = (id, paths) => {
    sendEmptyAnswers()
    _.last(_.last(history.value).answers).displayContent = JSON.stringify(paths)
    _.last(_.last(history.value).answers).chatType = store.ChatAction.ChatText2Image
    let extention = []
    extention.push({
            type: store.ExtentionType.PictureId,
        	idValue: id
        })
    _.last(_.last(history.value).answers).extention = JSON.stringify(extention)

    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

    handelScrol()
    showStop.value = false
    isRetry.value = false
}
const sigPPTChangeSuccess = (id, paths) => {
    for (let index = 0; index < history.value.length; index++) {
        for(let answerIndex = 0; answerIndex < history.value.at(index).answers.length; answerIndex++) {
            const ext = JSON.parse(history.value.at(index).answers[answerIndex].extention);
            for(let extIndex = 0; extIndex < ext.length; extIndex++) {
                const element = ext[extIndex];
                if (element.type == store.ExtentionType.PictureId && element.idValue == id) {
                    history.value.at(index).answers[answerIndex].displayContent = JSON.stringify(paths)
                    //存日志
                    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
                    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))
                    break
                }
            }
        }
    }
}
const sigPosterCreateSuccess = (id, paths) => {
    sendEmptyAnswers()
    _.last(_.last(history.value).answers).displayContent = JSON.stringify(paths)
    _.last(_.last(history.value).answers).chatType = store.ChatAction.ChatText2Image
    let extention = []
    extention.push({
            type: store.ExtentionType.PictureId,
        	idValue: id
        })
    _.last(_.last(history.value).answers).extention = JSON.stringify(extention)

    //存日志
    // Qrequest(chatQWeb.logConversations, JSON.stringify(history.value))
    Qrequest(chatQWeb.logCurrentConversations, currentAssistant.value.id, lastConversationInfo.value.conversationId, currentAssistant.value.displayname,  JSON.stringify(history.value))

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
const sigThemeChanged = (res) => {
    updateTheme(res)
    //切换深色主题时，文档图标用深色图标
    if(res == 2){
        isDarkMode.value = true
    }else{
        isDarkMode.value = false
    }
}

const sigFontChanged = (family, pixelSize) => {
    updateFont(family, pixelSize)
}
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

const isModalState = ref(false)
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
    isModalState.value = res
}
const switchModel = ref()
const switchModelRoot = ref(null)
const sigWebchatActiveChanged = (res) => {
    if (!res) {
        // 如果正在进行引导(有 showGuide 或 guideActiveAssistantId)，不关闭菜单
        if (!showGuide.value && !guideActiveAssistantId.value) {
            switchModel.value.showSwitchMenu = false
            switchModel.value.showAssistantMenu = false
        }
        // chatTopRef.value.showSeting = false
    }
    // handleActive(res)
}
const sigWebchatModalityChanged = (res) => handleActive(res)

const sigMainContentBackgroundColor = (color) => {
    updateMainContentBackgroundColor(color);
}

const sigKnowledgeBaseStatusChanged = (status) => {
    console.log("knowledge base status changed: ", status);
    isKnowledgeBaseExist.value = status;
}

const sigEmbeddingPluginsStatusChanged = (status) => {
    console.log("EmbeddingPlugins base status changed: ", status);
    isEmbeddingPluginsExist.value = status;
}

const sigWindowModeChanged = (res) => {
    console.log("onWindowModeChanged, is window mode: ", res);
    isWindowMode.value = res;
}
// 添加按钮引用
const localMaterialBtnRef = ref(null);
const localMaterialsListRef = ref(null);
// 添加本地素材列表显示状态
const showLocalMaterialsList = ref(false)
// 计算属性：区分大纲文件和本地素材文件
const outlineFiles = computed(() => {
    if (currentAssistant.value.type !== store.AssistantType.AI_WRITING_ASSISTANT) {
        return []
    }
    return inputFileList.value.filter(file => {
        // 大纲文件
        return file.fileCategory === store.DocFileCategory.FileOutline
    })
})

// 处理删除本地素材文件
const handleDeleteMaterialFile = async (index) => {
    const materialFile = materialFiles.value.find(item => item.index === index)
    if (materialFile) {
        const msg = store.loadTranslations['Confirm deletion of this reference material?']
        let ret = await Qrequest(chatQWeb.showRemoveFileDialog, msg)
        if (!ret)
            return

        // 使用文件的唯一index作为删除key
        deleteMaterialFile(materialFile.index)

        // 获取LocalMaterialsList组件引用并更新位置
        nextTick(() => {
            localMaterialsListRef.value?.refreshPosition()
        });
    }
}

const checkStatusBeforeParse = (status) => {
    if(disabled.value) return false  //回答中
    if(recording.value) return false  //录音中
    if(showGuide.value) return false  //引导页不处理拖拽
    if(currentAssistant.value.type != store.AssistantType.AI_WRITING_ASSISTANT && inputFileList.value.length >= 3) { //文件数量超过3个
        handleShowTip(store.loadTranslations['You can upload up to 3 files or image'])
        return false
    }

    if(currentAssistant.value.type == store.AssistantType.PLUGIN_ASSISTANT) return false  //非UOS AI助手 非PPT助手
    switch (status) {
        case store.DocParsingStatusType.Success:
            break
        case store.DocParsingStatusType.FileCountError:
            handleShowTip(store.loadTranslations['You can upload up to 3 files or image'])
            return false
        case store.DocParsingStatusType.SuffixError:
            handleShowTip(store.loadTranslations['The file format is not supported.'])
            return false
        case store.DocParsingStatusType.NoDocError:
            handleShowTip(store.loadTranslations['The file format is not supported.'])
            return false
        case store.DocParsingStatusType.ExceedSize:
            handleShowTip(store.loadTranslations['The file size exceeds the 100MB limit.'])
            return false
        case store.DocParsingStatusType.ImageExceedSize:
            handleShowTip(store.loadTranslations['Image size exceeds 15 MB'])
            return false
    }

    return true
}

const sigDocSummaryForOffice = async (filesData_, status, category) => {
    isDragging.value = false
    if (!checkStatusBeforeParse(status)) return

    try {
        // 解析JSON格式的文件数据
        const filesData = JSON.parse(filesData_)
        
        // 检查是否是数组格式
        if (!Array.isArray(filesData)) {
            console.error("Invalid files data format, expected array")
            return
        }
        
        // 处理每个文件
        for (const fileData of filesData) {
            if (category == store.DocFileCategory.LocalMaterial && materialFiles.value.length >= 10) {
                handleShowTip(store.loadTranslations['Supports uploading up to 10 local materials'])
                break
            }

            if (category == store.DocFileCategory.FileOutline && outlineFiles.value.length >= 1) {
                handleShowTip(store.loadTranslations['Only supports uploading 1 outline file'])
                break
            }

            const index = generateFileIndex()
            let file = {
                type: store.DocParsingFileType.Doc,  // 文件类型
                index: index,
                filePath: fileData.filePath,  // 文件路径
                fileNameText: fileData.filePath.substring(fileData.filePath.lastIndexOf('/') + 1), // 文件名
                imgBase64: fileData.fileIcon,  // 文件图标
                docContent: "",  // 文件内容
                isEnabledMouthOver: true,  // 是否启用hover事件
                isShowParsingStatus: false,  // 是否显示解析状态
                isParsingStatusEnd: true,  // 是否解析结束
                parsingStatusText: "",  // 解析状态文本
                isFileParsingSuccess: true,  // 是否解析成功
                fileCategory: category  // 文件分类 0:素材文件 1:模板文件
            }
            
            // 根据文件后缀名判断文件类型
            const suffix = fileData.filePath.substring(fileData.filePath.lastIndexOf('.') + 1)
            if(suffix == 'jpg' || suffix == 'png' || suffix == 'jpeg'){
                file.type = store.DocParsingFileType.Image
            } else {
                file.type = store.DocParsingFileType.Doc
            }

            inputFileList.value.push(file)
            // 设置文件输入状态
            isFileInInput.value = true
        }

        await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)
        docParsingPlaceHolder.value = store.loadTranslations['Summarize the key content of the file.']
        isAllParsingStatusEnd.value = true
        isFileParsingSuccess.value = true
    } catch (error) {
        console.error("Failed to parse files data:", error)
        console.error("Raw files data:", filesData_)
    }
}

const sigDocSummaryParsingStart = async (filePath_, iconPath, defaultPrompt, status) => {
    isDragging.value = false;
    if (!checkStatusBeforeParse(status)) return

    // 输入框中文件列表
    const index = Date.now().toString()

    let file = {
        type: store.DocParsingFileType.Doc,  // 文件类型
        index: index,
        filePath: filePath_,  // 文件路径
        fileNameText: filePath_.substring(filePath_.lastIndexOf('/') + 1),  // 文件名
        imgBase64:iconPath,  // 文件图标
        docContent: "",  // 文件内容
        isEnabledMouthOver: true,  // 是否启用hover事件
        isShowParsingStatus: true,  // 是否显示解析状态
        isParsingStatusEnd: false,  // 是否解析结束
        parsingStatusText: store.loadTranslations['Parsing...'],  // 解析状态文本
        isFileParsingSuccess: false  // 是否解析成功
    }

    // 根据filePath_的后缀名，判断是否为jpg/png
    const suffix = filePath_.substring(filePath_.lastIndexOf('.') + 1)
    if(suffix == 'jpg' || suffix == 'png' || suffix == 'jpeg'){
        file.type = store.DocParsingFileType.Image
    }else{
        file.type = store.DocParsingFileType.Doc
    }

    inputFileList.value.push(file)
    await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件


    /**
     * TODO
     * 1.文件拖入就调用后端接口进行解析
     * 2.解析中和解析失败情况下都将发送按钮置灰
     * 3.添加index字段，用于判断这是第几个文件
     */
    await Qrequest(chatQWeb.onDocSummaryParsing, file.index, file.filePath)  //开始解析

    // 设置文件输入状态
    isFileInInput.value = true
    isAllParsingStatusEnd.value = false  //有文件解析中，isAllParsingStatusEnd.value就为false


    // 如果defaultPrompt不为空，则设置docParsingPlaceHolder为defaultPrompt，并让questionInput.value.focus()
    // 全局搜索带入文档时直接让输入框可编辑
    if (defaultPrompt != '') {
        docParsingPlaceHolder.value = defaultPrompt
        nextTick(() => {
            questionInput.value.focus()  //全局搜索带入文档时直接让输入框可编辑
        })
    }else{
        docParsingPlaceHolder.value = store.loadTranslations['Summarize the key content of the file.']
    }

}

const sigDocSummaryParserResult = (index, status, fileName, docContent_) => {
    // 新的文件解析逻辑
    if(isFileInInput.value){
        // 根据index在inputFileList.value中找到对应的文件
        const file = inputFileList.value.find(item => item.index === index)
        if(status == 0){//解析成功
            if(!file){
                console.log('file not found')
                return  // 文件不存在
            }
            file.isShowParsingStatus = false
            file.docContent = docContent_
            file.isFileParsingSuccess = true
            file.isParsingStatusEnd = true

            let extensionFile = {
                type: file.type,
                index: file.index,
                content: file.docContent,  // 文件内容
                metaInfo: {
                    docPath: file.filePath,  // 文件路径
                    docName: file.fileNameText,  // 文件名
                    iconData: file.imgBase64,  // 文件图标
                }
            }
            extensionFileList.value.push(extensionFile)

            // 按照inputFileList的顺序重新排序extensionFileList
            extensionFileList.value.sort((a, b) => {
                const indexA = inputFileList.value.findIndex(file => file.index === a.index)
                const indexB = inputFileList.value.findIndex(file => file.index === b.index)
                return indexA - indexB
            })
        }else{
            file.isShowParsingStatus = true
            file.isParsingStatusEnd = true
            switch(status){
                case store.DocParserError.ParsingFailed:
                    file.parsingStatusText = store.loadTranslations['File Error']
                    break
                case store.DocParserError.NoTextError:
                    file.parsingStatusText = store.loadTranslations['No text was parsed']
                    break
                case store.DocParserError.NoTextExtracted:
                    file.parsingStatusText = store.loadTranslations['No text extracted']
                    break
                default:
                    break
            }
            file.isFileParsingSuccess = false
        }
    }

    // 循环inputFileList，如果isFileParsingSuccess为false，isFileParsingSuccess.value就为false
    isFileParsingSuccess.value = inputFileList.value.every(file => file.isFileParsingSuccess)
    isAllParsingStatusEnd.value = inputFileList.value.every(file => file.isParsingStatusEnd)
}

const sigOpenFileFromPathResult = (status) => {
    if(!status){
        handleShowTip(store.loadTranslations['File has been deleted.'])
    }
}

const sigAppendWordWizardConv = async (type) => {
    if(showGuide.value) return  //引导页不处理划词对话
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
        isRetry.value = false
    }

    // TODO: 切换会话模式后， 需要将当前模式同步到后端
    store.ConversationModeStatus = store.ConversionMode.Normal
    await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

    //根据type切助手到当前助手
    for (let index = 0; index < assistantList.value.length; index++) {
        const element = assistantList.value[index];
        if (element.type === type) {
            // 切换助手
            currentAssistant.value = element
            await Qrequest(chatQWeb.setCurrentAssistantId, element.id) // 将当前助手同步到后端，包子在下面创建新会话时后端是针对当前助手创建的
            switchModel.value.clickAssistantItem(element)  // 切换界面的助手，将各种按钮状态同步
            //创建新会话
            await Qrequest(chatQWeb.createNewConversation) //创建新会话
            await getNowConversationInfo(currentAssistant.value.id)  //设置当前助手的最后一个会话信息
            break
        }
    }

    //通知后端添加划词对话历史记录到当前助手历史记录
    Qrequest(chatQWeb.appendWordWizardConv, type)

    // 获取历史会话
    const _history = await Qrequest(chatQWeb.getConversations)
    history.value = JSON.parse(_history)
}

const sigPreviewReference = async (preViewList) => {
    let extention = []
    extention = extention.concat(JSON.parse(_.last(_.last(history.value).answers).extention))
    extention.push(JSON.parse(preViewList))
    _.last(_.last(history.value).answers).extention = JSON.stringify(extention)
}

const sigOverrideQues = async (inputQuestion) => {
    if (showGuide.value) return  //引导页不处理覆盖问题
    if (
        (currentAssistant.value.type === store.AssistantType.PERSONAL_KNOWLEDGE_ASSISTANT && (!isKnowledgeBaseExist.value ||!isEmbeddingPluginsExist.value)&& history.value.length === 0)||
        ((currentAssistant.value.type === store.AssistantType.UOS_SYSTEM_ASSISTANT||currentAssistant.value.type === store.AssistantType.DEEPIN_SYSTEM_ASSISTANT) && !isEmbeddingPluginsExist.value && history.value.length === 0))
        return

    // 延时填入，避免数字形象过来界面切过来的值被初始化覆盖
    setTimeout(() => {
        question.value = question.value + inputQuestion
        endQus.value =  endQus.value + inputQuestion //如果中断语音输入，可能会延迟返回语音输入信号，需要将随航带过来的值追加
    }, 10);

    nextTick(() => {
        questionInput.value.focus()  //随航带入文档时直接让输入框可编辑
    })
}

const sigAsyncWorker = async (type) => {
    if (type === 1) {  // 覆盖输入框内容
        //中断当前对话，切助手
        //打断正在进行的对话
        if(showStop.value){
            stopRequest()
        }

        //打断语音播报

        //打断正在进行的语音输入
        if (recording.value) {
            recording.value = false;
            Qrequest(chatQWeb.stopRecorder)
        }

        question.value = ''

        //切换助手，聊天窗口存在文件则删除
        if(isFileInInput.value){
            isShowFile.value = false
            isFileInInput.value = false
            isFileParsingSuccess.value = false
            isAllParsingStatusEnd.value = false

            inputFileList.value = []
            extensionFileList.value = []
            await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
        }

        //切换助手，聊天窗口存在标签则删除
        if (showPromptTag.value) {
            showPromptTag.value = false
            showPropmptList.value = false
        }

        //切换助手为UOS AI
        assistantList.value.forEach(element => {
            if (element.type === store.AssistantType.UOS_AI) {
                Qrequest(chatQWeb.setCurrentAssistantId, element.id)
            }
        });

        //切助手和模型
        currentAssistant.value.active = false
        const resCurAccountID = await Qrequest(chatQWeb.currentLLMAccountId)
        const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
        assistantList.value.forEach(element => {
            if (element.id === resCurAssistantID) {
                element.active = true
                currentAssistant.value = element
            }
        });

        currentAccount.value.active = false
        accountList.value.forEach(element => {
            if (element.id === resCurAccountID) {
                element.active = true
                currentAccount.value = element
            }
        });

        /**
         * TODO:
         * 查询当前助手指令列表
         */
        currentAccountChanged()

        // TODO: 切换会话模式后， 需要将当前模式同步到后端
        store.ConversationModeStatus = store.ConversionMode.Normal
        await Qrequest(chatQWeb.setConversationMode, store.ConversionMode.Normal)

        const _history = await Qrequest(chatQWeb.getConversations)
        history.value = JSON.parse(_history)
        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    } else if (type === 2) {  // 继续对话
             //打断正在进行的语音输入
        if (recording.value) {
            recording.value = false;
            await Qrequest(chatQWeb.stopRecorder)
        }

        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    } else if (type === 3) {  // 问一问
        //打断正在进行的对话
        if(showStop.value){
            stopRequest()
        }

        //打断语音播报

        //打断正在进行的语音输入
        if (recording.value) {
            recording.value = false;
            Qrequest(chatQWeb.stopRecorder)
        }

        //如果不是UOSAI助手则切换过去
        if (currentAssistant.value.type !== store.AssistantType.UOS_AI) {
             //切换助手，聊天窗口存在文件则删除
            if (isFileInInput.value){
                isShowFile.value = false
                isFileInInput.value = false
                isFileParsingSuccess.value = false
                isAllParsingStatusEnd.value = false

                inputFileList.value = []
                extensionFileList.value = []
                await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
            }

            //切换助手，聊天窗口存在标签则删除
            if (showPromptTag.value) {
                showPromptTag.value = false
                showPropmptList.value = false
            }

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

            /**
            * TODO:
            * 查询当前助手指令列表
            */
            currentAccountChanged()

            // TODO: 切换会话模式后， 需要将当前模式同步到后端
            store.ConversationModeStatus = store.ConversionMode.Normal
            await Qrequest(chatQWeb.setConversationMode, store.ConversionMode.Normal)
            await getNowConversationInfo(currentAssistant.value.id)  //设置当前助手的最后一个会话信息

            const _history = await Qrequest(chatQWeb.getConversations)
            history.value = JSON.parse(_history)
        }

        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    }else if (type === 4) {  // 截图OCR
        await Qrequest(chatQWeb.onAsyncWorkerFinished, type)
    }
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
                isRetry.value = false
            }
            //切助手到UOS AI
            assistantList.value.forEach(element => {
                if (element.type === store.AssistantType.UOS_AI) {
                    switchModel.value.clickAssistantItem(element)
                }
            });

            // TODO: 切换会话模式后， 需要将当前模式同步到后端
            store.ConversationModeStatus = store.ConversionMode.Normal
            await Qrequest(chatQWeb.setConversationMode, store.ConversationModeStatus)

        }else {
            //切助手到当前助手
            assistantList.value.forEach(element => {
                if (element.id === resCurAssistantID) {
                    currentAssistant.value.active = true
                }
            });
        }
    }
}

const sigGetNewHistoryList = async (currentConversationId) => {
    //重新查询历史记录列表
    await openHistoryList()

    // 如果删除的是当前会话，需要重新创建新会话
    if (currentConversationId === lastConversationInfo.value.conversationId) {
        await createNewConversation()
    }
}

const sigHideHistoryList = async () => {
    showConversionList.value = false
}

const titleBarBtnWidth = ref(0)
const AutoMcpSteps = ref([])
const sigToShowPromptWindow = async (width) => {
    titleBarBtnWidth.value = width

    if (guideConfigStack.length === 0) {
        return;
    }

    const latestGuide = guideConfigStack[0];
    guideConfig.value = latestGuide.factory();

    // Handle new-ai-writing-guide specifically
    if (latestGuide.id === 'new-ai-writing-guide') {
        // 1. Find writing assistant
        const writingAssistant = assistantList.value.find(
            item => item.type === store.AssistantType.AI_WRITING_ASSISTANT
        )

        if (!writingAssistant) {
            console.warn('Writing assistant not found')
            return
        }

        // 2. Set guide state and expand assistant list
        guideActiveAssistantId.value = writingAssistant.id

        // 3. Ensure assistant list is expanded
        await nextTick()
        await switchModel.value?.clickAssistantSwitch()

        // 4. Wait for DOM to render and get target element
        await nextTick()
        await nextTick()  // Extra wait for animation

        // 5. Get assistant item element
        const assistantItem = await switchModel.value?.getItemElementById(writingAssistant?.id)

        // 6. Set step 1 targets
        if (assistantItem && guideConfig.value.steps[0]) {
            guideConfig.value.steps[0].targets = [assistantItem]
            guideConfig.value.steps[0].primaryTarget = assistantItem
        }

        // 7. Show guide
        showGuide.value = true
        isFocus.value = false
        questionInput.value.blur()
        Qrequest(chatQWeb.setTitleBarStatus, true)

        return
    }

    // ********************************************************************************
    // 2.12需求
    if (latestGuide.id !== 'auto-mcp-guide' && !isEnableMcp.value) {
        return  //未启用mcp，不显示引导
    }

    // 2.9需求
    if (latestGuide.id !== 'mcp-guide' && !isEnableMcp.value) {
        return  //未启用mcp，不显示引导
    }

    //切助手到UOS AI
    if (currentAssistant.value.type !== store.AssistantType.UOS_AI) {
        assistantList.value.forEach(element => {
            if (element.type === store.AssistantType.UOS_AI) {
                switchModel.value.clickAssistantItem(element)
            }
        });

        //切换助手为UOS AI
        for (let index = 0; index < assistantList.value.length; index++) {
            const element = assistantList.value[index];
            if (element.type === store.AssistantType.UOS_AI) {
                await Qrequest(chatQWeb.setCurrentAssistantId, element.id)
                break
            }
        }

        //切助手和模型
        currentAssistant.value.active = false
        const resCurAssistantID = await Qrequest(chatQWeb.currentAssistantId)
        assistantList.value.forEach(element => {
            if (element.id === resCurAssistantID) {
                element.active = true
                currentAssistant.value = element
            }
        });
    }

    // 动态配置并显示引导
    if (store.loadTranslations && Object.keys(store.loadTranslations).length > 0) {
        showGuide.value = true;
        isFocus.value = false;
        questionInput.value.blur();
        Qrequest(chatQWeb.setTitleBarStatus, true); //打开标题栏遮罩
    }
}

const sigToChangeFreeAccountGuide = async (isShowFreeAccountGuide, isPreShow) => {
    const latestGuide = guideConfigStack[0];
    guideConfig.value = latestGuide.factory();

    if (!isShowFreeAccountGuide) {
        AutoMcpSteps.value[1].activeText = store.loadTranslations['Got it']
        AutoMcpSteps.value[1].onActiveClick = () => {
            onGuideClose()
        }
        AutoMcpSteps.value.pop()
        guideConfig.value.steps = AutoMcpSteps.value
    } else {
        if (isPreShow) {
            // 删除 AutoMcpSteps.value 前面两个元素
            AutoMcpSteps.value.splice(0, 2)
            guideConfig.value.steps = AutoMcpSteps.value

            // 动态配置并显示引导
            if (store.loadTranslations && Object.keys(store.loadTranslations).length > 0) {
                showGuide.value = true;
                isFocus.value = false;
                questionInput.value.blur();
                Qrequest(chatQWeb.setTitleBarStatus, true); //打开标题栏遮罩
            }
        }
    }
}

const deleteMaterialFile = async (index) => {
    // 根据index删除inputFileList中匹配的素材文件
    // 保留：1. index不匹配的文件
    inputFileList.value = inputFileList.value.filter(item => item.index !== index)

    if(inputFileList.value.length === 0){
        isFileInInput.value = false
    }

    // 如果删除后没有剩余的素材文件，关闭素材列表
    if(materialFiles.value.length === 0){
        showLocalMaterialsList.value = false
    }

    await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
}

const sigDeleteFile = async (index) => {
    // 如果删除的文件是文件大纲，需要清空fileOutline
    if (currentAssistant.value.type === store.AssistantType.AI_WRITING_ASSISTANT && inputFileList.value.find(item => item.index === index).filePath === fileOutline.value) {
        const msg = store.loadTranslations['Confirm deletion of this outline file?']
        let ret = await Qrequest(chatQWeb.showRemoveFileDialog, msg)
        if (!ret)
            return
    }
    // 根据index删除inputFileList和extensionFileList中index字段相同的文件
    inputFileList.value = inputFileList.value.filter(item => item.index !== index)
    extensionFileList.value = extensionFileList.value.filter(item => item.index !== index)
    isFileParsingSuccess.value = inputFileList.value.every(file => file.isFileParsingSuccess)
    // 如果inputFileList.value为空，则设置isAllParsingStatusEnd.value为false
    if(inputFileList.value.length === 0){
        isAllParsingStatusEnd.value = false
    }else{
        isAllParsingStatusEnd.value = inputFileList.value.every(file => file.isParsingStatusEnd)  // 结果返回前将文件删除，就看剩下文件是否解析完成
    }

    // 如果inputFileList.value为空，则设置isFileInInput.value为false
    if(inputFileList.value.length === 0){
        isFileInInput.value = false
    }
    await Qrequest(chatQWeb.setInputFileSize, inputFileList.value.length)  // 同步更新当前输入框中存在的文件
}

const sigInputFocus = async () => {
    questionInput.value.focus()  //页面加载完成，自动聚焦到输入框
}

const sigShowTip = async (tip) => {
    handleShowTip(tip)
}

// const sigKnowledgeBaseFAQGenFinished = () => {

// }

const fileButtonWithMenuRef = ref(null)
const handleSelectFiles = (category) => {
    // 根据category处理选择逻辑
    if (category === store.DocFileCategory.LocalMaterial) {
        if (materialFiles.value.length >= 10) {
            handleShowTip(store.loadTranslations['Supports uploading up to 10 local materials'])
            return
        }
        // 处理本地材料选择
        Qrequest(chatQWeb.onDocSummaryForOfficeSelect, store.DocFileCategory.LocalMaterial)
    } else if (category === store.DocFileCategory.FileOutline) {
        if (fileOutline.value !== "") {
            handleShowTip(store.loadTranslations['Only supports uploading 1 outline file'])
            return
        }
        // 处理文件大纲选择
        Qrequest(chatQWeb.onDocSummaryForOfficeSelect, store.DocFileCategory.FileOutline)
    }
}

function handleKeyDown(event) {
    // 新手引导期间，禁用所有 Enter 键操作
    if (showGuide.value) {
        return
    }

    if (event.key === "Enter" && showPropmptList.value) {
        event.stopPropagation()
        event.preventDefault()
        showPropmptList.value = false
        return
    }
    if (event.key === "Enter" && !showMarkdownEditor.value) {
        event.stopPropagation()
        event.preventDefault()
        console.log('chat ')
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
    sigPPTCreateSuccess,
    sigPPTChangeSuccess,
    sigPosterCreateSuccess,
    sigThemeChanged,
    sigFontChanged,
    sigWebchat2BeHiden,
    sigAudioSampleLevel,
    sigNetStateChanged,
    sigDigitalModeActive,
    sigChatModeActive,
    sigWebchatActiveChanged,
    sigWebchatModalityChanged,
    sigMainContentBackgroundColor,
    sigKnowledgeBaseStatusChanged,
    sigEmbeddingPluginsStatusChanged,
    // sigKnowledgeBaseFAQGenFinished,
    sigWindowModeChanged,
    sigDocSummaryParsingStart,  //拖入文档
    sigDocSummaryForOffice,     //拖入Office文档解析开始
    sigDocSummaryParserResult,  //文档解析结果
    sigOpenFileFromPathResult,  //文档打开结果
    sigAppendWordWizardConv,  //随航继续对话接口 : 停止当前对话,获取随航历史记录
    sigPreviewReference, //预览列表
    sigAsyncWorker,
    sigOverrideQues,
    sigAssistantListChanged,  //助手列表变化
    sigGetNewHistoryList,
    sigHideHistoryList,
    sigToShowPromptWindow, //显示提示窗口
    sigToChangeFreeAccountGuide,  // 是否显示免费额度引导
    sigInputFocus,
    sigShowTip,
    sigWordWizardAsk,
    sigGetFreeCreditsResult,  // 领取免费额度结果
    sigIsGotFreeCredits,  // 是否领取过免费额度
    sigActiveChatFromDigitalImage, // 是否从数字形象强制切换为聊天
}

// 窗口失去焦点时，关闭所有弹窗
const handleWindowBlur = () => {
    showLocalMaterialsList.value = false;
    fileButtonWithMenuRef.value?.closeMenu();
    if (showMcpSetting.value) {
        showMcpSetting.value = false;
    }
    if (showConversionMode.value) {
        showConversionMode.value = false;
    }
};

const chatHistoryScrollbarRef = ref(null);
onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }
    document.addEventListener("keydown", handleKeyDown);
    window.addEventListener('blur', handleWindowBlur);
    initChat()

    useGlobalStore().loadTranslations = await Qrequest(chatQWeb.loadTranslations)
    console.log(useGlobalStore().loadTranslations)

    var fontInfo = await Qrequest(chatQWeb.fontInfo);
    var fontInfoList = fontInfo.split('#');
    document.documentElement.style.fontFamily = fontInfoList[0];
    document.documentElement.style.fontSize = fontInfoList[1] + 'px';
    updateFont(fontInfoList[0], fontInfoList[1])

    if (chatHistoryScrollbarRef.value) {
        document.getElementById('chatHistory').addEventListener('scroll', handleHistoryScroll);
    }

    nextTick(() => {
        questionInput.value.focus()  //页面加载完成，自动聚焦到输入框
    }, 5)

    // 这里要等SwitchModel渲染好
    switchModelRoot.value = switchModel.value?.rootEl
})
onBeforeUnmount(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
    document.removeEventListener("keydown", handleKeyDown);
    window.removeEventListener('blur', handleWindowBlur);

    if (chatHistoryScrollbarRef.value) {
        document.getElementById('chatHistory').removeEventListener('scroll', handleHistoryScroll);
    }
})
const handlePaste = async (event) => {
    // 阻止文本插入输入框
    event.preventDefault();
    event.stopPropagation();

    // 调用后端接口，获取剪切板中的内容
    const clipText = await Qrequest(chatQWeb.processClipboardData)
    if (clipText.length > 0) {
        const start = questionInput.value.textarea.selectionStart
        const end = questionInput.value.textarea.selectionEnd
        question.value = questionInput.value.textarea.value.substr(0, start) + clipText + questionInput.value.textarea.value.substr(end)
        // 把光标移动到clipText后面
        // questionInput.value.textarea.selectionStart = start + clipText.length
        // questionInput.value.textarea.selectionEnd = start + clipText.length
        nextTick(() => event.target.setSelectionRange(start + clipText.length, start + clipText.length))
    }
}

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
    background-color: var(--main-content-background-color);

    position: relative; /* 为蒙版层定位 */
    // user-select: none;
    img {
        user-select: none;
    }

    .chat-history {
        width: calc(100% - 4px);
        padding-bottom: 15px;
        max-width: 1004px;
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
            max-width: calc(100% - 60px); /* 左右各留40px边距 */
            width: fit-content;

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
                white-space: normal; /* 允许换行 */
                word-wrap: break-word; /* 长单词换行 */
            }

            .tip-item-msg {
                color: var(--uosai-color-tip);
                font-size: 0.93rem;
                font-weight: 500;
                font-style: normal;
                padding: 2px 6px 5px;
                border-radius: 8px;
                border: 1px solid rgba(0, 0, 0, 0.05);
                box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
                background-color: var(--uosai-color-tip-bg);
                margin: 0 auto;
                text-align: center;
                margin-bottom: 10px;
                width: fit-content;
                user-select: none;
                white-space: normal; /* 允许换行 */
                word-wrap: break-word; /* 长单词换行 */
                line-height: 1.2;

                &.advanced-features {
                    background-color: var(--uosai-color-tip-bg-qt6);
                }
            }

            .top-stop {
                color: var(--activityColor);
                background-color: var(--uosai-color-stop-bg);
                border-radius: 8px;
                display: flex;
                align-items: center;

                cursor: pointer;
                box-shadow: 0 0 6px 0 rgba(0, 0, 0, 0.08);
                border: none;

                svg {
                    width: 15px;
                    height: 15px;
                    margin-right: 4px;
                }
            }


        }

        .top-returnBtnOut {
            position: absolute;
            // left: calc(98% - 10px);
            // transform: translateX(-50%);
            top: -57px;
            right: 0px;
            cursor: pointer;
            z-index: 999;



            .top-returnBtn {
                display: flex;
                align-items: center;
                justify-content: center;
                // transform: translateX(-50%);
                // border: 1px solid rgba(0, 0, 0, 0.05);
                box-shadow: 0px 4px 16px 0px rgba(0, 0, 0, 0.2);
                background-color: var(--uosai-color-tip-bg);
                width: 32px;
                height: 32px;
                border-radius: 50%;

                svg {
                    width: 8px;
                    height: 8px;
                    fill: var(--uosai-color-modelbtn);
                }

                &:not(.disabled):active {

                    svg {
                        fill: var(--activityColor);
                    }
                }
            }
        }


        .top {
            position: relative;
            margin-bottom: 10px;
            padding-left: 5px;
            display: flex;
            align-items: center;

            .add-new-conversation {
                position: relative;
                width: 36px;
                height: 36px;
                background-color: var(--activityAddNewConversationBtnBgNormal);
                border-radius: 8px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                // margin-right: auto;
                margin-right: 6px;

                &:not(.disabled):hover {
                    background-color: var(--activityAddNewConversationBtnBgHover);
                }

                &:not(.disabled):active {
                    background-color: var(--activityAddNewConversationBtnBgActive);
                }

                svg {
                    height: 20px;
                    width: 20px;
                    color: var(--activityColor);
                }

                .conversation-mode-icon {
                    display: flex;
                    align-items: center;
                    width: 9px;
                    height: 20px;
                }
            }

            .prompt-btn{
                width: 36px;
                height: 36px;
                background-color: var(--uosai-color-flat-btn-bg);
                border-radius: 8px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                margin-right: 10px;

                &:not(.disabled):hover {
                    background-color: var(--uosai-color-flat-btn-bg-hover);

                    svg {
                        fill: var(--uosai-color-flat-btn-icon);
                    }
                }

                &:not(.disabled):active {
                    background-color: var(--uosai-color-flat-btn-bg-press);

                    svg {
                        fill: var(--uosai-color-flat-btn-icon);
                    }
                }

                svg {
                    height: 20px;
                    width: 20px;
                    fill: var(--uosai-color-flat-btn-icon);
                }
            }

            .mcp-switch{
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;

                .setmcp-icon {
                    display: flex;
                    align-items: center;
                    width: 9px;
                    height: 20px;
                }
            }

            .base-outline-gen-content-btn{
                height: 36px;
                border-radius: 8px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                box-shadow:0 4px 6px 0 var(--baseOutlineGenContentboxShadow);
                background-color: var(--activityColor);
                white-space: nowrap;
                overflow: hidden;
                text-overflow: ellipsis;

                svg {
                    width: 16px;
                    height: 16px;
                    margin-left: 10px;
                    margin-right: 5px;
                    fill: var(--uosai-color-baseOutlineGenContentBtn-icon);
                }

                .base-outline-gen-content-btn-text {
                    margin: 7px 17px 9px 0px;
                    font-size: 1rem;
                    line-height: 1.2;
                    color: var(--uosai-color-baseOutlineGenContentBtn-text);
                    font-weight: 500;
                }

                &:not(.disabled):hover {
                    background-color: var(--activityColorHover);
                }

                &:not(.disabled):active {
                    background-color: var(--activityColor);
                }
            }

            .base-outline-gen-content-btn2{
                height: 36px;
                border-radius: 8px;
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                box-shadow:0 4px 6px 0 var(--baseOutlineGenContentboxShadow);
                background-color: var(--activityColor);
                white-space: nowrap;
            }
        }

        .input-content {
            position: relative;
            min-height: 120px;
            border-radius: 8px;
            // opacity: 1;
            background-color: var(--uosai-color-inputcontent-bg);
            margin-bottom: 7px;
            // padding-bottom: 8px;
            border: 2px solid rgba(0, 0, 0, 0);

            .input-content-all{
                // background-color: var(--uosai-color-inputcontent-bg);
                opacity: 1;
                // margin-bottom: 7px;
                padding-bottom: 8px;
                border-radius: 8px;
                // overflow: hidden;
                // border: 2px solid rgba(0, 0, 0, 0);

                .input-tag{
                    display: block;
                    align-content: center;

                    .document-parsing{
                        .document-parsing-item{
                            display: auto;
                            top: 0;
                            left: 0;
                            margin-top: 8px;
                            margin-left: 10px;
                        }
                    }

                    /* 本地素材按钮样式 */
                    .local-material-btn-wrapper {
                        position: relative;
                        left: 10px;
                        top: 8px;
                        width: fit-content;
                        height: 38px;
                        margin-right: 10px;
                        align-items: center; /* 水平方向居中 */  
                        cursor: pointer;
                    }

                    .local-material-btn {
                        display: flex;
                        align-items: center; /* 水平方向居中 */  
                        height: 30px;
                        max-width: 100%;
                        min-width: none;
                        text-overflow: ellipsis;/* 超出部分显示省略号 */
                        overflow: hidden;/* 超出部分不显示 */
                        background-color: var(--uosai-color-document-parsing-bg);
                        border-radius: 8px;  /* 圆角 */
                        
                        .svg-icon {
                            width: 12px;
                            height: 14px;
                            margin-left: 10px;
                            margin-right: 6px;
                            fill: var(--uosai-color-conversion-mode-icon);
                        }
                    }

                    .local-material-text {
                        display: flex;
                        height: 30px;
                        align-items: center; /* 水平方向居中 */  
                        font-size: 1rem;
                        margin-bottom: 3px;
                        flex: 1; /* 占据剩余空间 */
                        min-width: 0; /* 允许收缩 */
                        color: var(--uosai-color-document-file-name-text);
                        user-select: none;
                    }
                }
            }

            // user-select:auto;
            &.foucs {
                border: 2px solid rgba(0, 0, 0, 0);

                &::before {
                    content: '';
                    position: absolute;
                    top: -2px;
                    left: -2px;
                    right: -2px;
                    bottom: -2px;
                    border: 2px solid var(--activityColor);
                    border-radius: 8px;
                    pointer-events: none;
                    z-index: 10;
                }
            }

            // 隐私模式下的聚焦样式
            &.foucs.private-mode {
                border: 2px solid transparent;  // 保持透明边框
                position: relative;  // 确保伪元素定位正确
                border-radius: 8px;

                &::before {
                    content: '';
                    position: absolute;
                    top: -2px;
                    left: -2px;
                    right: -2px;
                    bottom: -2px;
                    border: 2px dashed var(--activityColor);
                    border-radius: 8px;
                    pointer-events: none;
                    z-index: 10;
                }
            }

            :deep(.el-textarea) {
                padding-top: 10px;
                max-height: 185px;
                display: flex;

                .el-textarea__inner {
                    box-shadow: none;
                    background: none;
                    color: var(--uosai-color-inputcontent);
                    max-height: 185px;
                    font-size: 0.93rem;  // 会影响高度

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

            .bottom {
                display: flex;
                justify-content: space-between;
                align-items: center;
            }

            .deep-think-btn {
                display: flex;
                align-items: center;
                justify-content: center;
                cursor: pointer;
                width: 88px;
                height: 30px;
                border-radius: 8px;
                // border: 1px solid rgba(0, 0, 0, 0.05);
                margin-top: 6px;
                background-color: var(--uosai-think-search-normal-bg);
                user-select: none;
                color: var(--uosai-think-search-color);
                font-size: 12px;

                svg {
                    width: 16px;
                    height: 16px;
                    fill: var(--uosai-color-flat-btn-icon);
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
                        width: 16px;
                        height: 16px;
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
                    margin-right: 10px;

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
                        fill: var(--uosai-color-flat-btn-icon);
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

                .file-btn{
                    background-color: var(--uosai-color-voicebtn-bg);
                    margin-right: 10px;

                    &:not(.disabled):hover {
                        background-color: var(--uosai-color-voicebtn-bg-hover);
                    }

                    &:not(.disabled):active {
                        background-color: var(--uosai-color-voicebtn-bg-active);

                        svg {
                            fill: var(--uosai-color-voicebtn-bg-active-color);
                        }
                    }

                    svg {
                        height: 20px;
                        width: 20px;
                        fill: var(--uosai-color-flat-btn-icon);
                    }
                }
            }
        }

        .bottom-tip {
            display: flex;
            align-items: center;

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

    .inner-dropzone {
        /* 确保子div可以接收拖拽事件 */
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background: linear-gradient(rgba(255, 255, 255, 0) 5%,
                                    rgba(255, 255, 255, 0.35) 10%,
                                    rgba(255, 255, 255, 0.65) 15%,
                                    rgba(255, 255, 255, 0.75) 23%,
                                    rgba(255, 255, 255, 0.85) 30%,
                                    rgba(255, 255, 255, 0.95) 42%,
                                    rgba(255, 255, 255, 0.95) 100%); /* 半透明渐变蒙版 */
        z-index: 9999; /* 确保蒙版在内容之上 */
        align-items: center; /* 水平方向居中 */
        justify-content: center; /* 垂直方向居中 */
        display: grid;

        .inner-dropzone-content{
            width: 230px;
            // height: 186px;
            display: flex;
            // place-items: center;
            align-items: center; /* 水平方向居中 */
            justify-content: center; /* 垂直方向居中 */
            z-index: 1;
            // border: 1px solid black;
            pointer-events: none; /* 使层不响应鼠标事件 ,防止蒙版闪烁*/
            flex-direction: column; /* 子元素垂直排列 */
            margin-top: -40%;

            .inner-dropzone-icon{
                display: grid;
                // width: 94px;
                // height: 106px;
                width: 150px;
                height: 150px;
                display: flex;
                align-items: center; /* 水平方向居中 */
                justify-content: center; /* 垂直方向居中 */
                // border: 1px solid black;
                z-index: 1;

                .inner-dropzone-icon-icon{
                    // width: 94px;
                    // height: 106px;
                    width: 150px;
                    height: 150px;
                }
            }

            .inner-dropzone-text{
                // width: 187px;
                // height: 25px;
                height: auto; /* 让容器根据内容自动调整高度 */
                display: flex;
                font-size: 1.21rem;
                font-weight: 500;
                font-style: normal;
                font-family: var(--font-family);
                color:rgba(0, 0, 0, 0.9);
                align-items: center; /* 水平方向居中 */
                justify-content: center; /* 垂直方向居中 */
                margin-top: none;

                z-index: 1;
                white-space: nowrap; /* 不换行 */
                // border: 1px solid red;
            }

            .inner-dropzone-text-suffix{
                // width: 230px;
                height: auto;
                display: flex;
                font-size: 0.85rem;
                font-weight: 500;
                font-style: normal;
                font-family: var(--font-family);
                font-weight: 500;
                color:rgba(0, 0, 0, 0.6);
                align-items: center; /* 水平方向居中 */
                justify-content: center; /* 垂直方向居中 */
                text-align: center;
                margin-top: 8px;
                // border: 1px solid blue;
                z-index: 1;
            }
        }

    }

    .conversion-list-wrapper {
        position: absolute;
        // top: 60px;
        left: 0;
        width: 100%;
        height: 100%;
        z-index: 1000;
    }

    .mcp-list {
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        z-index: 999;
    }
}

.markdown-editor-container{
    display: flex;
    flex-direction: column; /* 垂直方向顺序布局 */
    align-items: center; /* 水平方向居中 */
    justify-content: center; /* 垂直方向居中 */
    height: 100vh;
    width: 100vw;
    overflow: hidden;
    background-color: var(--main-content-background-color);
    position: relative; /* 为蒙版层定位 */

}

.dark {
    .top-stop {
        box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.15) !important;
    }

    .inner-dropzone{
        background: linear-gradient(rgba(33, 33, 33, 0) 5%,
                                    rgba(33, 33, 33, 0.35) 10%,
                                    rgba(33, 33, 33, 0.65) 15%,
                                    rgba(33, 33, 33, 0.75) 23%,
                                    rgba(33, 33, 33, 0.85) 30%,
                                    rgba(33, 33, 33, 0.95) 42%,
                                    rgba(33, 33, 33, 0.95) 100%); /* 半透明渐变蒙版 */
        // background: linear-gradient(rgba(33, 33, 33, 0) 0%, rgba(33, 33, 33, 0.1) 5%,rgba(33, 33, 33, 0.85) 21%, rgba(33, 33, 33, 0.95) 42%, rgba(33, 33, 33, 0.95) 100%); /* 半透明渐变蒙版 */
    }

    .inner-dropzone-text-span{
        color: rgba(255, 255, 255, 0.9);
    }

    .inner-dropzone-text-suffix-span{
        color: rgba(255, 255, 255, 0.6);
    }
}
</style>