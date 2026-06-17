import { ref } from "vue";
import { defineStore } from "pinia";
import { DigitalHumanState } from "@/types/DigitalHuman";
import { AudioEvent } from "@/types/DigitalHuman";
import { AssistantID } from "@/types/assistant";
import type { ChatMessage } from "@/types/message";
import { ContentType } from "@/types/message";
import type { Message as ConversationMessage, RenderData, ErrorMsg } from "@/types/conversation";
import { UserType } from "@/types/conversation";
import { createId } from "@/utils/date";
import { useModelInfosStore, useConversationManagerStore, useAssistantInfosStore } from "@/stores";
import { useBackendStore } from "@/stores/backend";
import { ErrorType } from "@/types/errortype";

export const useDigitalHumanStore = defineStore("digitalHuman", {
    state: () => ({
        // 当前状态
        digitalHumanState: ref<DigitalHumanState>(DigitalHumanState.Silence),
        // backendStore 引用
        backendStore: ref<any>(null),
        // 缩放比例最底层（listen2.png）
        scale: ref(1),
        // 缩放比例第二层（listen1.png）
        scale1: ref(1),

        // 点击动画动作
        manualClickIconStop: ref(false),
        // 点击按钮停止
        manualClickBtnStop: ref(false),

        // 录音相关
        recordedText: ref(""), // 录音识别的文本
        hasRecordedContent: ref(false), // 是否有录音内容

        // 计时器相关
        silenceTimer: ref<number | null>(null), // 静音检测计时器（2秒）
        maxRecordTimer: ref<number | null>(null), // 最长录音时长计时器（60秒）
        countdownTimer: ref<number | null>(null), // 倒计时显示计时器
        countdownDelayTimer: ref<number | null>(null), // 倒计时延迟显示计时器
        noSoundTimer: ref<number | null>(null), // 长时间无声音计时器（10秒）
        countdownValue: ref(0), // 倒计时显示值
        showCountdown: ref(false), // 是否显示倒计时

        // 错误信息相关
        errorMessage: ref<ErrorMsg | null>(null), // 错误信息结构
        // 模型名称
        model: ref<string>(""), // 当前模型名称
    }),

    getters: {
        // 获取当前状态
        getCurrentState: (state) => state.digitalHumanState,
        // 获取缩放比例
        getScale: (state) => state.scale,
        getScale1: (state) => state.scale1,
        // 获取录音文本
        getRecordedText: (state) => state.recordedText,
        // 获取倒计时显示状态
        getShowCountdown: (state) => state.showCountdown,
        // 获取倒计时值
        getCountdownValue: (state) => state.countdownValue,
        // 获取错误信息结构
        getErrorMessage: (state) => state.errorMessage,
        // 获取模型名称
        getModel: (state) => state.model,
    },

    actions: {
        // ================================定时器管理======================================

        // 清除所有计时器
        clearAllTimers() {
            if (this.silenceTimer) {
                clearTimeout(this.silenceTimer);
                this.silenceTimer = null;
            }
            if (this.maxRecordTimer) {
                clearTimeout(this.maxRecordTimer);
                this.maxRecordTimer = null;
            }
            if (this.countdownTimer) {
                clearInterval(this.countdownTimer);
                this.countdownTimer = null;
            }
            if (this.countdownDelayTimer) {
                clearTimeout(this.countdownDelayTimer);
                this.countdownDelayTimer = null;
            }
            if (this.noSoundTimer) {
                clearTimeout(this.noSoundTimer);
                this.noSoundTimer = null;
            }
            this.showCountdown = false;
            this.countdownValue = 0;
        },

        // 启动静音检测计时器（2秒无声音自动停止录音）
        startSilenceTimer() {
            this.clearSilenceTimer();
            this.silenceTimer = window.setTimeout(() => {
                if (this.hasRecordedContent) {
                    // 有录音内容，自动停止并发送
                    if (this.backendStore) {
                        this.stopRecorder(this.backendStore);
                    }
                }
            }, 2000);
        },

        // 清除静音检测计时器
        clearSilenceTimer() {
            if (this.silenceTimer) {
                clearTimeout(this.silenceTimer);
                this.silenceTimer = null;
            }
        },

        // 启动最长录音时长计时器（60秒）
        startMaxRecordTimer() {
            this.clearMaxRecordTimer();
            this.maxRecordTimer = window.setTimeout(() => {
                // 60秒时间到，自动停止录音
                if (this.backendStore) {
                    this.stopRecorder(this.backendStore);
                }
            }, 600000);

            // 50秒后开始显示倒计时
            this.countdownDelayTimer = window.setTimeout(() => {
                if (this.digitalHumanState === DigitalHumanState.Listen) {
                    this.startCountdown();
                }
            }, 50000);
        },

        // 清除最长录音时长计时器
        clearMaxRecordTimer() {
            if (this.maxRecordTimer) {
                clearTimeout(this.maxRecordTimer);
                this.maxRecordTimer = null;
            }
        },

        // 启动倒计时显示
        startCountdown() {
            this.showCountdown = true;
            this.countdownValue = 10;
            this.countdownTimer = window.setInterval(() => {
                this.countdownValue--;
                if (this.countdownValue <= 0) {
                    this.clearCountdownTimer();
                }
            }, 1000);
        },

        // 清除倒计时显示
        clearCountdownTimer() {
            if (this.countdownTimer) {
                clearInterval(this.countdownTimer);
                this.countdownTimer = null;
            }
            this.showCountdown = false;
            this.countdownValue = 0;
        },

        // 启动长时间无声音计时器（10秒后进入休眠）
        startNoSoundTimer() {
            this.clearNoSoundTimer();
            this.noSoundTimer = window.setTimeout(() => {
                // 10秒无声音，进入休眠状态
                if (this.backendStore) {
                    this.stopRecorder(this.backendStore);
                }
                this.digitalHumanState = DigitalHumanState.Silence;
            }, 10000);
        },

        // 清除长时间无声音计时器
        clearNoSoundTimer() {
            if (this.noSoundTimer) {
                clearTimeout(this.noSoundTimer);
                this.noSoundTimer = null;
            }
        },

        // ================================后端交互======================================
        // 开始录音
        startRecorder(backendStore: any) {
            this.recordedText = "";
            this.hasRecordedContent = false;
            this.digitalHumanState = DigitalHumanState.Listen;
            backendStore.requestAudio("startRecorder", "");

            // 启动计时器
            this.startSilenceTimer();
            this.startMaxRecordTimer();
            this.startNoSoundTimer();
        },

        // 停止录音
        stopRecorder(backendStore: any) {
            console.log("停止录音");
            this.clearAllTimers();
            backendStore.requestAudio("stopRecorder", "");
        },

        // 停止录音并发送数据
        stopRecorderAndSend(backendStore: any) {
            console.log("停止录音并发送数据");
            this.clearAllTimers();
            this.stopRecorder(backendStore);

            if (this.hasRecordedContent && this.recordedText) {
                // 有录音内容，发送数据
                this.sendRecorderData(this.recordedText, backendStore);
                this.manualClickIconStop = false; // 重置手动点击动画动作标志
            } else {
                // 没有录音内容，直接进入休眠状态
                this.digitalHumanState = DigitalHumanState.Silence;
            }
        },

        // 发送录音数据
        sendRecorderData(data: string, backendStore: any) {
            console.log("发送录音数据:", data);
            // 录音数据
            this.handleSendMessage(backendStore);
        },

        // 处理发送消息
        async handleSendMessage(backendStore: any) {
            const modelInfosStore = useModelInfosStore();
            const conversationManagerStore = useConversationManagerStore();
            const assistantInfos = useAssistantInfosStore();
            // 检查会话数限制
            if (!(await conversationManagerStore.canCreateNewConversation())) {
                console.log("会话数限制，无法创建新会话");
                this.digitalHumanState = DigitalHumanState.Silence;
                return;
            }
            // 切换到思考状态
            this.digitalHumanState = DigitalHumanState.Thinking;
            // 将模板渲染为最终纯文本再提交
            const renderedText = this.recordedText;
            if (!renderedText.trim()) {
                return;
            }

            // 从 modelList 获取第一个模型的 id
            const currentModel = modelInfosStore.getCurrentModel;

            // 保存输入的值
            const userMessage = {
                content: renderedText,
            };

            const currentMessageId = createId(); // 生成一个当前唯一的消息ID
            if (1) {
                // 创建会话id：毫秒时间戳_4位随机数
                const sessionId = createId();

                // 获取当前会话ID
                const currentConversationId = conversationManagerStore.getCurrentConversationId;
                console.log(
                    "DigitalHuman 发送问题: 当前会话ID:",
                    currentConversationId,
                    conversationManagerStore.conversionList,
                );

                const assistantList = assistantInfos.assistantList;
                const uosAiAssistant = assistantList.find((assistant) => assistant.id === AssistantID.UOS_AI);

                // 创建 ChatMessage 发送给后端（保持不变）
                let message: ChatMessage = {
                    session_id: sessionId,
                    conversation_id: currentConversationId,
                    assistant: uosAiAssistant?.id || "",
                    model: currentModel?.id || "",
                    model_name: currentModel?.name || "",
                    user: "user",
                    params: {},
                    message: {
                        id: currentMessageId, // 消息ID, 前端发送
                        previous:
                            currentConversationId ===
                            conversationManagerStore.getMessageIdByConversationId(currentConversationId)
                                ? ""
                                : (conversationManagerStore.getMessageIdByConversationId(
                                      currentConversationId,
                                  ) as string), // 当前显示上一条消息的message.id
                        content: [
                            {
                                type: "text",
                                data: userMessage,
                            },
                        ],
                    },
                };

                const conversationRecordMessage: ConversationMessage = {
                    id: currentMessageId,
                    cur_next: "",
                    extension: {},
                    message: [
                        {
                            content: [
                                {
                                    content: userMessage.content,
                                    type: "text",
                                },
                            ],
                            role: "user",
                            source: "",
                        },
                    ],
                    next: [],
                    previous: conversationManagerStore.getMessageIdByConversationId(currentConversationId) as string,
                    render_message: [
                        {
                            data: {
                                content: userMessage.content,
                            },
                            type: "text",
                        },
                    ],
                    role: UserType.USER,
                    model_id: currentModel?.id || "",
                    model_name: currentModel?.name || "",
                };

                // 会话管理器添加消息
                conversationManagerStore.addConversationMessage(
                    currentConversationId,
                    currentMessageId,
                    conversationRecordMessage,
                    sessionId,
                );

                // 重新注册监听器，确保监听器绑定到正确的 conversationRecord 实例
                console.log("DigitalHuman Sending message:", message);
                await backendStore.requestSession("sendMessage", JSON.stringify(message));
            } else {
                // 当前助手没有新建会话成功
                console.error("Failed to find conversation for assistant: DigitalHuman");
            }
        },

        // 中断回答（即思考中）
        interruptAnswer(backendStore: any) {
            console.log("中断回答");
            this.clearAllTimers();
            this.digitalHumanState = DigitalHumanState.Silence;
            // TODO: 调用后端API中断回答
            this.handleStopMessage(backendStore);
            this.manualClickBtnStop = false; // 重置手动点击按钮停止标志
            this.manualClickIconStop = false; // 重置手动点击动画动作标志
        },

        // 处理停止消息
        async handleStopMessage(backendStore: any) {
            const conversationManagerStore = useConversationManagerStore();
            const currentSessionId = conversationManagerStore.getSessionIdByConversationId; // 获取当前会话ID
            if (currentSessionId) {
                // 调用后端停止会话
                await backendStore.requestSession("cancel", JSON.stringify({ session_id: currentSessionId }));
            }
        },

        // 播放回答
        playAnswer(backendStore: any, conversation_id: string, data: string) {
            console.log("播放回答:", data);
            this.digitalHumanState = DigitalHumanState.Answer;
            // 用conversation_id播放回答
            const param = { id: conversation_id, text: data, isEnd: true };
            // TODO: 调用后端API播放回答
            backendStore.requestAudio("playTextAudio", JSON.stringify(param));
        },

        // 中断播放回答
        interruptPlayAnswer(backendStore: any) {
            console.log("中断播放回答");
            this.clearAllTimers();
            // TODO: 调用后端API中断播放回答
            backendStore.requestAudio("stopPlayTextAudio", "{}");
            this.digitalHumanState = DigitalHumanState.Silence;
            this.manualClickBtnStop = false; // 重置手动点击按钮停止标志
            this.manualClickIconStop = false; // 重置手动点击动画动作标志
        },

        // ================================用户操作======================================
        // 处理状态组件点击事件
        handleStateClick(state: DigitalHumanState, backendStore: any) {
            console.log("State clicked:", state);
            // 可以在这里添加点击后的业务逻辑
            switch (state) {
                case DigitalHumanState.Silence:
                    this.handleSilenceClick(backendStore);
                    break;
                case DigitalHumanState.Listen:
                    this.handleListenClick(backendStore);
                    break;
                case DigitalHumanState.Thinking:
                    this.handleThinkingClick(backendStore);
                    break;
                case DigitalHumanState.Answer:
                    this.handleAnswerClick(backendStore);
                    break;
                case DigitalHumanState.Error:
                    this.digitalHumanState = DigitalHumanState.Silence;
                    break;
                default:
                    break;
            }
        },

        // 处理静默状态点击事件
        handleSilenceClick(backendStore: any) {
            this.manualClickIconStop = false; // 重置手动点击动画动作标志
            this.clearErrorMessage(); // 清除之前的错误信息
            this.startRecorder(backendStore); // 开始录音
        },

        // 处理聆听状态点击事件
        handleListenClick(backendStore: any) {
            this.manualClickIconStop = true; // 点击动画动作
            this.stopRecorder(backendStore); // 停止录音
        },

        // 处理思考状态点击事件
        handleThinkingClick(backendStore: any) {
            this.interruptAnswer(backendStore); // 中断回答
        },

        // 处理回答状态点击事件
        handleAnswerClick(backendStore: any) {
            this.interruptPlayAnswer(backendStore); // 中断播放回答
        },

        // 处理发送按钮点击事件
        handleSendClick(backendStore: any) {
            // 场景1：正常录音流程
            this.manualClickIconStop = false;
            this.startRecorder(backendStore);
        },

        // 处理停止按钮点击事件
        handleStopClick(backendStore: any) {
            // 根据当前状态调用不同的停止方法
            switch (this.digitalHumanState) {
                case DigitalHumanState.Listen:
                    // 场景5：用户主动停止（聆听中）
                    this.manualClickBtnStop = true; // 手动点击停止按钮
                    this.digitalHumanState = DigitalHumanState.Silence;
                    this.stopRecorder(backendStore);
                    break;
                case DigitalHumanState.Thinking:
                    // 中断回答（思考中）
                    this.interruptAnswer(backendStore);
                    break;
                case DigitalHumanState.Answer:
                    // 中断播放回答（回答中）
                    this.interruptPlayAnswer(backendStore);
                    break;
                default:
                    break;
            }
        },

        // ================================音频事件处理======================================
        // 监听状态变化事件
        handleAudioStateChange(audioState: AudioEvent, id: string, result: string) {
            switch (audioState) {
                case AudioEvent.AeError:
                    console.log("audioEvent 错误:", audioState, result);
                    this.digitalHumanState = DigitalHumanState.Error;
                    break;
                case AudioEvent.AeSuccess: // 成功
                    console.log("audioEvent 成功:", audioState, result);
                    break;
                case AudioEvent.AeLevelUpdated: // 录音音量级别更新
                    console.log("audioEvent 录音音量级别更新:", audioState, result);
                    this.handleAudioLevelUpdated(result);
                    break;
                case AudioEvent.AeTextReceived: // 语音识别文本接收
                    console.log("audioEvent 语音识别文本接收:", audioState, result);
                    this.handleTextReceived(result);
                    break;
                case AudioEvent.AePlayFinished: // 音频播放完成
                    console.log("audioEvent 音频播放完成:", audioState, result);
                    // 播放完成，进入休眠状态
                    this.digitalHumanState = DigitalHumanState.Silence;
                    break;
                case AudioEvent.AeRecordError: // 录音错误
                    console.log("audioEvent 录音错误:", audioState, result);
                    this.digitalHumanState = DigitalHumanState.Error;
                    this.handleRecordError(result);
                    break;
                case AudioEvent.AePlayerError: // 播放错误
                    this.digitalHumanState = DigitalHumanState.Error;
                    console.log("audioEvent 播放错误:", audioState, result);
                    this.handleRecordError(result); // 使用相同的处理函数，因为错误格式可能相同
                    break;
                case AudioEvent.AePlayDeviceChanged: // 播放设备变化
                case AudioEvent.AeRecordDeviceChanged: // 录音设备变化
                    console.log("audioEvent 录音设备变化:", audioState, result);
                    this.handleAudioDeviceChange(result);
                    break;
                default:
                    console.log("audioEvent 其他情况:", audioState, result);
                    this.digitalHumanState = DigitalHumanState.Silence; // 其他情况默认为静默状态
                    break;
            }
        },

        // 处理录音音量级别更新
        handleAudioLevelUpdated(result: string) {
            try {
                const ret = JSON.parse(result);
                if (ret && ret.level) {
                    const data = 1 + (ret.level / 100) * 0.7; // 最底层缩放系数
                    const data1 = 1 + (ret.level / 100) * 1.2; // 第二层缩放系数

                    this.scale = data;
                    this.scale1 = data1;
                }
            } catch (error) {
                this.digitalHumanState = DigitalHumanState.Error; // 其他情况默认为错误状态
                console.error("handleAudioLevelUpdated error", result, error);
            }
        },

        // 处理文本接收
        handleTextReceived(result: string) {
            try {
                const ret = JSON.parse(result);
                const keys = Object.keys(ret);
                if (ret && keys.includes("isEnd") && keys.includes("text")) {
                    // 每次收到的是全量录音文本
                    if (ret.text) {
                        this.recordedText = ret.text;
                        this.hasRecordedContent = true;
                        // 重置静音检测计时器
                        this.startSilenceTimer();
                    }

                    if (ret.isEnd) {
                        if (this.manualClickBtnStop) {
                            this.manualClickBtnStop = false;
                            return; // 如果是手动点击停止按钮，则不发送文本
                        }
                        if (this.backendStore && this.digitalHumanState === DigitalHumanState.Listen) {
                            // 都通过isEnd判断是否是录音结束，结束则直接发送文本
                            this.stopRecorderAndSend(this.backendStore);
                        }
                    }
                }
            } catch (error) {
                this.digitalHumanState = DigitalHumanState.Error; // 其他情况默认为错误状态
                console.error("handleTextReceived error", result, error);
            }
        },

        // 音频设备变化
        handleAudioDeviceChange(result: string) {
            try {
                const ret = JSON.parse(result);
                if (ret && ret.valid) {
                    this.digitalHumanState = DigitalHumanState.Silence; // 其他情况默认为静默状态
                } else {
                    this.digitalHumanState = DigitalHumanState.Error; // 其他情况默认为错误状态
                }
            } catch (error) {
                this.digitalHumanState = DigitalHumanState.Error; // 其他情况默认为错误状态
                console.error("handleAudioDeviceChange error", result, error);
            }
        },

        // 处理录音错误
        handleRecordError(result: string) {
            try {
                const errorData = JSON.parse(result);
                if (errorData && (errorData.error || errorData.error_message || errorData.http_error)) {
                    // 将解析后的错误信息赋值给 errorMessage
                    this.errorMessage = errorData as ErrorMsg;
                    console.log("录音错误信息已存储:", errorData);
                }
            } catch (error) {
                console.error("handleRecordError 解析错误信息失败:", result, error);
                // 如果解析失败，创建一个包含原始错误的 ErrorMsg 结构
                this.errorMessage = {
                    error: -1,
                    error_message: `解析错误信息失败: ${result}`,
                    http_error: undefined,
                };
            }
        },

        // 清除错误信息
        clearErrorMessage() {
            this.errorMessage = null;
            this.model = "";
        },
        async sessionFinishedCallback(conversationId: string, messageId: string) {
            console.log("DigitalHuman: 会话完成信号已触发，会话ID:", conversationId, "消息ID:", messageId);
            if (this.manualClickIconStop) {
                // 如果是手动点击停止图标，则不播放回答
                return;
            }
            console.log("DigitalHuman: 当前状态:", this.digitalHumanState);
            if (this.digitalHumanState !== DigitalHumanState.Thinking) {
                // 如果不是回答状态，则不播放回答
                return;
            }
            // 获取需要朗读的消息
            const conversationManagerStore = useConversationManagerStore();
            const conversation = conversationManagerStore.getCurrentMessagesRender;
            console.log("DigitalHuman: 会话内容:", conversation);
            if (conversation) {
                const message = conversation.messages[messageId];
                if (message) {
                    const renderMessage = message.render_message;
                    if (renderMessage) {
                        // 查找是否有错误信息
                        const errorMessages = renderMessage.filter((msg) => msg.type === ContentType.CntError);
                        const model = message.model_id;
                        console.log("DigitalHuman: 错误信息:", errorMessages);
                        if (errorMessages && errorMessages.length > 0) {
                            // 如果有错误信息，则不播放回答
                            this.digitalHumanState = DigitalHumanState.Error; // 设置为错误状态
                            // 取第一个错误消息的 data 字段作为 ErrorMsg
                            const firstError = errorMessages[0];
                            if (firstError && firstError.data) {
                                this.errorMessage = firstError.data as ErrorMsg;
                            }
                            this.model = model; // 存储模型信息
                            return;
                        }
                        // 清理错误信息
                        this.clearErrorMessage();
                        // 找到所有type == "Text"的消息
                        const textMessages = renderMessage.filter((msg) => msg.type === ContentType.CntText);
                        if (textMessages) {
                            // 将所有Text消息的text拼接起来，作为朗读内容
                            const allText = textMessages.map((msg) => (msg.data as RenderData).content).join("\n");
                            if (allText !== "") {
                                // 调用朗读函数
                                this.playAnswer(this.backendStore, conversationId, allText);
                            }
                        }
                    }
                }
            }
        },

        // 初始化数字人状态
        async initDigitalHumanState() {
            // 检测输入输出设备
            // 检查是否有音频输入设备
            let isAudioInputDeviceExists = false;
            try {
                const deviceStatusRet = await useBackendStore().requestAudio("getDeviceStatus", "{}");
                const deviceStatus = JSON.parse(deviceStatusRet as string);
                isAudioInputDeviceExists = (deviceStatus.inputValid && deviceStatus.outputValid) || false;
                if (!isAudioInputDeviceExists) {
                    this.errorMessage = {
                        error: ErrorType.AudioInputDeviceInvalid,
                        error_message: useBackendStore().translate("No microphone detected"),
                    };
                }
            } catch (error) {
                console.error("Error checking audio input device:", error);
            }

            this.digitalHumanState = isAudioInputDeviceExists ? DigitalHumanState.Listen : DigitalHumanState.Error; // 初始化状态为监听状态
            if (isAudioInputDeviceExists) {
                this.handleSilenceClick(useBackendStore()); // 初始化就开始录音
                this.clearErrorMessage(); // 清除错误信息
            }

            const assistantInfos = useAssistantInfosStore();
            const modelInfosStore = useModelInfosStore();
            const conversationManagerStore = useConversationManagerStore();

            const assistantList = assistantInfos.assistantList;
            const uosAiAssistant = assistantList.find((assistant) => assistant.id === AssistantID.UOS_AI);

            if (uosAiAssistant) {
                assistantInfos.setCurrentAssistant(uosAiAssistant);
                await modelInfosStore.loadModelList(uosAiAssistant.id);

                // 会话管理器创建对话
                const conversationId = createId(); // 初始化会话ID，使用当前时间戳
                await conversationManagerStore.createConversation(
                    conversationId,
                    uosAiAssistant.id,
                    modelInfosStore.getCurrentModel?.id || "", // TODO: 暂时使用当前模型
                );

                // 获取 ConversationRecord 实例并注册信号监听器
                const conversationRecord = conversationManagerStore.getConversationById(conversationId);
                if (conversationRecord) {
                    conversationRecord.on("sessionFinished", this.sessionFinishedCallback); // 处理会话完成信号
                }
            }
        },

        // 关闭数字人会话（只重置状态，不关闭会话，会话后台继续运行）
        async closeDigitalHumanSession() {
            // 如果是回答中，需要中断播放回答（回答中）计时器
            this.stopRecorder(this.backendStore); // 停止录音
            this.interruptPlayAnswer(this.backendStore); // 中断回答（回答中）
            this.clearAllTimers(); // 清除所有计时器
            this.manualClickBtnStop = false; // 重置手动点击停止按钮状态
            this.manualClickIconStop = false; // 重置手动点击停止图标状态
            // 获取 ConversationRecord 实例并注销除信号监听器
            const conversationManagerStore = useConversationManagerStore();
            const conversationId = conversationManagerStore.getCurrentConversationId;
            const conversationRecord = conversationManagerStore.getConversationById(conversationId);
            if (conversationRecord) {
                conversationRecord.off("sessionFinished", this.sessionFinishedCallback); // 处理会话完成信号
            }

            this.clearErrorMessage(); // 清除错误信息
        },
    },
});
