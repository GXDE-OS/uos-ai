import { defineStore } from "pinia";

import { useBackendStore } from "@/stores/backend";
import { useExtensionPanelStore } from "@/stores/extensionPanel";
import { useReportChannelStore } from "@/stores/reportchannel";
import { MAIN_WINDOW_WORKSPACE_PAGES, type MainWindowWorkspacePage } from "@/types/mainwindow";
import { getMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { ReportEventType } from "@/types/report";

type PersistedSidebarState = {
    sidebarWidth?: number;
    sidebarExpanded?: boolean;
    groupCollapsedStates?: Record<string, boolean>;
};

export const useMainWindowStore = defineStore("mainwindow", {
    state: () => ({
        workspacePage: MAIN_WINDOW_WORKSPACE_PAGES.CHAT as MainWindowWorkspacePage,
        workspacePageNavigationStack: [] as MainWindowWorkspacePage[],
        sidebarWidth: 200,
        prevSidebarWidth: 200,
        sidebarOriginalWidth: 200,
        isSidebarCollapsed: false,
        // 区分“用户主动收起”和“窗口过窄被动收起”，避免窗口回拉时误自动展开。
        isSidebarAutoCollapsedByResize: false,
        isDragging: false,
        isResizing: false,
        dragStartX: 0,
        dragStartWidth: 0,
        resizeDebounceTimer: null as ReturnType<typeof setTimeout> | null,

        minSidebarWidth: 136,
        maxSidebarWidth: 320,
        collapsedWidth: 0,
        defaultSidebarWidth: 200,
        minWorkspaceWidth: 560,
        dualColumnWorkspaceMinWidth: 990,
        splitterWidth: 2,
        windowWidth: 0,
        isDarkMode: false, // 是否为暗黑模式

        // 侧边栏分组折叠状态
        sidebarGroupCollapsedStates: {} as Record<string, boolean>,

        // 新手引导弹窗状态
        newUserGuideDialogVisible: false,
        newUserGuideDialogPageIndex: 0,
    }),

    getters: {
        isVisible: (state) => !state.isSidebarCollapsed || state.sidebarWidth > 0,
    },

    actions: {
        applyPersistedSidebarState(state?: PersistedSidebarState) {
            const restoredWidth = this.normalizeSidebarWidth(state?.sidebarWidth || this.defaultSidebarWidth);
            const sidebarExpanded = state?.sidebarExpanded !== false;

            this.sidebarOriginalWidth = restoredWidth;
            this.prevSidebarWidth = restoredWidth;
            this.sidebarWidth = sidebarExpanded ? restoredWidth : this.collapsedWidth;
            this.isSidebarCollapsed = !sidebarExpanded;
            this.isSidebarAutoCollapsedByResize = false;
            this.updateCssVariable();
        },

        async loadPersistedSidebarState() {
            const backend = useBackendStore();
            if (!backend.windowChannel) {
                this.applyPersistedSidebarState();
                this.sidebarGroupCollapsedStates = {};
                return;
            }

            try {
                const state = (await backend.requestWindow("getMainWindowSidebarState")) as
                    | PersistedSidebarState
                    | undefined;
                this.applyPersistedSidebarState(state);
                this.sidebarGroupCollapsedStates = state?.groupCollapsedStates ?? {};
                if (state?.sidebarExpanded !== false) {
                    await this.ensureWindowWidthForSidebarExpand(this.getSidebarRestoreWidth(), {
                        allowUnknownWindowWidth: true,
                    });
                }
            } catch (error) {
                console.warn("[MainWindow] Failed to load persisted sidebar state:", error);
                this.applyPersistedSidebarState();
                this.sidebarGroupCollapsedStates = {};
                await this.ensureWindowWidthForSidebarExpand(this.getSidebarRestoreWidth(), {
                    allowUnknownWindowWidth: true,
                });
            }
        },

        async persistSidebarState() {
            const backend = useBackendStore();
            if (!backend.windowChannel) {
                return;
            }

            const preferredWidth = this.normalizeSidebarWidth(
                this.sidebarOriginalWidth || this.prevSidebarWidth || this.sidebarWidth || this.defaultSidebarWidth,
            );
            const sidebarExpanded = !this.isSidebarCollapsed || this.isSidebarAutoCollapsedByResize;
            const groupCollapsedStatesJson = JSON.stringify(this.sidebarGroupCollapsedStates);

            try {
                await backend.requestWindow("saveMainWindowSidebarState", preferredWidth, sidebarExpanded);
                await backend.requestWindow("saveMainWindowSidebarGroupCollapsedStates", groupCollapsedStatesJson);
            } catch (error) {
                console.warn("[MainWindow] Failed to persist sidebar state:", error);
            }
        },

        async runWorkspacePageEnter(page: MainWindowWorkspacePage, fromPage: MainWindowWorkspacePage | null) {
            try {
                await getMainWindowWorkspacePage(page)?.enter?.({
                    fromPage,
                    toPage: page,
                });
            } catch (error) {
                console.error(`[MainWindow] Failed to enter workspace page "${page}":`, error);
            }
        },

        async runWorkspacePageLeave(page: MainWindowWorkspacePage, toPage: MainWindowWorkspacePage) {
            try {
                await getMainWindowWorkspacePage(page)?.leave?.({
                    fromPage: page,
                    toPage,
                });
            } catch (error) {
                console.error(`[MainWindow] Failed to leave workspace page "${page}":`, error);
            }
        },

        async transitionWorkspacePage(
            page: MainWindowWorkspacePage,
            options?: {
                skipNavigationStackUpdate?: boolean;
            },
        ) {
            const currentPage = this.workspacePage;

            if (page === currentPage) {
                return this.runWorkspacePageEnter(page, currentPage);
            }

            await this.runWorkspacePageLeave(currentPage, page);

            if (!options?.skipNavigationStackUpdate) {
                const targetPageDefinition = getMainWindowWorkspacePage(page);

                if (targetPageDefinition?.backButton) {
                    this.workspacePageNavigationStack.push(currentPage);
                } else {
                    this.workspacePageNavigationStack = [];
                }
            }

            this.workspacePage = page;
            return this.runWorkspacePageEnter(page, currentPage);
        },

        setWorkspacePage(page: MainWindowWorkspacePage) {
            return this.transitionWorkspacePage(page);
        },

        openWorkspacePage(page: MainWindowWorkspacePage) {
            return this.setWorkspacePage(page);
        },

        toggleWorkspacePage(
            page: MainWindowWorkspacePage,
            fallbackPage: MainWindowWorkspacePage = MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
        ) {
            if (this.workspacePage === page) {
                if (getMainWindowWorkspacePage(page)?.backButton) {
                    return this.goBackWorkspacePage();
                }

                return this.openWorkspacePage(fallbackPage);
            }

            // 切换到数字人时，埋点数字人使用
            if (page === MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN) {
                useReportChannelStore().writeReportEvent([{ type: ReportEventType.DigitalChatPoint }]);
            }
            return this.openWorkspacePage(page);
        },

        goBackWorkspacePage() {
            const currentPageDefinition = getMainWindowWorkspacePage(this.workspacePage);
            const fallbackPage = currentPageDefinition?.backButton?.fallbackPage || MAIN_WINDOW_WORKSPACE_PAGES.CHAT;
            const previousPage = this.workspacePageNavigationStack.pop() || fallbackPage;

            return this.transitionWorkspacePage(previousPage, {
                skipNavigationStackUpdate: true,
            });
        },

        // Update CSS variable on document
        updateCssVariable() {
            document.documentElement.style.setProperty("--sidebar-width", `${this.sidebarWidth}px`);
        },

        normalizeSidebarWidth(width: number) {
            return Math.max(this.minSidebarWidth, Math.min(this.maxSidebarWidth, width));
        },

        getSidebarRestoreWidth() {
            return this.normalizeSidebarWidth(this.sidebarOriginalWidth || this.defaultSidebarWidth);
        },

        getWorkspaceMinWidthForCurrentLayout() {
            const extensionStore = useExtensionPanelStore();
            const isDualColumnVisible =
                extensionStore.showExtensionPanel && extensionStore.showChatArea && !extensionStore.panelFullscreen;

            return isDualColumnVisible ? this.dualColumnWorkspaceMinWidth : this.minWorkspaceWidth;
        },

        getSidebarMaxWidthForCurrentLayout() {
            const workspaceMinWidth = this.getWorkspaceMinWidthForCurrentLayout();
            if (this.windowWidth <= 0) {
                return this.maxSidebarWidth;
            }

            return Math.max(0, Math.min(this.maxSidebarWidth, this.windowWidth - workspaceMinWidth));
        },

        async ensureWindowWidthForSidebarExpand(
            targetSidebarWidth?: number,
            options?: { allowUnknownWindowWidth?: boolean },
        ) {
            const preferredSidebarWidth =
                targetSidebarWidth ?? this.prevSidebarWidth ?? this.sidebarOriginalWidth ?? this.defaultSidebarWidth;
            const normalizedSidebarWidth = this.normalizeSidebarWidth(preferredSidebarWidth);
            const workspaceMinWidth = this.getWorkspaceMinWidthForCurrentLayout();
            const requiredWindowWidth = normalizedSidebarWidth + workspaceMinWidth;
            const hasKnownWindowWidth = this.windowWidth > 0;

            if ((!hasKnownWindowWidth && !options?.allowUnknownWindowWidth) || this.windowWidth >= requiredWindowWidth) {
                return;
            }

            // 当前窗口宽度不足时，先让 Qt 窗口补足空间，再展开侧边栏，
            // 避免只改前端布局导致工作区被异常挤压。
            const backend = useBackendStore();
            if (!backend.windowChannel) {
                return;
            }

            try {
                await backend.requestWindow("ensureMinimumWidth", requiredWindowWidth);
            } catch (error) {
                console.warn("[MainWindow] Failed to ensure window width for sidebar expand:", error);
            }
        },

        // Toggle sidebar collapse state
        toggleSidebarCollapse() {
            if (this.isSidebarCollapsed) {
                const restoreWidth = this.normalizeSidebarWidth(
                    this.prevSidebarWidth || this.sidebarOriginalWidth || this.defaultSidebarWidth,
                );
                void this.ensureWindowWidthForSidebarExpand(restoreWidth);
                this.sidebarWidth = restoreWidth;
                if (this.sidebarWidth > 0) {
                    this.sidebarOriginalWidth = this.sidebarWidth;
                }
                this.isSidebarAutoCollapsedByResize = false;
            } else {
                this.prevSidebarWidth = this.sidebarWidth;
                document.documentElement.setAttribute("data-previous-sidebar-width", `${this.sidebarWidth}px`);
                this.sidebarWidth = this.collapsedWidth;
                this.isSidebarAutoCollapsedByResize = false;
            }
            this.isSidebarCollapsed = !this.isSidebarCollapsed;
            this.updateCssVariable();
            void this.persistSidebarState();
        },

        // Set sidebar width (used during drag)
        setSidebarWidth(width: number) {
            const dynamicMaxSidebarWidth = this.getSidebarMaxWidthForCurrentLayout();
            if (dynamicMaxSidebarWidth < this.minSidebarWidth) {
                this.prevSidebarWidth = this.sidebarWidth;
                this.sidebarWidth = this.collapsedWidth;
                this.isSidebarCollapsed = true;
                this.isSidebarAutoCollapsedByResize = true;
                this.updateCssVariable();
                return;
            }

            this.sidebarWidth = Math.max(this.minSidebarWidth, Math.min(dynamicMaxSidebarWidth, width));
            this.isSidebarCollapsed = false;
            this.isSidebarAutoCollapsedByResize = false;
            this.updateCssVariable();
        },

        // Start drag operation
        startDrag(e: MouseEvent) {
            if (this.isSidebarCollapsed) {
                this.toggleSidebarCollapse();
                return false;
            }
            this.isDragging = true;
            this.dragStartX = e.clientX;
            this.dragStartWidth = this.sidebarWidth;

            // Disable transition during drag
            document.querySelector(".window-sidebar")?.classList.add("is-dragging");
            document.addEventListener("mousemove", this.handleDrag);
            document.addEventListener("mouseup", this.endDrag);
            e.preventDefault();
            return true;
        },

        // Handle drag movement
        handleDrag(e: MouseEvent) {
            if (!this.isDragging) return;

            const deltaX = e.clientX - this.dragStartX;
            const dynamicMaxSidebarWidth = this.getSidebarMaxWidthForCurrentLayout();
            if (dynamicMaxSidebarWidth < this.minSidebarWidth) {
                return;
            }

            const newWidth = Math.max(
                this.minSidebarWidth,
                Math.min(dynamicMaxSidebarWidth, this.dragStartWidth + deltaX),
            );

            this.sidebarWidth = newWidth;
            this.updateCssVariable();
        },

        // End drag operation
        endDrag() {
            if (!this.isDragging) return;

            this.isDragging = false;
            document.querySelector(".window-sidebar")?.classList.remove("is-dragging");
            document.removeEventListener("mousemove", this.handleDrag);
            document.removeEventListener("mouseup", this.endDrag);

            // Auto collapse if width is very small
            if (this.sidebarWidth < 60 && !this.isSidebarCollapsed) {
                this.prevSidebarWidth = this.dragStartWidth;
                this.isSidebarCollapsed = true;
                this.isSidebarAutoCollapsedByResize = false;
                this.sidebarWidth = this.collapsedWidth;
                this.updateCssVariable();
            } else {
                // 更新侧边栏原始宽度（用于窗口拉伸时恢复）
                this.sidebarOriginalWidth = this.sidebarWidth;
            }

            void this.persistSidebarState();
        },

        // Handle window resize
        handleWindowResize(width: number) {
            const oldWidth = this.windowWidth;
            this.windowWidth = width;
            const backend = useBackendStore();
            const extensionStore = useExtensionPanelStore();

            // 全局窗口最小宽度约束。
            if (width < this.minWorkspaceWidth) {
                void backend.requestWindow("ensureMinimumWidth", this.minWorkspaceWidth);
            }

            // 添加 is-resizing 类，禁用过渡动画
            const sidebar = document.querySelector(".window-sidebar") as HTMLElement;
            if (sidebar && !this.isResizing) {
                sidebar.classList.add("is-resizing");
                this.isResizing = true;
            }

            // 防抖移除 is-resizing 类
            if (this.resizeDebounceTimer) {
                clearTimeout(this.resizeDebounceTimer);
            }
            this.resizeDebounceTimer = setTimeout(() => {
                if (sidebar) {
                    sidebar.classList.remove("is-resizing");
                }
                this.isResizing = false;
            }, 100);

            if (width === oldWidth) {
                return;
            }

            const hasExtensionPanel = extensionStore.showExtensionPanel;
            const sidebarRestoreWidth = this.getSidebarRestoreWidth();
            const currentSidebarRestoreThreshold = this.minSidebarWidth + this.getWorkspaceMinWidthForCurrentLayout();

            if (width < oldWidth) {
                // 缩小时优先压缩侧边栏，先保证工作区达到当前布局的最小宽度。
                if (!this.isSidebarCollapsed) {
                    const workspaceMinWidth = this.getWorkspaceMinWidthForCurrentLayout();
                    const maxSidebarWidthFromWindow = width - workspaceMinWidth;

                    if (maxSidebarWidthFromWindow >= this.minSidebarWidth) {
                        const nextSidebarWidth = this.normalizeSidebarWidth(maxSidebarWidthFromWindow);

                        if (nextSidebarWidth < this.sidebarWidth) {
                            this.sidebarWidth = nextSidebarWidth;
                            this.updateCssVariable();
                        }
                    } else {
                        if (this.sidebarWidth > 0) {
                            this.prevSidebarWidth = this.sidebarWidth;
                        }
                        this.sidebarWidth = this.collapsedWidth;
                        this.isSidebarCollapsed = true;
                        this.isSidebarAutoCollapsedByResize = true;
                        this.updateCssVariable();
                    }
                }

                // 侧边栏已隐藏后，如果双栏工作区不足 900，则自动切到扩展区全屏并隐藏聊天区。
                if (
                    hasExtensionPanel &&
                    extensionStore.showChatArea &&
                    !extensionStore.panelFullscreen &&
                    width < this.dualColumnWorkspaceMinWidth
                ) {
                    extensionStore.applyResizeFullscreenMode(true);
                }

                return;
            }

            // 放大窗口时，先恢复聊天区（如果它是被 resize 自动隐藏的）。
            if (extensionStore.isChatAreaAutoHiddenByResize && extensionStore.isPanelFullscreenAutoByResize) {
                if (width < this.dualColumnWorkspaceMinWidth) {
                    return;
                }

                extensionStore.applyResizeFullscreenMode(false);
                return;
            }

            // 聊天区恢复后，再恢复被自动收起的侧边栏。
            if (this.isSidebarCollapsed && this.isSidebarAutoCollapsedByResize) {
                if (width < currentSidebarRestoreThreshold) {
                    return;
                }

                const workspaceMinWidth = this.getWorkspaceMinWidthForCurrentLayout();
                const nextSidebarWidth = this.normalizeSidebarWidth(
                    Math.min(sidebarRestoreWidth, width - workspaceMinWidth),
                );

                this.sidebarWidth = nextSidebarWidth;
                this.isSidebarCollapsed = false;
                this.isSidebarAutoCollapsedByResize = false;
                this.updateCssVariable();
                return;
            }

            // 侧边栏可见时，随窗口放大逐步恢复到用户期望宽度。
            if (!this.isSidebarCollapsed) {
                const workspaceMinWidth = this.getWorkspaceMinWidthForCurrentLayout();
                const maxSidebarWidthFromWindow = width - workspaceMinWidth;

                if (maxSidebarWidthFromWindow >= this.minSidebarWidth) {
                    const nextSidebarWidth = this.normalizeSidebarWidth(
                        Math.min(sidebarRestoreWidth, maxSidebarWidthFromWindow),
                    );

                    if (nextSidebarWidth > this.sidebarWidth) {
                        this.sidebarWidth = nextSidebarWidth;
                        this.updateCssVariable();
                    }
                }
            }
        },

        openMcpServicesPage() {
            return this.openWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.MCP_SERVICES);
        },

        openSkillsPage() {
            return this.openWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.SKILLS);
        },

        openChatPage() {
            return this.openWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.CHAT);
        },

        openHistoryConversationPage() {
            return this.openWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION);
        },

        openNewUserGuideDialog() {
            this.newUserGuideDialogPageIndex = 0;
            this.newUserGuideDialogVisible = true;
        },

        closeNewUserGuideDialog() {
            this.newUserGuideDialogVisible = false;
            this.newUserGuideDialogPageIndex = 0;
        },

        async openStartupNewUserGuideDialogIfNeeded() {
            const backend = useBackendStore();
            if (!backend.windowChannel) {
                return;
            }

            try {
                const shouldShow = await backend.requestWindow("shouldShowNewUserGuideOnStartup");
                if (!shouldShow) {
                    return;
                }

                this.openNewUserGuideDialog();
                await backend.requestWindow("recordNewUserGuideShown");
            } catch (error) {
                console.warn("[MainWindow] Failed to check startup new user guide state:", error);
            }
        },

        goToPreviousNewUserGuideDialogPage() {
            this.newUserGuideDialogPageIndex = Math.max(0, this.newUserGuideDialogPageIndex - 1);
        },

        goToNextNewUserGuideDialogPage(totalPages: number) {
            if (totalPages <= 0) {
                return;
            }

            this.newUserGuideDialogPageIndex = Math.min(totalPages - 1, this.newUserGuideDialogPageIndex + 1);
        },

        toggleHistoryConversationPage() {
            return this.toggleWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION);
        },

        toggleDigitalHumanPage() {
            return this.toggleWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN);
        },

        // 设置侧边栏分组折叠状态
        setSidebarGroupCollapsed(groupId: string, collapsed: boolean) {
            this.sidebarGroupCollapsedStates = {
                ...this.sidebarGroupCollapsedStates,
                [groupId]: collapsed,
            };
            void this.persistSidebarState();
        },
    },
});

// Bind methods for event listeners
const mainWindowStore = useMainWindowStore();
mainWindowStore.handleDrag = mainWindowStore.handleDrag.bind(mainWindowStore);
mainWindowStore.endDrag = mainWindowStore.endDrag.bind(mainWindowStore);
