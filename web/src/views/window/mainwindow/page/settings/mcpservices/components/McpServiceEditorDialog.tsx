import { computed, defineComponent, nextTick, ref, watch } from "vue";

import type { PropType } from "vue";
import type { DialogButton } from "@/types/dialog";
import { MCP_SERVICE_EDITOR_MODE, type McpServiceDraft, type McpServiceEditorMode } from "@/types/mcp-service";

import BaseDialog from "@/components/dialog/BaseDialog";
import ScrollBar from "@/components/ScrollBar";

import { useBackendStore } from "@/stores";

const DEFAULT_JSON_CONFIG = `{
  "mcpServers": {
    "my-custom-mcp": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/workspace"]
    }
  }
}`;

const normalizeLineBreaks = (value: string) => value.replace(/\r\n?/g, "\n");

const getEditablePlainText = (element: HTMLElement | null) => {
    if (!element) {
        return "";
    }

    return normalizeLineBreaks(element.textContent || "");
};

const syncEditablePlainText = (element: HTMLElement | null, value: string) => {
    if (!element) {
        return;
    }

    const normalizedValue = normalizeLineBreaks(value);
    if (getEditablePlainText(element) === normalizedValue) {
        return;
    }

    element.textContent = normalizedValue;
};

const insertPlainTextAtSelection = (text: string) => {
    const selection = window.getSelection();
    if (!selection || selection.rangeCount === 0) {
        return false;
    }

    const range = selection.getRangeAt(0);
    range.deleteContents();

    const textNode = document.createTextNode(text);
    range.insertNode(textNode);
    range.setStartAfter(textNode);
    range.collapse(true);

    selection.removeAllRanges();
    selection.addRange(range);
    return true;
};

type ScrollBarInstance = InstanceType<typeof ScrollBar>;

const getScrollContainer = (scrollBar: ScrollBarInstance | null | undefined) => {
    const containerRef = scrollBar?.scrollContainerRef as
        | HTMLElement
        | { value: HTMLElement | null }
        | null
        | undefined;

    if (!containerRef) {
        return null;
    }

    return containerRef instanceof HTMLElement ? containerRef : containerRef.value;
};

const scrollCaretIntoView = (scrollBar: ScrollBarInstance | null | undefined) => {
    const selection = window.getSelection();
    const container = getScrollContainer(scrollBar);
    if (!selection || selection.rangeCount === 0 || !container || !container.contains(selection.anchorNode)) {
        return;
    }

    const range = selection.getRangeAt(0).cloneRange();
    range.collapse(true);

    const caretRectCandidate =
        range.getClientRects().item(range.getClientRects().length - 1) || range.getBoundingClientRect();
    let caretRect = caretRectCandidate;
    let marker: HTMLSpanElement | null = null;

    if (!caretRect || (caretRect.width === 0 && caretRect.height === 0)) {
        marker = document.createElement("span");
        marker.textContent = "\u200b";
        range.insertNode(marker);

        const markerRange = document.createRange();
        markerRange.setStartAfter(marker);
        markerRange.collapse(true);
        selection.removeAllRanges();
        selection.addRange(markerRange);

        caretRect = marker.getBoundingClientRect();
    }

    if (!caretRect) {
        marker?.remove();
        return;
    }

    const containerRect = container.getBoundingClientRect();
    const padding = 12;

    if (caretRect.bottom > containerRect.bottom - padding) {
        container.scrollTop += caretRect.bottom - containerRect.bottom + padding;
    } else if (caretRect.top < containerRect.top + padding) {
        container.scrollTop -= containerRect.top + padding - caretRect.top;
    }

    marker?.remove();
};

