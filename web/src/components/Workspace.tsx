import { defineComponent, ref, onMounted, onUnmounted, computed, provide, watch, type PropType } from "vue";
import { useMainWindowStore, useExtensionPanelStore, useUploadFilesStore, useBackendStore } from "@/stores";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import { EXTENSION_PANEL_KEY, type ExtensionPanelAPI } from "@/types/extension-panel";
import ExtensionPanel from "@/components/ExtensionPanel";
import Splitter from "@/components/Splitter";
import SvgIcon from "@/components/SvgIcon";
import FileTypeSelectDialog from "@/components/FileTypeSelectDialog";
import ExpiredFilesDialog from "@/components/ExpiredFilesDialog";
import { FileCategory } from "@/types/uploadfile";
import type {
    MainWindowWorkspaceFileDropAction,
    MainWindowWorkspaceFileDropCategorySelectAction,
    MainWindowWorkspaceFileDropContext,
} from "@/utils/mainwindow/workspacePages";
import Footer from "@/views/window/mainwindow/footer/Footer";
import { useTitleBarState } from "@/composables/useTitleBarState";

export default defineComponent({
    name: "Workspace",
    props: {
        acceptFileDrop: {
            type: Boolean,
            default: false,
        },
        fileDropHandler: {
            type: Function as PropType<
                (
                    context: MainWindowWorkspaceFileDropContext,
                ) =>
                    | MainWindowWorkspaceFileDropAction
                    | null
                    | undefined
                    | Promise<MainWindowWorkspaceFileDropAction | null | undefined>
            >,
            default: undefined,
        },
    },
    components: {
        FileTypeSelectDialog,
        ExpiredFilesDialog,
        ExtensionPanel,
        Splitter,
    },
    setup(props) {
        const mainWindowStore = useMainWindowStore();
        const extensionStore = useExtensionPanelStore();
        const uploadFilesStore = useUploadFilesStore();
        const backend = useBackendStore();
        const isDraggingFiles = ref(false);
        const dragDepth = ref(0);
        const workspaceContentRef = ref<HTMLElement | null>(null);
        const fileTypeSelectDialogConfig = ref<MainWindowWorkspaceFileDropCategorySelectAction | null>(null);
        const fileTypeSelectDialogPaths = ref<string[]>([]);

        // Create ExtensionPanelAPI and provide to descendants
        const api: ExtensionPanelAPI = {
            openExtensionPanel: extensionStore.openExtensionPanel,
            openExtensionPanelWithOptions: extensionStore.openExtensionPanelWithOptions,
            closeExtensionPanel: extensionStore.closeExtensionPanel,
            toggleExtensionPanel: extensionStore.toggleExtensionPanel,
            setChatAreaVisible: extensionStore.setChatAreaVisible,
            setPanelFullscreen: extensionStore.setPanelFullscreen,
        };

        provide(EXTENSION_PANEL_KEY, api);

        // Layout state
        const containerClass = computed(() => {
            const showChat = extensionStore.showChatArea;
            const showExt = extensionStore.showExtensionPanel;
            const panelFS = extensionStore.panelFullscreen;

            const classes = ["workspace-content"];

            if (panelFS && showExt) {
                // 面板全屏：聊天区动画滑出，扩展面板充满工作区
                classes.push("workspace-content--editor-fullscreen");
            } else if (showChat && showExt) {
                classes.push("workspace-content--both-visible");
            } else if (showChat) {
                classes.push("workspace-content--chat-only");
            } else if (showExt) {
                classes.push("workspace-content--extension-only");
            }

            return classes.join(" ");
        });

        const showSplitter = computed(() => {
            return extensionStore.showChatArea && extensionStore.showExtensionPanel && !extensionStore.panelFullscreen;
        });

        const handlePanelClose = () => {
            extensionStore.closeExtensionPanel();
        };

        const dropzoneTitle = computed(() => backend.translate("Drag files here to add them."));
        const dropzoneDescription = computed(() =>
            backend.translate(
                "You can only add 50 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.",
            ),
        );
        const dropzoneUploadIcon = computed(() =>
            mainWindowStore.isDarkMode ? "img_upload_file_dark" : "img_upload_file",
        );

        const closeFileTypeSelectDialog = () => {
            fileTypeSelectDialogConfig.value = null;
            fileTypeSelectDialogPaths.value = [];
        };

        const processDroppedFiles = async (paths: string[], category?: FileCategory) => {
            if (paths.length === 0) {
                return;
            }

            try {
                await uploadFilesStore.handleDroppedFiles(paths, "", { category });
            } catch (error) {
                console.error("[Workspace] Failed to process dropped files:", error);
            }
        };

        const handleFileDropAction = async (paths: string[]) => {
            const action = await props.fileDropHandler?.({ paths });

            if (!action) {
                await processDroppedFiles(paths);
                return;
            }

            if (action.type === "upload") {
                await processDroppedFiles(paths, action.category);
                return;
            }

            fileTypeSelectDialogPaths.value = [...paths];
            fileTypeSelectDialogConfig.value = action;
        };

        const handleFileTypeSelect = (index: number) => {
            const button = fileTypeSelectDialogConfig.value?.buttons[index];
            if (!button) return;
            const paths = [...fileTypeSelectDialogPaths.value];
            closeFileTypeSelectDialog();
            void processDroppedFiles(paths, button.category);
        };

        const resetDragState = () => {
            dragDepth.value = 0;
            isDraggingFiles.value = false;
        };

        const isFileDragEvent = (event: DragEvent) => {
            if (!props.acceptFileDrop) {
                return false;
            }

            const types = Array.from(event.dataTransfer?.types || []);
            return types.includes("Files");
        };

        const handleDragEnter = (event: DragEvent) => {
            if (!isFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            dragDepth.value += 1;
            isDraggingFiles.value = true;
        };

        const handleDragOver = (event: DragEvent) => {
            if (!isFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            if (event.dataTransfer) {
                event.dataTransfer.dropEffect = "copy";
            }
            isDraggingFiles.value = true;
        };

        const handleDragLeave = (event: DragEvent) => {
            if (!isFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            dragDepth.value = Math.max(0, dragDepth.value - 1);
            if (dragDepth.value === 0) {
                isDraggingFiles.value = false;
            }
        };

        const handleDrop = async (event: DragEvent) => {
            if (!isFileDragEvent(event)) {
                return;
            }

            event.preventDefault();
            resetDragState();
        };

        watch(
            () => uploadFilesStore.pendingNativeDropVersion,
            async () => {
                const payload = uploadFilesStore.pendingNativeDrop;
                uploadFilesStore.clearPendingNativeDrop();
                resetDragState();

                if (!payload || !props.acceptFileDrop) {
                    return;
                }

                const workspaceEl = workspaceContentRef.value;
                if (!workspaceEl) {
                    return;
                }

                const rect = workspaceEl.getBoundingClientRect();
                const isInsideWorkspace =
                    payload.x >= rect.left &&
                    payload.x <= rect.right &&
                    payload.y >= rect.top &&
                    payload.y <= rect.bottom;

                if (!isInsideWorkspace) {
                    return;
                }

                const validatedPaths = await uploadFilesStore.validateIncomingPaths(payload.paths);
                if (validatedPaths.length === 0) {
                    return;
                }

                // 原生拖拽拿到的是整批路径，先在后端按统一格式规则过滤，再走前端状态约束和后续分类逻辑。
                const { acceptedPaths: droppedPaths, toastMessageKey } =
                    uploadFilesStore.resolveIncomingPaths(validatedPaths);
                if (toastMessageKey) {
                    uploadFilesStore.showConstraintToast(toastMessageKey);
                }
                if (droppedPaths.length === 0) {
                    return;
                }

                await handleFileDropAction(droppedPaths);
            },
        );

        // 过期文件对话框状态
        const showExpiredFilesDialog = ref(false);

        // 监听过期文件变化
        watch(
            () => uploadFilesStore.expiredFilesVersion,
            () => {
                const files = uploadFilesStore.expiredFiles;
                if (files.length > 0) {
                    showExpiredFilesDialog.value = true;
                }
            },
        );

        // 关闭过期文件对话框
        const closeExpiredFilesDialog = () => {
            showExpiredFilesDialog.value = false;
            uploadFilesStore.clearExpiredFiles();
        };

        // 确认过期文件对话框
        const confirmExpiredFilesDialog = () => {
            showExpiredFilesDialog.value = false;
            uploadFilesStore.confirmExpiredFiles();
        };

        const setupWindowResizeObserver = () => {
            const mainWindow = document.querySelector(".main-window") as HTMLElement;
            if (!mainWindow) return;

            // 初始化窗口宽度
            const initialWidth = mainWindow.offsetWidth;
            mainWindowStore.windowWidth = initialWidth;

            // 初始化侧边栏原始宽度
            if (mainWindowStore.sidebarOriginalWidth <= 0) {
                mainWindowStore.sidebarOriginalWidth =
                    mainWindowStore.prevSidebarWidth ||
                    mainWindowStore.sidebarWidth ||
                    mainWindowStore.defaultSidebarWidth;
            }

            // 监听窗口大小变化
            const resizeObserver = new ResizeObserver((entries) => {
                for (const entry of entries) {
                    mainWindowStore.handleWindowResize(entry.contentRect.width);
                }
            });
            resizeObserver.observe(mainWindow);

            // Cleanup
            onUnmounted(() => {
                resizeObserver.disconnect();
            });
        };

        onMounted(() => {
            setupWindowResizeObserver();
            window.addEventListener("drop", resetDragState);
            window.addEventListener("dragend", resetDragState);
        });

        onUnmounted(() => {
            window.removeEventListener("drop", resetDragState);
            window.removeEventListener("dragend", resetDragState);
        });

        const isHistoryConversation = computed(
            () => mainWindowStore.workspacePage === MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION,
        );

        return {
            extensionStore,
            isHistoryConversation,
            containerClass,
            showSplitter,
            handlePanelClose,
            isEnableAdvancedCssFeatures: computed(() => backend.isEnableAdvancedCssFeatures),
            isDraggingFiles,
            dropzoneTitle,
            dropzoneDescription,
            dropzoneUploadIcon,
            workspaceContentRef,
            fileTypeSelectDialogConfig,
            fileTypeSelectDialogPaths,
            closeFileTypeSelectDialog,
            handleFileTypeSelect,
            handleDragEnter,
            handleDragOver,
            handleDragLeave,
            handleDrop,
            showExpiredFilesDialog,
            closeExpiredFilesDialog,
            confirmExpiredFilesDialog,
            ...useTitleBarState(),
        };
    },
    render() {
        const defaultSlot = this.$slots.default ? this.$slots.default() : null;

        return (
            <div class={["workspace", this.extensionStore.panelFullscreen && "workspace--editor-fullscreen", this.isHistoryConversation && "workspace--history-conversation"]}>
                {/* 40px 标题栏——替代原有 padding-top，用于防止内容被窗口标题栏遮挡，同时在助手名滚出时显示名称 */}
                <div
                    class={["workspace__titlebar", this.titleBarScrolled && "workspace__titlebar--scrolled"]}
                    aria-hidden="true"
                >
                    <span
                        class="workspace__titlebar-text"
                        style={{ opacity: this.extensionStore.panelFullscreen ? 0 : this.titleBarOpacity }}
                    >
                        {this.assistantName}
                    </span>
                </div>
                <div
                    class={this.containerClass}
                    ref="workspaceContentRef"
                    onDragenter={this.handleDragEnter}
                    onDragover={this.handleDragOver}
                    onDragleave={this.handleDragLeave}
                    onDrop={this.handleDrop}
                >
                    {this.isDraggingFiles && (
                        <div
                            class={[
                                "workspace__dropzone",
                                this.isEnableAdvancedCssFeatures && "workspace__dropzone--advanced-css",
                            ]}
                            aria-hidden="true"
                        >
                            <div class="workspace__dropzone-content">
                                <div class="workspace__dropzone-icon">
                                    <SvgIcon
                                        icon={this.dropzoneUploadIcon}
                                        class="workspace__dropzone-icon-svg"
                                        size={[150, 150]}
                                    />
                                </div>
                                <div class="workspace__dropzone-text">{this.dropzoneTitle}</div>
                                <div class="workspace__dropzone-text-suffix">{this.dropzoneDescription}</div>
                            </div>
                        </div>
                    )}
                    {/* Main content area (chat) */}
                    {this.extensionStore.showChatArea && (
                        <div class="workspace__main">
                            {defaultSlot}
                            <Footer />
                        </div>
                    )}

                    {/* Splitter - only show when both areas are visible */}
                    {this.showSplitter && <Splitter disabled={true} />}

                    {/* Extension Panel */}
                    <ExtensionPanel visible={this.extensionStore.showExtensionPanel} onClose={this.handlePanelClose}>
                        {this.extensionStore.extensionContent?.()}
                    </ExtensionPanel>
                </div>
                <FileTypeSelectDialog
                    visible={!!this.fileTypeSelectDialogConfig}
                    fileCount={this.fileTypeSelectDialogPaths.length}
                    filePaths={this.fileTypeSelectDialogPaths}
                    buttons={this.fileTypeSelectDialogConfig?.buttons || []}
                    onClose={this.closeFileTypeSelectDialog}
                    onSelect={this.handleFileTypeSelect}
                />
                <ExpiredFilesDialog
                    visible={this.showExpiredFilesDialog}
                    files={useUploadFilesStore().expiredFiles}
                    onClose={this.closeExpiredFilesDialog}
                    onConfirm={this.confirmExpiredFilesDialog}
                />
            </div>
        );
    },
});
