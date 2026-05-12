import { defineComponent, ref, inject, computed, onMounted, onUnmounted, watch } from "vue";
import type { PropType } from "vue";

import type {
    Message,
    OutlineData,
    OutlineRefData,
    ReasoningData,
    AgentStepData,
    DocCardData,
    ToolUseData,
    ErrorMsg,
    RenderData,
    CommandCardData,
} from "@/types/conversation";
import { UserType } from "@/types/conversation";
import { CopyDataType, ContentType } from "@/types/message";
import { CHAT_INPUT_KEY } from "@/types/chat-input";
import { ASSISTANT_VIEW_CONFIG_KEY } from "@/types/assistant-view";
import { type UploadFile, FileEvent } from "@/types/uploadfile";
import FileGroup from "@/components/filegroup/FileGroup";
import type { DisplayFile } from "@/types/file";
import { DocParsingFileType, DocFileCategory, FileAlignment, PopoverAlign, PopoverPlacement } from "@/types/file";
import { formatFileSize, getFileType } from "@/utils/filehelper";

import { OutlineViewer } from "@/views/window/mainwindow/page/chat/components/outline";
import { Reasoning, AgentStep } from "@/views/window/mainwindow/page/chat/components/reasoning";
import { DocCard } from "@/views/window/mainwindow/page/chat/components/docCard";
import { CommandCard } from "./commandCard";
import MarkdownRenderer from "@/views/window/mainwindow/page/chat/components/MarkdownRenderer";
import MessageActionButtons from "@/views/window/mainwindow/page/chat/components/MessageActionButtons";
import LoadingDots from "@/views/window/mainwindow/page/chat/components/LoadingDots";
import ToolUse from "@/views/window/mainwindow/page/chat/components/toolUse/ToolUse";
import Error from "@/views/window/mainwindow/page/chat/components/error/error";
import OperationCanceledHint from "@/views/window/mainwindow/page/chat/components/error/OperationCanceledHint";

import { useBackendStore, useConversationManagerStore, useNetworkStore, useUploadFilesStore } from "@/stores";
import { AudioEvent } from "@/types/DigitalHuman";
import { ErrorType } from "@/types/errortype";
import { cascaderEmits } from "element-plus";

