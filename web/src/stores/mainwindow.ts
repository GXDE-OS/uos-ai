import { defineStore } from "pinia";

import { useBackendStore } from "@/stores/backend";
import { useReportChannelStore } from "@/stores/reportchannel";
import { MAIN_WINDOW_WORKSPACE_PAGES, type MainWindowWorkspacePage } from "@/types/mainwindow";
import { getMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { ReportEventType } from "@/types/report";

export const useMainWindowStore = defineStore("mainwindow", {
    state: () => ({
        workspacePage: MAIN_WINDOW_WORKSPACE_PAGES.CHAT as MainWindowWorkspacePage,
        workspacePageNavigationStack: [] as MainWindowWorkspacePage[],
        sidebarWidth: 200,
        prevSidebarWidth: 200,
        sidebarOriginalWidth: 200,
        isCollapsed: false,
        // 区分“用户主动收起”和“窗口过窄被动收起”，避免窗口回拉时误自动展开。
        isAutoCollapsedByResize: false,
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
        splitterWidth: 2,
        windowWidth: 0,
        isDarkMode: false, // 是否为暗黑模式
    }),

    getters: {
        isVisible: (state) => !state.isCollapsed || state.sidebarWidth > 0,
    },

    actions: {
        applyPersistedSidebarState(state?: { sidebarWidth?: number; sidebarExpanded?: boolean }) {
            const restoredWidth = this.normalizeSidebarWidth(state?.sidebarWidth || this.defaultSidebarWidth);
            const sidebarExpanded = state?.sidebarExpanded !== false;

            this.sidebarOriginalWidth = restoredWidth;
            this.prevSidebarWidth = restoredWidth;
            this.sidebarWidth = sidebarExpanded ? restoredWidth : this.collapsedWidth;
            this.isCollapsed = !sidebarExpanded;
            this.isAutoCollapsedByResize = false;
            this.updateCssVariable();
        },

        async loadPersistedSidebarState() {
            const backend = useBackendStore();
            if (!backend.windowChannel) {
                this.applyPersistedSidebarState();
                return;
            }

            try {
                const state = (await backend.requestWindow("getMainWindowSidebarState")) as
                    | {
                          sidebarWidth?: number;
                          sidebarExpanded?: boolean;
                      }
                    | undefined;
                this.applyPersistedSidebarState(state);
            } catch (error) {
                console.warn("[MainWindow] Failed to load persisted sidebar state:", error);
                this.applyPersistedSidebarState();
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
            const sidebarExpanded = !this.isCollapsed || this.isAutoCollapsedByResize;

            try {
                await backend.requestWindow("saveMainWindowSidebarState", preferredWidth, sidebarExpanded);
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

        ensureWindowWidthForSidebarExpand(targetSidebarWidth?: number) {
            const preferredSidebarWidth =
                targetSidebarWidth ?? this.prevSidebarWidth ?? this.sidebarOriginalWidth ?? this.defaultSidebarWidth;
            const normalizedSidebarWidth = this.normalizeSidebarWidth(preferredSidebarWidth);
            const requiredWindowWidth = normalizedSidebarWidth + this.minWorkspaceWidth + this.splitterWidth;

            if (this.windowWidth <= 0 || this.windowWidth >= requiredWindowWidth) {
                return;
            }

            // 当前窗口宽度不足时，先让 Qt 窗口补足空间，再展开侧边栏，
            // 避免只改前端布局导致工作区被异常挤压。
            useBackendStore().requestWindow("ensureMinimumWidth", requiredWindowWidth);
        },

        // Toggle sidebar collapse state
        toggleSidebarCollapse() {
            if (this.isCollapsed) {
                const restoreWidth = this.normalizeSidebarWidth(
                    this.prevSidebarWidth || this.sidebarOriginalWidth || this.defaultSidebarWidth,
                );
                this.ensureWindowWidthForSidebarExpand(restoreWidth);
                this.sidebarWidth = restoreWidth;
                if (this.sidebarWidth > 0) {
                    this.sidebarOriginalWidth = this.sidebarWidth;
                }
                this.isAutoCollapsedByResize = false;
            } else {
                this.prevSidebarWidth = this.sidebarWidth;
                document.documentElement.setAttribute("data-previous-sidebar-width", `${this.sidebarWidth}px`);
                this.sidebarWidth = this.collapsedWidth;
                this.isAutoCollapsedByResize = false;
            }
            this.isCollapsed = !this.isCollapsed;
            this.updateCssVariable();
            void this.persistSidebarState();
        },

        // Set sidebar width (used during drag)
        setSidebarWidth(width: number) {
            this.sidebarWidth = this.normalizeSidebarWidth(width);
            this.isCollapsed = false;
            this.isAutoCollapsedByResize = false;
            this.updateCssVariable();
        },

        // Start drag operation
        startDrag(e: MouseEvent) {
            if (this.isCollapsed) {
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
            const newWidth = Math.max(
                this.minSidebarWidth,
                Math.min(this.maxSidebarWidth, this.dragStartWidth + deltaX),
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
            if (this.sidebarWidth < 60 && !this.isCollapsed) {
                this.prevSidebarWidth = this.dragStartWidth;
                this.isCollapsed = true;
                this.isAutoCollapsedByResize = false;
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

            // 手动收起后保持收起状态，窗口缩放不自动干预。
            if (this.isCollapsed && !this.isAutoCollapsedByResize) {
                return;
            }

            const sidebarRestoreWidth = this.getSidebarRestoreWidth();
            const workspaceFirstThreshold = sidebarRestoreWidth + this.minWorkspaceWidth + this.splitterWidth;
            const sidebarMinThreshold = this.minSidebarWidth + this.minWorkspaceWidth + this.splitterWidth;

            if (width < oldWidth) {
                if (this.isAutoCollapsedByResize) {
                    return;
                }

                if (width >= workspaceFirstThreshold) {
                    // 第一阶段：只压工作区，侧边栏维持用户上次的目标宽度。
                    if (this.isCollapsed || this.sidebarWidth !== sidebarRestoreWidth) {
                        this.sidebarWidth = sidebarRestoreWidth;
                        this.isCollapsed = false;
                        this.isAutoCollapsedByResize = false;
                        this.updateCssVariable();
                    }
                    return;
                }

                if (width >= sidebarMinThreshold) {
                    // 第二阶段：工作区已到最小宽度，开始压缩侧边栏。
                    const nextSidebarWidth = this.normalizeSidebarWidth(
                        width - this.minWorkspaceWidth - this.splitterWidth,
                    );

                    if (this.isCollapsed || this.sidebarWidth !== nextSidebarWidth) {
                        this.sidebarWidth = nextSidebarWidth;
                        this.isCollapsed = false;
                        this.isAutoCollapsedByResize = false;
                        this.updateCssVariable();
                    }
                    return;
                }

                if (this.sidebarWidth > 0) {
                    this.prevSidebarWidth = this.sidebarWidth;
                }
                // 第三阶段：侧边栏也无法继续压缩时，直接收起，让空间重新让给工作区。
                this.sidebarWidth = this.collapsedWidth;
                this.isCollapsed = true;
                this.isAutoCollapsedByResize = true;
                this.updateCssVariable();
                return;
            }

            if (this.isCollapsed && this.isAutoCollapsedByResize) {
                if (width < sidebarMinThreshold) {
                    return;
                }

                // 窗口回拉时，先从“自动收起”恢复到可容纳的最小侧边栏宽度。
                const nextSidebarWidth = this.normalizeSidebarWidth(
                    Math.min(sidebarRestoreWidth, width - this.minWorkspaceWidth - this.splitterWidth),
                );

                this.sidebarWidth = nextSidebarWidth;
                this.isCollapsed = false;
                this.isAutoCollapsedByResize = false;
                this.updateCssVariable();
                return;
            }

            if (width < workspaceFirstThreshold) {
                // 侧边栏恢复完成前，优先把新增空间补给侧边栏，工作区维持最小宽度。
                const nextSidebarWidth = this.normalizeSidebarWidth(
                    Math.min(sidebarRestoreWidth, width - this.minWorkspaceWidth - this.splitterWidth),
                );

                if (nextSidebarWidth > this.sidebarWidth) {
                    this.sidebarWidth = nextSidebarWidth;
                    this.updateCssVariable();
                }
                return;
            }

            if (this.sidebarWidth !== sidebarRestoreWidth) {
                this.sidebarWidth = sidebarRestoreWidth;
                this.updateCssVariable();
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

        toggleHistoryConversationPage() {
            return this.toggleWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.HISTORY_CONVERSATION);
        },

        toggleDigitalHumanPage() {
            return this.toggleWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN);
        },
    },
});

// Bind methods for event listeners
const mainWindowStore = useMainWindowStore();
mainWindowStore.handleDrag = mainWindowStore.handleDrag.bind(mainWindowStore);
mainWindowStore.endDrag = mainWindowStore.endDrag.bind(mainWindowStore);
