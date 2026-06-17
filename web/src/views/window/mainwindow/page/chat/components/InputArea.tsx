import { defineComponent, ref, computed, onMounted, onBeforeUnmount, watch, type PropType, nextTick } from "vue";
import TemplateEditor from "./templateEditor/TemplateEditor";
import InputAreaAction from "./inputAreaAction/inputAreaAction";
import FileGroup from "@/components/filegroup/FileGroup";
import { useUploadFilesStore, useBackendStore, useNetworkStore } from "@/stores";
import { useCurrentConversationPresentation } from "@/composables/useCurrentConversationPresentation";
import type { DisplayFile } from "@/types/file";
import { DocParsingFileType, DocFileCategory, FileAlignment, PopoverAlign, PopoverPlacement } from "@/types/file";
import type { MenuItem } from "@/types/menu";
import { formatFileSize, getFileType } from "@/utils/filehelper";
import { AudioEvent } from "@/types/DigitalHuman";
import { ErrorType } from "@/types/errortype";

export default defineComponent({
    name: "InputArea",

    props: {
        // 输入框的值
        modelValue: {
            type: String,
            default: "",
        },
        // 占位符文本
        placeholder: {
            type: String,
            default: "请输入内容...",
        },
        // 是否允许发送（支持“仅附件”场景）
        canSend: {
            type: Boolean,
            default: false,
        },
        // 是否禁用
        disabled: {
            type: Boolean,
            default: false,
        },
        // 最大长度
        maxlength: {
            type: Number,
            default: undefined,
        },
        // 行数
        rows: {
            type: Number,
            default: 4,
        },
        // 是否自动调整高度
        autoResize: {
            type: Boolean,
            default: false,
        },
        // 发送按钮文本
        sendButtonText: {
            type: String,
            default: "发送",
        },
        // 是否显示发送按钮
        showSendButton: {
            type: Boolean,
            default: true,
        },
        // 是否在发送/停止模式下（停止模式下不检查输入是否为空）
        isSending: {
            type: Boolean,
            default: false,
        },
        // 发送按钮被业务规则拦截时的原因；按钮保持可点击，让上层展示提示。
        sendBlockedReason: {
            type: String,
            default: "",
        },
        actionMenuItems: {
            type: Array as PropType<MenuItem[]>,
            default: () => [],
        },
    },

    emits: {
        // 更新值事件
        updateValue: (value: string) => {
            return typeof value === "string";
        },
        // 输入事件
        input: (value: string) => {
            return typeof value === "string";
        },
        // 获得焦点事件
        focus: null,
        // 失去焦点事件
        blur: null,
        // 按下 Enter 键事件
        enter: null,
        // 发送按钮点击事件
        send: null,
        // 深度思考按钮点击事件
        deepThink: null,
        // 联网搜索按钮点击事件
        webSearch: null,
        // 输入区菜单动作点击事件
        selectAction: (actionId: string) => {
            return typeof actionId === "string";
        },
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const networkStore = useNetworkStore();
        const { currentConversationPresentation } = useCurrentConversationPresentation();

        const textareaRef = ref<InstanceType<typeof TemplateEditor> | null>(null);
        const uploadFilesStore = useUploadFilesStore();
        const backend = useBackendStore();

        // 输入区域焦点状态（点击 input-area 区域内为 true，点击外部为 false）
        const isFocused = ref(false);
        const inputAreaRef = ref<HTMLDivElement | null>(null);

        // 网络和设备状态
        const isNetworkOnline = computed(() => networkStore.isNetworkOnline);
        const isAudioInputDeviceExists = ref(false);
        const audioNetworkError = ref(false);

        // Convert UploadFile to DisplayFile for FileGroup component
        const displayFiles = computed(() => {
            return uploadFilesStore.uploadedFiles.map((file, index) => ({
                index,
                fileNameText: file.fileName,
                filePath: file.filePath,
                type: getDocParsingFileType(file.fileName),
                imgBase64: file.icon,
                fileSize: file.fileSize,
                category: file.category !== undefined ? (file.category as unknown as DocFileCategory) : undefined,
                fileType: getFileType(file.fileName, (key) => backend.translate(key)),
                fileSizeText: formatFileSize(file.fileSize),
                parseStatus: file.parseStatus,
            }));
        });

        // Get file type from file name extension
        const getDocParsingFileType = (fileName: string): DocParsingFileType => {
            if (!fileName) return DocParsingFileType.Doc;

            const ext = fileName.split(".").pop()?.toLowerCase() || "";
            const imageExtensions = ["jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"];
            return imageExtensions.includes(ext) ? DocParsingFileType.Image : DocParsingFileType.Doc;
        };

        // Handle file click
        const handleOpenFile = async (filePath: string) => {
            console.log("Open file:", filePath);
            await backendStore.requestSystem("openFile", filePath);
        };

        // Handle file delete
        const handleDeleteFile = async (file: DisplayFile) => {
            await uploadFilesStore.removeFile(file.filePath);
        };

        // Voice recording state
        const recording = ref(false);
        const voiceRequestPending = ref(false);
        const startQus = ref("");
        const endQus = ref("");
        const audioLevel = ref(0);

        // 处理输入事件（TemplateEditor 已解析好 value，直接接收 string）
        const handleInput = (value: string) => {
            emit("updateValue", value);
            emit("input", value);
        };

        // 处理获得焦点事件
        const handleFocus = () => {
            if (!isFocused.value) {
                isFocused.value = true;
            }
            emit("focus");
        };

        // 处理失去焦点事件
        const handleBlur = () => {
            nextTick(() => {
                const activeElement = document.activeElement;
                if (inputAreaRef.value && !inputAreaRef.value.contains(activeElement)) {
                    isFocused.value = false;
                }
            });
            emit("blur");
        };

        // Enter 由 TemplateEditor 内部处理后向上 emit
        const handleEnter = () => {
            emit("enter");
        };

        // 处理发送按钮点击事件
        const handleSendClick = () => {
            emit("send");
        };

        // 处理录音按钮点击
        const handleVoice = async () => {
            //TODO:实现直接插入当前光标位置
            // 防止快速重复点击：如果请求正在进行，忽略
            if (voiceRequestPending.value) {
                return;
            }

            // 如果正在录音，则停止录音
            if (recording.value) {
                voiceRequestPending.value = true;
                try {
                    await backendStore.requestAudio("stopRecorder", "{}");
                    // 停止录音成功后重置状态
                    recording.value = false;
                    audioLevel.value = 0;
                    startQus.value = "";
                    endQus.value = "";
                } catch (error) {
                    console.error("Failed to stop recorder:", error);
                } finally {
                    voiceRequestPending.value = false;
                }
                return;
            }

            // 停止正在播放的音频
            try {
                await backendStore.requestAudio("stopPlayTextAudio", "{}");
            } catch (error) {
                console.error("Failed to stop playback:", error);
            }

            // 获取当前选区和文本
            const selection = window.getSelection();
            const currentText = props.modelValue;

            // 计算光标位置的偏移量
            let cursorOffset = 0;
            if (selection && selection.rangeCount > 0) {
                const range = selection.getRangeAt(0);
                const container = textareaRef.value?.containerRef;
                if (container && container.contains(range.startContainer)) {
                    // 计算在编辑器内的光标位置
                    cursorOffset = getTextOffsetInContainer(container, range);
                }
            }

            // 保存光标前后的文本
            startQus.value = currentText.slice(0, cursorOffset);
            endQus.value = currentText.slice(cursorOffset);

            // 开始录音
            voiceRequestPending.value = true;
            try {
                await backendStore.requestAudio("startRecorder", "{}");
                recording.value = true;
            } catch (error) {
                console.error("Failed to start recorder:", error);
            } finally {
                voiceRequestPending.value = false;
            }
        };

        // 计算选区在容器内的文本偏移量
        const getTextOffsetInContainer = (container: HTMLElement, range: Range): number => {
            let offset = 0;
            const walker = document.createTreeWalker(container, NodeFilter.SHOW_TEXT, null);

            let node: Node | null = walker.currentNode;
            while (node) {
                if (node === range.startContainer) {
                    offset += range.startOffset;
                    break;
                }
                offset += node.textContent?.length || 0;
                node = walker.nextNode();
            }

            return offset;
        };

        // 处理音频级别更新事件
        const handleAudioLevel = (event: number, id: string, json: string) => {
            if (event === AudioEvent.AeLevelUpdated) {
                // AeLevelUpdated
                try {
                    const data = JSON.parse(json);
                    audioLevel.value = data.level || 0;
                } catch (error) {
                    console.error("Failed to parse audio level data:", error);
                }
            }
        };

        // 处理 ASR 文本接收事件
        const handleASRText = (event: number, id: string, json: string) => {
            if (event === AudioEvent.AeTextReceived) {
                // AeTextReceived
                try {
                    const data = JSON.parse(json);
                    const text = data.text || "";
                    const isEnd = data.isEnd || false;

                    if (recording.value && !(isEnd && text === "")) {
                        // 更新输入框内容
                        const newText = startQus.value + text + endQus.value;
                        emit("updateValue", newText);
                        emit("input", newText);
                    }

                    // 如果录音结束
                    if (isEnd) {
                        // 停止录音
                        stopRecording();

                        // 聚焦到输入框并将光标移动到文本末尾
                        nextTick(() => {
                            textareaRef.value?.focus();
                        });
                    }
                } catch (error) {
                    console.error("Failed to parse ASR text data:", error);
                }
            }
        };

        // 处理录音错误事件
        const handleRecordError = async (event: number, id: string, json: string) => {
            if (event === AudioEvent.AeRecordError) {
                // AeRecordError
                console.error("Recording error:", json);
                await stopRecording();
            }
        };

        // 处理录音设备变更事件
        const handleRecordDeviceChanged = (event: number, id: string, json: string) => {
            if (event === AudioEvent.AeRecordDeviceChanged) {
                // AeRecordDeviceChanged
                console.log("Recording device changed:", json);
                try {
                    const ret = JSON.parse(json);
                    if (ret && ret.valid) {
                        isAudioInputDeviceExists.value = true;
                    } else {
                        isAudioInputDeviceExists.value = false;
                    }
                } catch (error) {
                    console.error("handleRecordDeviceChanged error", json, error);
                }
            }
        };

        // 处理音频错误事件
        const handleAudioError = (event: number, id: string, json: string) => {
            if (
                event === AudioEvent.AeError ||
                event === AudioEvent.AePlayerError ||
                event === AudioEvent.AeRecordError
            ) {
                // AePlayerError - 播放器错误
                console.error("AudioPlayer error:", json);
                try {
                    const result = JSON.parse(json);
                    if (result.error && result.error === ErrorType.AudioNetworkError) {
                        console.error("Audio network error:", result.error);
                        audioNetworkError.value = true;
                        stopRecording();
                    }
                } catch (error) {
                    console.error("Error parsing audio player error:", error);
                }
            }
        };

        // 停止录音
        const stopRecording = async () => {
            if (!recording.value) return;

            try {
                await backendStore.requestAudio("stopRecorder", "{}");
            } catch (error) {
                console.error("Failed to stop recorder:", error);
            }

            // 重置状态
            recording.value = false;
            audioLevel.value = 0;
            startQus.value = "";
            endQus.value = "";
        };

        // 音频事件处理器（统一处理所有音频事件）
        const handleAudioEvent = (event: number, id: string, json: string) => {
            console.log("Received audio event:", event, id, json);
            switch (event) {
                case AudioEvent.AeLevelUpdated:
                    handleAudioLevel(event, id, json);
                    break;
                case AudioEvent.AeTextReceived:
                    handleASRText(event, id, json);
                    break;
                case AudioEvent.AeRecordDeviceChanged:
                    handleRecordDeviceChanged(event, id, json);
                    break;
                case AudioEvent.AeError:
                case AudioEvent.AeRecordError:
                case AudioEvent.AePlayerError:
                    handleAudioError(event, id, json);
                    break;
                default:
                    break;
            }
        };

        // 设置音频事件监听器
        const setupAudioEventListeners = async () => {
            console.log("Setting up audio event listeners");
            const audio = backendStore.audioChannel as any;
            if (!audio || !audio.audioEvent) return;

            // 检查是否有音频输入设备
            try {
                const deviceStatusRet = await backendStore.requestAudio("getDeviceStatus", "{}");
                const deviceStatus = JSON.parse(deviceStatusRet as string);
                isAudioInputDeviceExists.value = (deviceStatus.inputValid && deviceStatus.outputValid) || false;
                console.log("deviceStatus:", deviceStatus);
            } catch (error) {
                console.error("Error checking audio input device:", error);
            }

            audio.audioEvent.connect(handleAudioEvent);
        };

        // 移除音频事件监听器
        const cleanupAudioEventListeners = () => {
            console.log("Disconnecting audio event handler");
            const audio = backendStore.audioChannel as any;
            if (!audio || !audio.audioEvent) return;

            audio.audioEvent.disconnect(handleAudioEvent);
        };

        // Capture-phase paste: always intercept before milkdown sees it.
        // QtWebEngine's Chromium sandbox never exposes file:// URIs or File
        // objects via clipboardData, so file detection must happen in C++ by
        // reading the native Qt clipboard.  We let C++ decide:
        //   ""        → files/images are being processed (FeFileReady follows)
        //   non-empty → plain text; insert it at the cursor ourselves
        const handlePasteCapture = async (event: ClipboardEvent) => {
            event.preventDefault();
            event.stopPropagation();
            const text = (await backendStore.requestFile("processClipboardData")) as string;
            if (text) {
                textareaRef.value?.insertTextAtCursor(text);
            }
        };

        // 处理 input-area 区域内的点击事件
        const handleInputAreaMouseDown = (event: MouseEvent) => {
            const editorContainer = textareaRef.value?.containerRef;
            const clickedOnEditor = editorContainer?.contains(event.target as Node);

            if (clickedOnEditor) {
                // 点击在编辑器上，让浏览器自然处理光标定位
                if (!isFocused.value) {
                    isFocused.value = true;
                }
                return;
            }

            // 点击在 input-area 内但编辑器外（如附件区、空白区域）
            event.preventDefault(); // 阻止浏览器默认的焦点变更

            if (!isFocused.value) {
                isFocused.value = true;
                if (!props.disabled) {
                    textareaRef.value?.focus();
                }
            }
            // 已聚焦时：preventDefault 阻止焦点丢失，不调用 focus() 避免光标重置
        };

        // 处理文档级别的点击事件，检测是否点击在 input-area 外部
        const handleDocumentMouseDown = (event: MouseEvent) => {
            if (!isFocused.value) return;

            const target = event.target as Node;
            const inputAreaElement = inputAreaRef.value;

            // 如果点击在 input-area 区域外部，取消焦点状态
            if (inputAreaElement && !inputAreaElement.contains(target)) {
                isFocused.value = false;
            }
        };

        // 当输入区域聚焦时，在 capture 阶段拦截 Enter，阻止其他全局监听器（如 BashApprove）吞掉事件
        const handleWindowKeyDownCapture = (e: KeyboardEvent) => {
            // 仅拦截无修饰键的纯 Enter，带有任意修饰键(ctrl,shift,alt,meta)的组合交由编辑器处理换行
            const hasModifier = e.ctrlKey || e.shiftKey || e.altKey || e.metaKey;
            if (e.key === "Enter" && !hasModifier && isFocused.value) {
                e.stopImmediatePropagation();
                emit("enter");
            }
        };

        // 组件挂载时设置事件监听器
        onMounted(() => {
            setupAudioEventListeners();
            // 添加文档级别的点击事件监听
            document.addEventListener("mousedown", handleDocumentMouseDown);
            window.addEventListener("keydown", handleWindowKeyDownCapture, { capture: true });
        });

        // 监听网络状态变化，网络断开时停止录音
        watch(isNetworkOnline, (newValue) => {
            if (!newValue && recording.value) {
                console.log("Network offline, stopping recording");
                stopRecording();
            }
        });

        // 组件卸载前清理事件监听器
        onBeforeUnmount(() => {
            cleanupAudioEventListeners();
            // 移除文档级别的点击事件监听
            document.removeEventListener("mousedown", handleDocumentMouseDown);
            window.removeEventListener("keydown", handleWindowKeyDownCapture, true);
            // 如果正在录音，停止它
            if (recording.value) {
                stopRecording();
            }
        });

        // 监听 InputAreaAction 的深度思考事件
        return {
            textareaRef,
            inputAreaRef,
            isFocused,
            displayFiles,
            handleInput,
            handleFocus,
            handleBlur,
            handleEnter,
            handleSendClick,
            handleOpenFile,
            handleDeleteFile,
            handleVoice,
            handlePasteCapture,
            handleInputAreaMouseDown,
            recording,
            audioLevel,
            currentConversationPresentation,
            isNetworkOnline,
            isAudioInputDeviceExists,
            audioNetworkError,
            // Expose focus method
            focus: () => {
                isFocused.value = true;
                textareaRef.value?.focus();
            },
        };
    },

    render() {
        const hasUploadedFiles = this.displayFiles.length > 0;

        return (
            <div
                ref="inputAreaRef"
                class={[
                    "input-area",
                    {
                        "input-area--dashed":
                            this.isFocused && this.currentConversationPresentation.inputArea.borderStyle === "dashed",
                        "input-area--focused":
                            this.isFocused && this.currentConversationPresentation.inputArea.borderStyle !== "dashed",
                    },
                ]}
                onMousedown={this.handleInputAreaMouseDown}
                onPasteCapture={this.handlePasteCapture}
            >
                {/* Attachment display area */}
                {hasUploadedFiles && (
                    <div class="input-area__attachments">
                        <FileGroup
                            fileList={this.displayFiles}
                            isWindowMode={true}
                            onOpenFile={this.handleOpenFile}
                            onDeleteFile={this.handleDeleteFile}
                            align={FileAlignment.Left}
                            popoverAlign={PopoverAlign.Right}
                            popoverPlacement={PopoverPlacement.Top}
                            showSingleImage={true}
                            imageDisplayMode={"small"}
                        />
                    </div>
                )}
                <TemplateEditor
                    ref="textareaRef"
                    class="input-area__field"
                    modelValue={this.$props.modelValue}
                    placeholder={this.$props.placeholder}
                    disabled={this.$props.disabled}
                    maxlength={this.$props.maxlength}
                    onUpdateValue={this.handleInput}
                    onFocus={this.handleFocus}
                    onBlur={this.handleBlur}
                    onEnter={this.handleEnter}
                />
                {/* 按钮区域 */}
                <InputAreaAction
                    showSendButton={this.$props.showSendButton}
                    isSending={this.$props.isSending}
                    modelValue={this.$props.modelValue}
                    canSend={this.$props.canSend}
                    sendBlockedReason={this.$props.sendBlockedReason}
                    disabled={this.$props.disabled}
                    actionMenuItems={this.$props.actionMenuItems}
                    recording={this.recording}
                    audioLevel={this.audioLevel}
                    isNetworkOnline={this.isNetworkOnline}
                    isAudioInputDeviceExists={this.isAudioInputDeviceExists}
                    audioNetworkError={this.audioNetworkError}
                    onSend={this.handleSendClick}
                    onVoice={this.handleVoice}
                    onSelectAction={(actionId: string) => this.$emit("selectAction", actionId)}
                >
                    {this.$slots.default?.()}
                </InputAreaAction>
            </div>
        );
    },
});