export default defineComponent({
    name: "Message",

    components: {
        FileGroup,
        OutlineViewer,
        Reasoning,
        ToolUse,
        DocCard,
        Error,
        CommandCard,
        MarkdownRenderer,
        LoadingDots,
        MessageActionButtons,
        OperationCanceledHint,
    },

    props: {
        // 单个消息
        message: {
            type: Object as PropType<Message>,
            required: true,
        },
        // 是否正在流式输出最后一条消息
        isStreamingLastMessage: {
            type: Boolean,
            default: false,
        },
        // 是否是最后一条消息
        isLastMessage: {
            type: Boolean,
            default: false,
        },
        // 兄弟消息信息
        siblingMessage: {
            type: Object as PropType<{
                currIndex: number;
                total: number;
                parentId?: string;
                currentId?: string;
            }>,
            default: () => ({ currIndex: 0, total: 1 }),
        },
        // 是否正在运行会话
        isSessionRunning: {
            type: Boolean,
            default: false,
        },
        // 操作是否被取消
        isOperationCanceled: {
            type: Boolean,
            default: false,
        },
        // 上一个消息（仅当当前消息是 ASSISTANT 时传递）
        previousMessage: {
            type: Object as PropType<Message>,
            default: undefined,
        },
        // 是否从历史记录中加载
        isFromHistory: {
            type: Boolean,
            default: false,
        },
        // 当前正在播放的消息ID
        playingMessageId: {
            type: String as PropType<string | null>,
            default: null,
        },
    },

    emits: {
        // 消息点击事件
        messageClick: (message: Message) => {
            return message && typeof message === "object";
        },
        // 重试消息事件
        retryMessage: (message: Message) => {
            return message && typeof message === "object";
        },
        // 切换到前一个兄弟消息
        switchToPrevious: (parentId: string, currentId: string) => {
            return parentId && currentId;
        },
        // 切换到后一个兄弟消息
        switchToNext: (parentId: string, currentId: string) => {
            return parentId && currentId;
        },
        // 播放状态变更事件
        playingMessageIdChange: (messageId: string | null) => {
            return true;
        },
    },

    setup(props, { emit }) {
        const conversationManagerStore = useConversationManagerStore();
        const backendStore = useBackendStore();
        const networkStore = useNetworkStore();
        const uploadFilesStore = useUploadFilesStore();
        const chatInputContext = inject(CHAT_INPUT_KEY);
        const assistantViewConfig = inject(ASSISTANT_VIEW_CONFIG_KEY);
        const showRetry = computed(() => assistantViewConfig?.value.message?.showRetry ?? true);
        const isNetworkOnline = computed(() => networkStore.isNetworkOnline); // 是否网络在线
        const isAudioOutputDeviceExists = ref(false); // 是否存在音频输出设备

        // 跟踪当前 hover 的消息索引
        const hoveredMessageIndex = ref<number | null>(null);

        // 控制操作取消提示的显示
        const showOperationCanceledHint = ref(true);
        // 音频网络错误状态
        const audioNetworkError = ref(false);
        const getDocParsingFileType = (fileName: string): DocParsingFileType => {
            if (!fileName) return DocParsingFileType.Doc;

            const ext = fileName.split(".").pop()?.toLowerCase() || "";
            const imageExtensions = ["jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"];
            return imageExtensions.includes(ext) ? DocParsingFileType.Image : DocParsingFileType.Doc;
        };

        const cachedFiles = computed<UploadFile[]>(() => {
            // 统一从 extension.uploadedFiles 读取前端元数据
            const uploadedFiles = props.message.extension?.uploadedFiles;
            if (Array.isArray(uploadedFiles)) {
                return uploadedFiles
                    .filter(
                        (file): file is UploadFile =>
                            typeof file === "object" &&
                            file !== null &&
                            typeof (file as UploadFile).filePath === "string" &&
                            typeof (file as UploadFile).fileName === "string",
                    )
                    .map((file) => ({ ...file }));
            }

            return [];
        });

        const cachedDisplayFiles = computed<DisplayFile[]>(() => {
            return cachedFiles.value.map((file, index) => ({
                index,
                fileNameText: file.fileName,
                filePath: file.filePath,
                type: getDocParsingFileType(file.fileName),
                imgBase64: file.icon,
                fileSize: file.fileSize,
                category: file.category !== undefined ? (file.category as unknown as DocFileCategory) : undefined,
                fileType: getFileType(file.fileName, (key) => backendStore.translate(key)),
                fileSizeText: formatFileSize(file.fileSize),
            }));
        });

        // 处理消息点击
        const handleMessageClick = (message: Message) => {
            emit("messageClick", message);
        };

        // 处理消息 hover 进入
        const handleMessageMouseEnter = (index: number) => {
            hoveredMessageIndex.value = index;
        };

        // 处理消息 hover 离开
        const handleMessageMouseLeave = () => {
            hoveredMessageIndex.value = null;
        };

        // 判断收到停止的错误消息时，气泡中是否有内容
        const hasContent = computed(() => {
            return props.message.render_message.some((item) => {
                switch (item.type) {
                    case ContentType.CntOutline:
                    case ContentType.CntAgentReasoning:
                    case ContentType.CntAgentAction:
                    case ContentType.CntDocCard:
                        return true;
                    case ContentType.CntText: {
                        return (item.data as RenderData).content !== "";
                    }
                    default:
                        return false;
                }
            });
        });

        // 判断操作按钮的可见性类名
        const getActionButtonsVisibilityClass = (msg: Message) => {
            // AI 消息且是最后一条消息：常驻显示
            if (msg.role === UserType.ASSISTANT && props.isLastMessage) {
                return "message-action-buttons--visible";
            }
            // 其他情况（用户消息 或 非最后一条的AI消息）根据 hover 状态控制可见性
            return hoveredMessageIndex.value !== null
                ? "message-action-buttons--visible"
                : "message-action-buttons--hidden";
        };

        // 从 workspace 加载的大纲数据缓存（响应式，用于渲染）
        const loadedOutlineData = ref<OutlineData | null>(null);
        const outlineLoading = ref(false);

        // 最新的编辑数据缓存（非响应式，仅用于复制/播放，避免触发重绘）
        let latestOutlineData: OutlineData | null = null;

        // 处理大纲内容变更并持久化（编辑标题、拖拽排序、增删章节均会触发）
        const handleOutlineChange = (msg: Message, outlineData: OutlineData) => {
            if (!msg.id) {
                console.warn("handleOutlineChange: message has no id, skipping persistence");
                return;
            }

            // 更新非响应式缓存（避免触发界面重绘）
            latestOutlineData = outlineData;

            // 持久化到数据库
            const conversationId = conversationManagerStore.currentConversationId;
            conversationManagerStore.updateOutlineData(conversationId, msg.id, outlineData);
        };

        // 处理大纲删除事件
        const handleOutlineDeleteParagraph = (messageIndex: number, paragraphIndex: number) => {
            // 预留编辑功能实现
            console.log("Delete paragraph:", messageIndex, paragraphIndex);
        };

        // 处理小章节删除
        const handleOutlineDeleteSubsection = (
            messageIndex: number,
            paragraphIndex: number,
            subsectionIndex: number,
        ) => {
            // 预留编辑功能实现
            console.log("Delete subsection:", messageIndex, paragraphIndex, subsectionIndex);
        };

        // Check if a message should show loading indicator
        const shouldShowLoading = (message: Message) => {
            // Only show for AI messages
            if (message.role !== UserType.ASSISTANT) {
                return false;
            }
            // Only show if streaming is active
            return props.isStreamingLastMessage;
        };

        // 处理编辑按钮点击
        const handleEditClick = async (event: MouseEvent, msg: Message) => {
            console.log("Edit clicked", event);
            // 检查消息是否有 ID
            if (!msg.id) {
                console.warn("Message has no id");
                return;
            }

            // 判断文件是否存在
            const filePaths = cachedFiles.value.map((file) => ({
                filePath: file.filePath,
                imgBase64: file.icon,
                isExist: false,
            }));

            // 并行检查所有文件是否存在
            await Promise.all(
                filePaths.map(async (filePath) => {
                    const isExist = await backendStore.requestFile("isFileExist", filePath.filePath);
                    filePath.isExist = isExist as boolean;
                }),
            );

            //找出不存在的文件
            const nonExistPaths = filePaths
                .filter((filePath) => !filePath.isExist)
                .map((filePath) => ({
                    imgBase64: filePath.imgBase64,
                    filePath: filePath.filePath,
                }));

            // 找出存在的文件
            const validFiles = cachedFiles.value.filter((file) => {
                const found = filePaths.find((fp) => fp.filePath === file.filePath);
                return found?.isExist === true;
            });

            if (nonExistPaths.length > 0) {
                // 保存待编辑的消息、有效文件列表并显示过期文件对话框
                uploadFilesStore.setExpiredFiles(nonExistPaths, msg.id, validFiles);
                return;
            }

            // 继续执行编辑逻辑
            proceedWithEdit(msg, cachedFiles.value);
        };

        // 执行实际的编辑操作
        const proceedWithEdit = (msg: Message, filesToProcess?: UploadFile[]) => {
            // 使用传入的文件列表，如果没有则使用 cachedFiles
            const targetFiles = filesToProcess || cachedFiles.value;

            // 查找当前消息的文件
            /**
             * {
             *   paths: [],
             *   default_prompt: "",
             *   category: "",
             *   backend_method: "handleDroppedFiles",
             * }
             */
            // 按 category 分类
            const filesByCategory = new Map<string | undefined, string[]>();

            targetFiles.forEach((file) => {
                const category = file.category as unknown as DocFileCategory | undefined;
                const categoryKey = category === undefined ? undefined : String(category);

                if (!filesByCategory.has(categoryKey)) {
                    filesByCategory.set(categoryKey, []);
                }
                filesByCategory.get(categoryKey)!.push(file.filePath);
            });

            // 创建 files 数组
            const files: any[] = [];

            // 处理 undefined 类别（不包含 category 字段）
            const undefinedPaths = filesByCategory.get(undefined);
            if (undefinedPaths && undefinedPaths.length > 0) {
                files.push({
                    paths: undefinedPaths,
                    default_prompt: "",
                    backend_method: "handleDroppedFiles",
                });
            }

            // 处理其他类别
            filesByCategory.forEach((paths, categoryKey) => {
                if (categoryKey !== undefined && paths.length > 0) {
                    files.push({
                        paths: paths,
                        default_prompt: "",
                        category: categoryKey,
                        backend_method: "handleDroppedFiles",
                    });
                }
            });

            // 清空输入框文件
            uploadFilesStore.clearFiles();
            files.forEach((file) => {
                uploadFilesStore.handleFileEvent(FileEvent.FeIncomingFiles, "", JSON.stringify(file));
            });

            // 提取消息中的文本内容
            const textContent = msg.render_message
                .filter((item) => item.type === "text" && "content" in item.data)
                .map((item) => (item.data as { content: string }).content)
                .join("\n");

            // 将文本内容填充到输入框
            if (chatInputContext) {
                chatInputContext.fillInput(textContent, "replace");
                chatInputContext.focusInput();
            } else {
                console.warn("ChatInputContext not available");
            }

            // 隐藏操作取消提示
            showOperationCanceledHint.value = false;
        };

        // 将大纲数据转换为 Markdown 格式
        const convertOutlineToMarkdown = (outlineData: OutlineData): string => {
            const lines: string[] = [];

            // 添加标题
            lines.push(`# ${outlineData.title}`);

            // 遍历大章节
            outlineData.paragraphs.forEach((paragraph) => {
                lines.push(""); // 大章节后加空行
                lines.push(`## ${paragraph.title}`);

                // 遍历小章节
                paragraph.content.forEach((section) => {
                    lines.push(`### ${section.title}`);
                });
            });

            return lines.join("\n");
        };

        // 提取消息中的文本内容（支持普通文本、大纲和错误消息）
        const extractTextContent = (msg: Message): string => {
            // 优先检查错误消息（错误消息独立显示，不与文本混合）
            const errorItem = msg.render_message.find((item) => item.type === ContentType.CntError);
            if (errorItem) {
                const errorData = errorItem.data as ErrorMsg;
                return errorData.error_message || "Unknown error";
            }

            // 检查是否包含大纲
            const outlineItem = msg.render_message.find((item) => item.type === ContentType.CntOutline);
            if (outlineItem) {
                // 优先使用最新的编辑数据（避免触发重绘），否则使用加载的数据
                const outlineData = latestOutlineData || loadedOutlineData.value;
                if (outlineData) {
                    return convertOutlineToMarkdown(outlineData);
                }
                // 如果大纲数据还未加载，返回引用中的标题
                const refData = outlineItem.data as OutlineRefData;
                return `# ${refData.title}`;
            }

            // 普通文本：提取所有 text 类型的内容
            return msg.render_message
                .filter((item) => item.type === "text" && "content" in item.data)
                .map((item) => (item.data as { content: string }).content)
                .join("\n");
        };

        // 处理复制按钮点击
        const handleCopyClick = (event: MouseEvent, msg: Message) => {
            console.log("Copy clicked", event);
            // 检查消息是否有 ID
            if (!msg.id) {
                console.warn("Message has no id");
                return;
            }

            // 提取消息中的文本内容（支持大纲）
            const textContent = extractTextContent(msg);

            backendStore.requestSystem("copyToClipboard", textContent, CopyDataType.CopyText);
        };

        // 处理播放按钮点击
        const handlePlayClick = async (event: MouseEvent, msg: Message) => {
            event.stopPropagation();

            const isPlayingCurrent = props.playingMessageId === msg.id;

            // 如果当前正在播放这条消息，则停止播放
            if (isPlayingCurrent) {
                await backendStore.requestAudio("stopPlayTextAudio", "{}");
                emit("playingMessageIdChange", null);
                return;
            }

            // 如果有其他消息正在播放，先停止播放设备
            if (props.playingMessageId) {
                await backendStore.requestAudio("stopPlayTextAudio", "{}");
            }

            // 检查消息是否有 ID
            if (!msg.id) {
                console.warn("Message has no id");
                return;
            }

            // 提取消息中的文本内容（支持大纲）
            const textContent = extractTextContent(msg);

            if (!textContent) {
                console.warn("No text content found in message");
                return;
            }

            // 生成唯一的播放 ID
            const playbackId = `playback_${msg.id}_${Date.now()}`;
            emit("playingMessageIdChange", msg.id);

            // 调用 AudioChannel 的 playTextAudio 方法
            await backendStore.requestAudio(
                "playTextAudio",
                JSON.stringify({
                    id: playbackId,
                    text: textContent,
                    isEnd: true,
                }),
            );
        };

        // 处理重试按钮点击
        const handleRetryClick = (event: MouseEvent) => {
            console.log("Retry clicked", event);
            // 重试消息
            emit("retryMessage", props.message);
        };

        const handleOpenCachedFile = async (filePath: string) => {
            await backendStore.requestSystem("openFile", filePath);
        };

        // 根据消息类型动态生成 MessageActionButtons 配置
        const getActionButtonsConfig = (msg: Message) => {
            const visibilityClass = getActionButtonsVisibilityClass(msg);
            const { siblingMessage } = props;

            // 只有当有多个兄弟消息时才添加切换按钮
            const hasSiblings = siblingMessage.total > 1;

            if (msg.role === UserType.USER) {
                const config: any = {
                    className: ["message-action-buttons--right", visibilityClass].filter(Boolean).join(" "),
                    actionButtons: [
                        {
                            icon: "copy",
                            onClick: (event: MouseEvent) => handleCopyClick(event, msg),
                            tooltip: backendStore.translate("Copy"),
                        },
                        {
                            icon: "edit",
                            onClick: (event: MouseEvent) => handleEditClick(event, msg),
                            tooltip: backendStore.translate("Re-edit"),
                        },
                    ],
                };

                // 如果有兄弟消息，添加切换按钮配置
                if (hasSiblings) {
                    config.siblingMessage = {
                        currIndex: siblingMessage.currIndex,
                        total: siblingMessage.total,
                        onPrevious: () =>
                            emit("switchToPrevious", siblingMessage.parentId || "", siblingMessage.currentId || ""),
                        onNext: () =>
                            emit("switchToNext", siblingMessage.parentId || "", siblingMessage.currentId || ""),
                    };
                }

                return config;
            } else {
                // 如果会话正在运行，返回空配置
                if (props.isSessionRunning && props.isLastMessage) {
                    return {};
                }
                const isPlaying = props.playingMessageId === msg.id && !audioNetworkError.value;
                const voicePlayTooltip = () => {
                    if (!isAudioOutputDeviceExists.value) {
                        return backendStore.translate(
                            "The sound output device is not detected, please check and try again!",
                        );
                    }

                    if (!isNetworkOnline.value || audioNetworkError.value) {
                        return backendStore.translate(
                            "Voice broadcast is temporarily unavailable, please check the network!",
                        );
                    }

                    // TODO: 设备插拔时，需要更新语音播放按钮的 tooltip

                    if (isPlaying) {
                        return backendStore.translate("Stop Reading");
                    }

                    return backendStore.translate("Voice Read");
                };

                const actionButtons = [
                    {
                        icon: "voice-play",
                        onClick: (event: MouseEvent) => handlePlayClick(event, msg),
                        tooltip: voicePlayTooltip(),
                        isPlaying: isPlaying,
                        disabled: !isNetworkOnline.value || audioNetworkError.value || !isAudioOutputDeviceExists.value,
                    },
                    {
                        icon: "copy",
                        onClick: (event: MouseEvent) => handleCopyClick(event, msg),
                        tooltip: backendStore.translate("Copy"),
                    },
                ];

                // 只有最后一条AI消息才添加重试按钮
                if (props.isLastMessage && showRetry.value) {
                    // 最大重试次数为5，当兄弟消息总数达到5时禁用重试按钮
                    const isRetryDisabled = siblingMessage.total >= 5;
                    const retryTooltip = isRetryDisabled
                        ? backendStore.translate("Answer each question up to 5 times")
                        : backendStore.translate("Regenerate");

                    actionButtons.push({
                        icon: "retry",
                        onClick: isRetryDisabled ? () => {} : handleRetryClick,
                        tooltip: retryTooltip,
                        disabled: isRetryDisabled,
                    });
                }

                const config: any = {
                    className: ["message-action-buttons--left", visibilityClass].filter(Boolean).join(" "),
                    modelIcon: "model-default",
                    modelName: msg.model_name,
                    actionButtons,
                };

                // 如果有兄弟消息，添加切换按钮配置
                if (hasSiblings) {
                    config.siblingMessage = {
                        currIndex: siblingMessage.currIndex,
                        total: siblingMessage.total,
                        onPrevious: () =>
                            emit("switchToPrevious", siblingMessage.parentId || "", siblingMessage.currentId || ""),
                        onNext: () =>
                            emit("switchToNext", siblingMessage.parentId || "", siblingMessage.currentId || ""),
                    };
                }

                return config;
            }
        };

        // 创建响应式的配置计算属性
        const actionButtonsConfig = computed(() => getActionButtonsConfig(props.message));

        // 监听音频事件
        const setupAudioEventListeners = async () => {
            const audioChannel = backendStore.audioChannel as any;
            if (!audioChannel) return;

            // 检查是否有音频输出设备
            const deviceStatusRet = await backendStore.requestAudio("getDeviceStatus", "{}");
            try {
                let deviceStatus = JSON.parse(deviceStatusRet as string);
                isAudioOutputDeviceExists.value = (deviceStatus.inputValid && deviceStatus.outputValid) || false;
                console.log("deviceStatus:", deviceStatus);
            } catch (error) {
                console.error("Error checking audio output device:", error);
            }

            audioChannel.audioEvent.connect((event: number, id: string, json: string) => {
                console.log("Audio event:", event, id, json);
                switch (event) {
                    case AudioEvent.AeError: // AeError - 错误
                        console.error("Audio playback error:", json);
                        if (props.playingMessageId) {
                            emit("playingMessageIdChange", null);
                        }
                        break;
                    case AudioEvent.AePlayFinished: // AePlayFinished - 播放完成
                        if (props.playingMessageId) {
                            emit("playingMessageIdChange", null);
                        }
                        break;
                    case AudioEvent.AePlayerError: // AePlayerError - 播放器错误
                        console.error("AudioPlayer error:", json);
                        try {
                            const result = JSON.parse(json);
                            if (result.error && result.error === ErrorType.AudioNetworkError) {
                                console.error("AudioPlayer error:", result.error);
                                // 网络错误，提示用户检查网络
                                audioNetworkError.value = true;
                                return;
                            }
                        } catch (error) {
                            console.error("Error updating playing message ID:", error);
                        }
                        break;
                    case AudioEvent.AeRecordDeviceChanged: // AeRecordDeviceChanged - 录音设备改变
                        try {
                            const ret = JSON.parse(json);
                            console.log("AeRecordDeviceChanged ret:", ret);
                            if (ret && ret.valid) {
                                isAudioOutputDeviceExists.value = true;
                                break;
                            } else {
                                isAudioOutputDeviceExists.value = false;
                                break;
                            }
                        } catch (error) {
                            console.error("handleAudioDeviceChange error", json, error);
                        }
                        break;
                }
            });
        };

        // 加载大纲数据（从 workspace）
        const loadOutlineFromWorkspace = async () => {
            if (outlineLoading.value || loadedOutlineData.value) return;
            const outlineRef = props.message.render_message.find((item) => item.type === "outline");
            if (!outlineRef) return;
            const articleId = (outlineRef.data as OutlineRefData).id;
            if (!articleId) return;
            outlineLoading.value = true;
            const data = await conversationManagerStore.getWorkspaceOutline(
                conversationManagerStore.getCurrentConversationId,
                articleId,
            );
            if (data) {
                loadedOutlineData.value = data;
            }
            outlineLoading.value = false;
        };

        // 组件挂载时，设置音频事件监听器并加载大纲
        onMounted(() => {
            setupAudioEventListeners();
            const hasOutlineRef = props.message.render_message.some((item) => item.type === "outline");
            if (hasOutlineRef) {
                loadOutlineFromWorkspace();
            }
        });

        // 监听过期文件确认事件
        const stopWatchExpiredFiles = watch(
            () => uploadFilesStore.expiredFilesConfirmedVersion,
            () => {
                const pendingMessageId = uploadFilesStore.getPendingEditMessageId();
                if (pendingMessageId === props.message.id) {
                    // 获取存储的有效文件列表
                    const validFiles = uploadFilesStore.getValidFilesForEdit();
                    // 继续执行编辑操作，只处理有效文件
                    proceedWithEdit(props.message, validFiles || undefined);
                    uploadFilesStore.clearPendingEditMessage();
                }
            },
        );

        onUnmounted(() => {
            stopWatchExpiredFiles();
            // 如果当前消息有待编辑的状态，清理它
            if (uploadFilesStore.getPendingEditMessageId() === props.message.id) {
                uploadFilesStore.clearPendingEditMessage();
            }
        });

        return {
            handleMessageClick,
            handleOutlineChange,
            handleOutlineDeleteParagraph,
            handleOutlineDeleteSubsection,
            shouldShowLoading,
            handleEditClick,
            handleCopyClick,
            handlePlayClick,
            handleRetryClick,
            handleOpenCachedFile,
            handleMessageMouseEnter,
            handleMessageMouseLeave,
            getActionButtonsVisibilityClass,
            loadedOutlineData,
            outlineLoading,
            cachedDisplayFiles,
            FileAlignment,
            PopoverPlacement,
            PopoverAlign,
            currentConversationId: computed(() => conversationManagerStore.getCurrentConversationId),
            actionButtonsConfig,
            hasContent,
            previousMessage: computed(() => props.previousMessage), // 将 previousMessage 传递给子组件
            showOperationCanceledHint,
            isFromHistory: computed(() => props.isFromHistory), // 是否从历史记录中加载
        };
    },

    render() {
        const msg = this.$props.message;
        if (!msg) return null;

        return (
            <div class="message__item" onMouseleave={() => this.handleMessageMouseLeave()}>
                {/* 消息内容区 */}
                <div
                    class={[
                        "chat-bubbles__item",
                        msg.role === UserType.USER ? "chat-bubbles__item--user" : "chat-bubbles__item--ai",
                        msg.role === UserType.USER && this.cachedDisplayFiles.length > 0
                            ? "chat-bubbles__item--with-files"
                            : "",
                        msg.render_message.some(
                            (item) =>
                                item.type === ContentType.CntOutline ||
                                item.type === ContentType.CntReasoning ||
                                item.type === ContentType.CntAgentStep ||
                                item.type === ContentType.CntTool ||
                                (item.type === ContentType.CntText && msg.role === UserType.ASSISTANT),
                        )
                            ? "chat-bubbles__item--full-width"
                            : "",
                    ]
                        .filter(Boolean)
                        .join(" ")}
                    onClick={() => this.handleMessageClick(msg)}
                    onMouseenter={() => this.handleMessageMouseEnter(0)}
                >
                    {msg.role === UserType.USER && this.cachedDisplayFiles.length > 0 && (
                        <FileGroup
                            class="message__file-group"
                            fileList={this.cachedDisplayFiles}
                            deletable={false}
                            align={this.FileAlignment.Right}
                            popoverPlacement={this.PopoverPlacement.Bottom}
                            popoverAlign={this.PopoverAlign.Right}
                            onOpenFile={this.handleOpenCachedFile}
                        />
                    )}
                    <div class="chat-bubbles__bubble">
                        {msg.render_message.map((item, itemIndex) => {
                            // 检查表格内容
                            if (item.type === ContentType.CntText && "content" in item.data) {
                                const content = item.data.content;
                                if (typeof content === "string" && content.includes("|")) {
                                    console.log("[Message] Found table-like content:", {
                                        hasBar: content.includes("|"),
                                        hasTableTag: content.includes("<table"),
                                        preview: content.substring(0, 100),
                                    });
                                }
                            }

                            if (item.type === ContentType.CntText && "content" in item.data) {
                                // User messages: render as plain text
                                // AI messages: render with Markdown
                                if (msg.role === UserType.USER) {
                                    return (
                                        <div key={itemIndex} style="white-space: pre-wrap; line-height: 1.5;">
                                            {item.data.content}
                                        </div>
                                    );
                                } else {
                                    // Check if this is the last text item and streaming is active
                                    const isLastItem = itemIndex === msg.render_message.length - 1;
                                    const onlyTextItems = msg.render_message.filter(
                                        (item) => item.type === ContentType.CntText,
                                    );
                                    const isOnlyEmptyTextItem =
                                        onlyTextItems &&
                                        onlyTextItems.length === 1 &&
                                        (onlyTextItems[0]?.data as RenderData)?.content === "";
                                    const hasActiveReasoning = msg.render_message.some(
                                        (ri) =>
                                            ri.type === ContentType.CntReasoning &&
                                            (ri.data as ReasoningData).status !== 1,
                                    );
                                    const shouldAppendLoading =
                                        this.shouldShowLoading(msg) && isLastItem && !hasActiveReasoning;

                                    return (
                                        <MarkdownRenderer
                                            key={itemIndex}
                                            content={item.data.content}
                                            isUser={false}
                                            showLoading={shouldAppendLoading}
                                            isOnlyEmptyTextItem={isOnlyEmptyTextItem}
                                        />
                                    );
                                }
                            } else if (item.type === ContentType.CntOutline) {
                                // 引用格式：从 workspace 加载完整大纲数据
                                const outlineData = this.loadedOutlineData;
                                if (!outlineData) {
                                    // 正在加载或加载失败，显示标题占位
                                    const refData = item.data as OutlineRefData;
                                    return (
                                        <div key={itemIndex} class="outline-loading">
                                            {refData.title || "加载大纲中..."}
                                        </div>
                                    );
                                }
                                const isLastMessage =
                                    this.$props.isLastMessage && itemIndex === msg.render_message.length - 1;
                                return (
                                    <OutlineViewer
                                        key={itemIndex}
                                        data={outlineData}
                                        isLastMessage={isLastMessage}
                                        editable={isLastMessage}
                                        onDeleteParagraph={(index) => this.handleOutlineDeleteParagraph(0, index)}
                                        onDeleteSubsection={(pIndex, sIndex) =>
                                            this.handleOutlineDeleteSubsection(0, pIndex, sIndex)
                                        }
                                        onOutlineChange={(data: OutlineData) => this.handleOutlineChange(msg, data)}
                                    />
                                );
                            } else if (item.type === ContentType.CntReasoning) {
                                return <Reasoning key={itemIndex} data={item.data as ReasoningData} />;
                            } else if (item.type === ContentType.CntAgentStep) {
                                return <AgentStep key={itemIndex} data={item.data as AgentStepData} />;
                            } else if (item.type === ContentType.CntTool) {
                                const toolUseData = item.data as ToolUseData;
                                return <ToolUse key={itemIndex} data={toolUseData} />;
                            } else if (item.type === ContentType.CntDocCard) {
                                // doc_card：引用格式，显示文档卡片
                                const docData = item.data as DocCardData;
                                return (
                                    <DocCard
                                        key={itemIndex}
                                        data={docData}
                                        conversationId={this.currentConversationId}
                                        messageId={msg.id ?? ""}
                                        isNew={item.isNew === true}
                                        createdAt={(msg.extension?.created_at as number | string) || ""}
                                        onCardClick={(data: DocCardData) => {
                                            console.log("[Message] doc card clicked:", data.id);
                                        }}
                                        onCardOpened={() => {
                                            delete item.isNew;
                                        }}
                                    />
                                );
                            } else if (item.type === ContentType.CntError) {
                                const errorData = item.data as ErrorMsg;
                                return (
                                    <Error
                                        isLastMessage={this.$props.isLastMessage}
                                        hasContent={this.hasContent}
                                        key={itemIndex}
                                        data={errorData}
                                        model={msg.model_id}
                                        isFromHistory={this.isFromHistory} // 是否从历史记录中加载
                                        onOperationCanceled={() => {
                                            // 操作取消状态由父组件控制，这里不需要处理
                                        }}
                                    />
                                );
                            } else if (item.type === "command_card") {
                                // command_card：显示指令卡片，用于系统控制功能
                                const commandData = item.data as CommandCardData;
                                console.log("[Message] commandData:", commandData);

                                if (!this.$props.isLastMessage) {
                                    return null;
                                }

                                // 检查当前会话的showCards状态
                                const conversationRecord = useConversationManagerStore().conversionList.get(
                                    this.currentConversationId,
                                );
                                if (!conversationRecord?.showCards) {
                                    return null;
                                }

                                // 检查命令执行是否成功，如果存在错误则不显示控制卡片
                                if (commandData.errorCode && commandData.errorCode !== 0) {
                                    console.log(
                                        "[Message] Command execution failed, skipping card for:",
                                        commandData.toolName,
                                        "errorCode:",
                                        commandData.errorCode,
                                    );
                                    return null;
                                }

                                return (
                                    <CommandCard
                                        key={itemIndex}
                                        data={commandData}
                                        onCardClick={(data: CommandCardData) => {
                                            console.log("[Message] command card clicked:", data.toolName);
                                        }}
                                    />
                                );
                            }
                            return null;
                        })}
                    </div>
                </div>
                {/* 消息功能按钮区 */}
                <MessageActionButtons config={this.actionButtonsConfig} />
                {/* 停止后的提示信息，只有 ASSISTANT且最后一条消息才添加*/}
                {msg.role === UserType.ASSISTANT &&
                    this.$props.isLastMessage &&
                    this.$props.isOperationCanceled &&
                    this.showOperationCanceledHint && (
                        <OperationCanceledHint
                            onEdit={() => {
                                if (this.previousMessage) {
                                    this.handleEditClick(new MouseEvent("click"), this.previousMessage); // 模拟点击编辑按钮
                                }
                            }}
                        />
                    )}
            </div>
        );
    },
});