export default defineComponent({
    name: "McpServiceEditorDialog",

    props: {
        visible: {
            type: Boolean,
            required: true,
        },
        mode: {
            type: String as PropType<McpServiceEditorMode>,
            required: true,
        },
        initialDraft: {
            type: Object as PropType<McpServiceDraft | null>,
            default: null,
        },
        serviceName: {
            type: String,
            default: "",
        },
        submitError: {
            type: String,
            default: "",
        },
    },

    emits: {
        close: () => true,
        saveService: (_draft: McpServiceDraft) => true,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const description = ref("");
        const jsonConfig = ref("");
        const isJsonWatermarkVisible = ref(true); // JSON 编辑器暗文是否显示
        const isDescriptionWatermarkVisible = ref(true); // 描述编辑器暗文是否显示
        const localErrorMessage = ref("");
        const jsonEditorRef = ref<HTMLDivElement | null>(null);
        const descriptionEditorRef = ref<HTMLDivElement | null>(null);
        const jsonScrollBarRef = ref<ScrollBarInstance | null>(null);
        const descriptionScrollBarRef = ref<ScrollBarInstance | null>(null);

        const syncEditableFields = () => {
            syncEditablePlainText(jsonEditorRef.value, jsonConfig.value);
            syncEditablePlainText(descriptionEditorRef.value, description.value);
        };

        const syncFormState = () => {
            description.value = props.initialDraft?.description || "";
            const draftConfig = props.initialDraft?.jsonConfig || "";
            jsonConfig.value = draftConfig;
            // 有草稿内容时暗文隐藏；无草稿时显示默认暗文
            isJsonWatermarkVisible.value = !draftConfig;
            isDescriptionWatermarkVisible.value = !description.value;
            localErrorMessage.value = "";
            void nextTick(() => {
                syncEditableFields();
            });
        };

        watch(
            () => [props.visible, props.initialDraft],
            () => {
                if (props.visible) {
                    syncFormState();
                }
            },
            {
                immediate: true,
            },
        );

        const handleClose = () => {
            emit("close");
        };

        const handleDescriptionInput = (event: Event) => {
            const currentTarget = event.currentTarget as HTMLDivElement | null;
            description.value = getEditablePlainText(currentTarget);
            isDescriptionWatermarkVisible.value = !description.value;
        };

        const handleConfigInput = (event: Event) => {
            const currentTarget = event.currentTarget as HTMLDivElement | null;
            jsonConfig.value = getEditablePlainText(currentTarget);
            localErrorMessage.value = "";
            // 用户首次输入时隐藏暗文；如果用户输入后内容恰好等于默认模板，恢复暗文样式
            isJsonWatermarkVisible.value = !jsonConfig.value;
        };

        const handlePlainTextPaste = (
            event: ClipboardEvent,
            onInput: (currentTarget: HTMLDivElement | null) => void,
            scrollBar: ScrollBarInstance | null | undefined,
        ) => {
            const currentTarget = event.currentTarget as HTMLDivElement | null;
            const pastedText = normalizeLineBreaks(event.clipboardData?.getData("text/plain") || "");

            event.preventDefault();

            if (pastedText && !document.execCommand("insertText", false, pastedText)) {
                insertPlainTextAtSelection(pastedText);
            }

            requestAnimationFrame(() => {
                onInput(currentTarget);
                scrollCaretIntoView(scrollBar);
            });
        };

        const handleEditorKeyDown = (
            event: KeyboardEvent,
            onInput: (currentTarget: HTMLDivElement | null) => void,
            scrollBar: ScrollBarInstance | null | undefined,
        ) => {
            if (event.key !== "Enter" || event.isComposing) {
                return;
            }

            const currentTarget = event.currentTarget as HTMLDivElement | null;

            event.preventDefault();

            if (!document.execCommand("insertText", false, "\n")) {
                insertPlainTextAtSelection("\n");
            }

            requestAnimationFrame(() => {
                onInput(currentTarget);
                scrollCaretIntoView(scrollBar);
            });
        };

        const handleConfigPaste = (event: ClipboardEvent) => {
            handlePlainTextPaste(
                event,
                (currentTarget) => {
                    jsonConfig.value = getEditablePlainText(currentTarget);
                    localErrorMessage.value = "";
                    isJsonWatermarkVisible.value = !jsonConfig.value;
                },
                jsonScrollBarRef.value,
            );
        };

        const handleDescriptionPaste = (event: ClipboardEvent) => {
            handlePlainTextPaste(
                event,
                (currentTarget) => {
                    description.value = getEditablePlainText(currentTarget);
                    isDescriptionWatermarkVisible.value = !description.value;
                },
                descriptionScrollBarRef.value,
            );
        };

        const handleConfigKeyDown = (event: KeyboardEvent) => {
            handleEditorKeyDown(
                event,
                (currentTarget) => {
                    jsonConfig.value = getEditablePlainText(currentTarget);
                    localErrorMessage.value = "";
                },
                jsonScrollBarRef.value,
            );
        };

        const handleDescriptionKeyDown = (event: KeyboardEvent) => {
            handleEditorKeyDown(
                event,
                (currentTarget) => {
                    description.value = getEditablePlainText(currentTarget);
                },
                descriptionScrollBarRef.value,
            );
        };

        const focusEditor = (editorRef: typeof jsonEditorRef) => {
            editorRef.value?.focus();
        };

        const handleJsonShellClick = (event: MouseEvent) => handleShellMouseDown(event, jsonEditorRef);
        const handleDescriptionShellClick = (event: MouseEvent) => handleShellMouseDown(event, descriptionEditorRef);

        // 阻止点击编辑器时事件冒泡到 shell，避免先失焦再聚焦导致闪烁
        const handleEditorClick = (event: MouseEvent) => {
            event.stopPropagation();
        };

        // mousedown 阶段阻止冒泡，避免 preventDefault 阻止文本选择
        const handleEditorMouseDown = (event: MouseEvent) => {
            event.stopPropagation();
        };

        // mousedown 阶段就聚焦，避免 click 阶段编辑器已失焦导致闪烁
        const handleShellMouseDown = (event: MouseEvent, editorRef: typeof jsonEditorRef) => {
            // 阻止浏览器默认的 focus 行为（点击 shell 会让当前焦点元素失焦）
            event.preventDefault();
            editorRef.value?.focus();
        };

        const validateJsonConfig = () => {
            localErrorMessage.value = "";

            try {
                JSON.parse(jsonConfig.value);
            } catch (_error) {
                localErrorMessage.value = "JSON 配置格式不合法，请检查后重试。";
                return false;
            }

            return true;
        };

        const dialogButtons = computed<DialogButton[]>(() => [
            {
                key: "cancel",
                text: backend.translate("Cancel"),
                type: "default",
            },
            {
                key: "confirm",
                text: backend.translate("Confirm"),
                type: "primary",
                suggested: true,
                beforeClick: validateJsonConfig,
            },
        ]);

        const handleDialogAction = (key: string) => {
            if (key === "cancel") {
                emit("close");
                return;
            }

            emit("saveService", {
                id: props.initialDraft?.id,
                description: description.value,
                jsonConfig: jsonConfig.value,
            });
        };

        const descriptionText = computed(() => backend.translate("Describe"));
        const descriptionPlaceholder = computed(() =>
            backend.translate("Describe MCP server functions to facilitate quick search tools"),
        );
        const jsonConfigText = computed(() => backend.translate("JSON configuration"));
        const jsonConfigHintText = computed(() =>
            backend.translate("Please paste the MCP JSON configuration code into the input box."),
        );

        return {
            description,
            jsonConfig,
            dialogButtons,
            title: computed(() =>
                props.mode === MCP_SERVICE_EDITOR_MODE.EDIT
                    ? backend.translate("Edit MCP Server")
                    : backend.translate("Add MCP Server"),
            ),
            errorMessage: computed(() => localErrorMessage.value || props.submitError),
            handleClose,
            handleDialogAction,
            handleDescriptionInput,
            handleConfigInput,
            handleConfigPaste,
            handleDescriptionPaste,
            handleConfigKeyDown,
            handleDescriptionKeyDown,
            descriptionText,
            jsonConfigText,
            isJsonWatermarkVisible,
            isDescriptionWatermarkVisible,
            isDefaultJsonConfig: computed(() => jsonConfig.value === DEFAULT_JSON_CONFIG),
            jsonConfigHintText,
            descriptionPlaceholder,
            jsonEditorRef,
            descriptionEditorRef,
            jsonScrollBarRef,
            descriptionScrollBarRef,
            handleJsonShellClick,
            handleDescriptionShellClick,
            handleEditorClick,
            handleEditorMouseDown,
        };
    },

    render() {
        return (
            <BaseDialog
                visible={this.$props.visible}
                title={this.title}
                titlePlacement="header"
                closeOnOverlayClick
                dialogClass="mcp-service-editor-dialog"
                buttons={this.dialogButtons}
                onCancel={this.handleClose}
                onButtonClick={this.handleDialogAction}
            >
                <label class={["mcp-service-editor-dialog__field", "mcp-service-editor-dialog__field--grow"]}>
                    <span class="mcp-service-editor-dialog__label-row">
                        <span class="mcp-service-editor-dialog__label-main">
                            <span class="mcp-service-editor-dialog__required">*</span>
                            <span
                                class={["mcp-service-editor-dialog__label", "mcp-service-editor-dialog__label--inline"]}
                            >
                                {this.jsonConfigText}
                            </span>
                        </span>
                        <span class="mcp-service-editor-dialog__label-description">{this.jsonConfigHintText}</span>
                    </span>
                    <div
                        class={[
                            "mcp-service-editor-dialog__editor-shell",
                            "mcp-service-editor-dialog__editor-shell--json",
                        ]}
                        onMousedown={this.handleJsonShellClick}
                    >
                        <ScrollBar
                            ref="jsonScrollBarRef"
                            autoHide={false}
                            class="mcp-service-editor-dialog__editor-scroll"
                            momentum
                        >
                            <div
                                ref="jsonEditorRef"
                                class={[
                                    "mcp-service-editor-dialog__editor",
                                    "mcp-service-editor-dialog__editor--json",
                                    !this.jsonConfig && "mcp-service-editor-dialog__editor--empty",
                                    this.isJsonWatermarkVisible && "mcp-service-editor-dialog__editor--watermark",
                                    this.isDefaultJsonConfig && "mcp-service-editor-dialog__editor--default",
                                ]}
                                contenteditable="true"
                                data-placeholder=""
                                data-watermark={DEFAULT_JSON_CONFIG}
                                role="textbox"
                                aria-multiline="true"
                                spellcheck={false}
                                onInput={this.handleConfigInput}
                                onKeydown={this.handleConfigKeyDown}
                                onPaste={this.handleConfigPaste}
                                onClick={this.handleEditorClick}
                                onMousedown={this.handleEditorMouseDown}
                            />
                        </ScrollBar>
                    </div>
                </label>

                <label class="mcp-service-editor-dialog__field">
                    <span class="mcp-service-editor-dialog__label">{this.descriptionText}</span>
                    <div
                        class={[
                            "mcp-service-editor-dialog__editor-shell",
                            "mcp-service-editor-dialog__editor-shell--description",
                        ]}
                        onMousedown={this.handleDescriptionShellClick}
                    >
                        <ScrollBar
                            ref="descriptionScrollBarRef"
                            autoHide={false}
                            class="mcp-service-editor-dialog__editor-scroll"
                            momentum
                        >
                            <div
                                ref="descriptionEditorRef"
                                class={[
                                    "mcp-service-editor-dialog__editor",
                                    "mcp-service-editor-dialog__editor--description",
                                    this.isDescriptionWatermarkVisible && "mcp-service-editor-dialog__editor--watermark",
                                ]}
                                contenteditable="true"
                                data-watermark={this.descriptionPlaceholder}
                                role="textbox"
                                aria-multiline="true"
                                onInput={this.handleDescriptionInput}
                                onKeydown={this.handleDescriptionKeyDown}
                                onPaste={this.handleDescriptionPaste}
                                onClick={this.handleEditorClick}
                                onMousedown={this.handleEditorMouseDown}
                            />
                        </ScrollBar>
                    </div>
                </label>

                {this.errorMessage && <div class="mcp-service-editor-dialog__error">{this.errorMessage}</div>}
            </BaseDialog>
        );
    },
});
