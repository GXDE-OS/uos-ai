import {
    defineComponent,
    ref,
    computed,
    watch,
    inject,
    h,
    onMounted,
    onBeforeUnmount,
    nextTick,
    Teleport,
    type PropType,
} from "vue";
import type { ArticleReference } from "@/types/conversation";
import MarkdownEditorLinkDialog from "./MarkdownEditorLinkDialog";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";

/** 目录标题项 */
interface TocHeading {
    level: number;
    text: string;
    /** ProseMirror doc 中该节点起始位置 */
    pos: number;
}
interface LinkHoverRect {
    top: number;
    left: number;
    width: number;
    height: number;
}
interface MouseClientPoint {
    x: number;
    y: number;
}
import { Editor, rootCtx, defaultValueCtx, commandsCtx, editorViewCtx } from "@milkdown/kit/core";
import {
    commonmark,
    toggleStrongCommand,
    toggleEmphasisCommand,
    wrapInHeadingCommand,
    wrapInBlockquoteCommand,
    wrapInBulletListCommand,
    wrapInOrderedListCommand,
    turnIntoTextCommand,
} from "@milkdown/kit/preset/commonmark";
import { liftListItem } from "@milkdown/kit/prose/schema-list";
import { gfm, toggleStrikethroughCommand } from "@milkdown/kit/preset/gfm";
import { listener, listenerCtx } from "@milkdown/kit/plugin/listener";
import { clipboard } from "@milkdown/kit/plugin/clipboard";
import { history, undoCommand, redoCommand } from "@milkdown/kit/plugin/history";
import { indent } from "@milkdown/kit/plugin/indent";
import { mathPlugins, normalizeMathDelimiters } from "./milkdownMathPlugins";
import { footnotePlugins } from "./milkdownFootnotePlugins";
import { Milkdown, useEditor, MilkdownProvider } from "@milkdown/vue";
import { replaceAll, insert, callCommand } from "@milkdown/kit/utils";
import { undoDepth, redoDepth } from "prosemirror-history";
import { Selection } from "@milkdown/kit/prose/state";
import "@milkdown/crepe/theme/common/style.css";
import "@milkdown/crepe/theme/frame.css";
import "katex/dist/katex.min.css";
import { EXTENSION_PANEL_KEY } from "@/types/extension-panel";
import { useBackendStore, useExtensionPanelStore, useConversationManagerStore, useNotifyStore } from "@/stores";
import { useMilkdownPrintExport } from "./composables/useMilkdownPrintExport";
import "@/assets/styles/components/MarkdownEditor.css";
import MarkdownEditorToolbar from "./MarkdownEditorToolbar";
import ScrollBar from "@/components/ScrollBar";
import Menu from "@/components/menu/Menu";
import type { MenuItem } from "@/types/menu";
import { CopyDataType } from "@/types/message";

/**
 * Markdown 编辑器内部组件（需要 MilkdownProvider 包裹）
 */
const MarkdownEditorInner = defineComponent({
    name: "MilkdownEditorInner",
    props: {
        modelValue: {
            type: String,
            required: true,
        },
        onEditorReady: {
            type: Function as PropType<(editor: any) => void>,
            required: true,
        },
    },
    emits: ["update:modelValue"],
    setup(props, { emit }) {
        // 追踪编辑器自身最后一次向外 emit 的值，防止 store 回显触发 replaceAll 导致焦点丢失
        const lastEmittedValue = ref(props.modelValue);

        const { get } = useEditor((root) =>
            Editor.make()
                .config((ctx) => {
                    ctx.set(rootCtx, root);
                    ctx.set(defaultValueCtx, normalizeMathDelimiters(props.modelValue));
                    ctx.get(listenerCtx).markdownUpdated((ctx, markdown) => {
                        lastEmittedValue.value = markdown;
                        emit("update:modelValue", markdown);
                    });
                })
                .use(commonmark)
                .use(gfm)
                .use(history)
                .use(indent)
                .use(listener)
                .use(clipboard)
                .use(mathPlugins)
                .use(footnotePlugins),
        );

        // 编辑器创建完成后，通知父组件
        watch(
            () => get(),
            (editor) => {
                if (editor) {
                    props.onEditorReady(editor);
                }
            },
            { immediate: true },
        );

        watch(
            () => props.modelValue,
            (newValue, oldValue) => {
                const editor = get();
                if (editor && newValue !== oldValue) {
                    // 若变化来自编辑器自身的 emit 回显（如 auto-save 写入 store 后 prop 反弹），
                    // 则跳过 replaceAll，避免重置编辑器视图并丢失焦点
                    if (newValue === lastEmittedValue.value) {
                        return;
                    }
                    editor.action(replaceAll(normalizeMathDelimiters(newValue)));
                }
            },
        );

        return () => h(Milkdown);
    },
});

/**
 * Markdown 编辑器组件
 *
 * 用于扩展面板的 Markdown 编辑器
 */
export default defineComponent({
    name: "MarkdownEditor",

    props: {
        /** Markdown 内容 */
        content: {
            type: String,
            default: "",
        },
        /** 文章 ID（workspace 中的 article_id） */
        articleId: {
            type: String,
            default: "",
        },
        /** 文档标题 */
        title: {
            type: String,
            default: "",
        },
        /** 会话 ID */
        conversationId: {
            type: String,
            default: "",
        },
        /** 隐藏全屏按钮（用于自动全屏模式） */
        hideFullscreenButton: {
            type: Boolean,
            default: false,
        },
        /** 文章引用列表 */
        references: {
            type: Array as PropType<ArticleReference[]>,
            default: () => [],
        },
        /** 初始更新时间（ISO 字符串），用于工具栏显示 */
        updatedAt: {
            type: String,
            default: "",
        },
    },

    emits: {
        /** 内容变化事件 */
        "update:content": (value: string) => typeof value === "string",
    },

    setup(props, { emit }) {
        const extensionPanelAPI = inject(EXTENSION_PANEL_KEY);
        const extensionStore = useExtensionPanelStore();
        const backendStore = useBackendStore();
        const conversationManagerStore = useConversationManagerStore();
        const notifyStore = useNotifyStore();
        const editorInstance = ref<any>(null);
        const printInProgress = ref(false);
        const { exportPrintableHtml } = useMilkdownPrintExport();

        // ─── 宽度检测：ResizeObserver 替代 CSS @container query（Qt5 WebEngine 不支持 @container） ───
        const layoutEl = ref<HTMLElement | null>(null);
        const isWide = ref(false);
        let resizeObserver: ResizeObserver | null = null;
        // rAF 节流句柄：同一帧内的多次 ResizeObserver 回调合并为一次处理
        let resizeRafId: number | null = null;
        onMounted(() => {
            if (layoutEl.value) {
                // 缓存上次写入 DOM 的整数像素值，相同时跳过 setProperty（避免无谓 style recalculation）
                let lastPaddingX = -1;
                let lastTooltipMaxWidth = -1;

                resizeObserver = new ResizeObserver((entries) => {
                    // 同帧已有待处理的 rAF，直接返回
                    if (resizeRafId !== null) return;
                    resizeRafId = requestAnimationFrame(() => {
                        resizeRafId = null;
                        const width = entries[entries.length - 1]?.contentRect.width ?? 0;

                        // 仅在布尔值真正改变时触发 Vue 响应式更新，避免每帧 re-render
                        const newIsWide = width >= 620;
                        if (newIsWide !== isWide.value) {
                            // 宽度变窄时自动关闭目录嵌入显示
                            if (isWide.value && !newIsWide && tocOpen.value) {
                                tocOpen.value = false;
                            }
                            isWide.value = newIsWide;
                        }

                        // Qt5 WebEngine 不可靠支持 clamp()，改为 JS 计算等价值：
                        // clamp(16px, 10%, 96px)
                        const editorPaddingX = Math.round(Math.max(16, Math.min(96, width * 0.1)));
                        if (editorPaddingX !== lastPaddingX) {
                            lastPaddingX = editorPaddingX;
                            layoutEl.value?.style.setProperty("--mde-editor-padding-x", `${editorPaddingX}px`);
                        }

                        // 同步更新 CSS 变量，供所有 tooltip 使用（不超过容器宽度 2/3）
                        const tooltipMaxWidth = Math.floor((width * 2) / 3);
                        if (tooltipMaxWidth !== lastTooltipMaxWidth) {
                            lastTooltipMaxWidth = tooltipMaxWidth;
                            document.documentElement.style.setProperty(
                                "--mde-tooltip-max-width",
                                `${tooltipMaxWidth}px`,
                            );
                        }
                    });
                });
                resizeObserver.observe(layoutEl.value);
            }
        });

        // ─── TOC 状态 ───
        const tocOpen = ref(false);
        const tocHeadings = ref<TocHeading[]>([]);
        // 悬浮目录相关状态
        const tocHoverActive = ref(false);
        const tocHoverTimer = ref<ReturnType<typeof setTimeout> | null>(null);

        // ─── 悬浮目录事件处理（窄屏 hover 触发） ───
        const handleTocToggleMouseEnter = () => {
            // 窄屏模式才启用 hover
            if (isWide.value) return;

            if (tocHoverTimer.value) {
                clearTimeout(tocHoverTimer.value);
                tocHoverTimer.value = null;
            }
            tocHoverActive.value = true;
        };

        const handleTocMouseLeave = () => {
            // 窄屏模式才启用 hover 关闭
            if (isWide.value) return;

            // 延迟关闭，避免用户在按钮和目录间移动时闪烁
            tocHoverTimer.value = setTimeout(() => {
                tocHoverActive.value = false;
            }, 150);
        };

        const handleTocMouseEnter = () => {
            // 鼠标进入目录区域，取消关闭定时器
            if (tocHoverTimer.value) {
                clearTimeout(tocHoverTimer.value);
                tocHoverTimer.value = null;
            }
        };

        // ─── 参考资料折叠状态 ───
        const refsOpen = ref(true);
        const highlightedRefIndex = ref<string | null>(null);
        const refsHighlightTimer = ref<ReturnType<typeof setTimeout> | null>(null);
        const bodyScrollRef = ref<any>(null);
        const refsHeaderEl = ref<HTMLElement | null>(null);
        const refsListEl = ref<HTMLElement | null>(null);

        const setBodyScrollRef = (instance: any) => {
            bodyScrollRef.value = instance;
        };

        const setRefsHeaderEl = (el: HTMLElement | null) => {
            refsHeaderEl.value = el;
        };

        const setRefsListEl = (el: HTMLElement | null) => {
            refsListEl.value = el;
        };

        const clearReferenceHighlight = () => {
            if (refsHighlightTimer.value) {
                clearTimeout(refsHighlightTimer.value);
                refsHighlightTimer.value = null;
            }
            highlightedRefIndex.value = null;
        };

        const triggerReferenceHighlight = (referenceIndex: string) => {
            if (refsHighlightTimer.value) {
                clearTimeout(refsHighlightTimer.value);
                refsHighlightTimer.value = null;
            }

            const applyHighlight = () => {
                highlightedRefIndex.value = referenceIndex;
                refsHighlightTimer.value = window.setTimeout(() => {
                    highlightedRefIndex.value = null;
                    refsHighlightTimer.value = null;
                }, 1000);
            };

            // 允许重复点击同一条目时重播闪烁动画
            if (highlightedRefIndex.value === referenceIndex) {
                highlightedRefIndex.value = null;
                requestAnimationFrame(applyHighlight);
                return;
            }

            applyHighlight();
        };

        const getBodyScrollContainer = (): HTMLElement | null => {
            const containerRef = bodyScrollRef.value?.scrollContainerRef;
            if (!containerRef) {
                return null;
            }

            if (containerRef instanceof HTMLElement) {
                return containerRef;
            }

            return (containerRef.value as HTMLElement | null) ?? null;
        };

        const animateBodyScrollTo = (targetScrollTop: number, duration = 220) => {
            const container = getBodyScrollContainer();
            if (!container) {
                return;
            }

            const startScrollTop = container.scrollTop;
            // 不在动画起点提前按 maxScroll 钳制，避免展开动画期间 maxScroll 增长导致目标丢失
            const target = Math.max(0, targetScrollTop);
            const distance = target - startScrollTop;

            if (Math.abs(distance) < 1) {
                return;
            }

            const easeOutCubic = (t: number) => 1 - Math.pow(1 - t, 3);
            const startTime = performance.now();

            const step = (now: number) => {
                const progress = Math.min(1, (now - startTime) / duration);
                const next = startScrollTop + distance * easeOutCubic(progress);
                if (typeof bodyScrollRef.value?.setScrollPosition === "function") {
                    bodyScrollRef.value.setScrollPosition(next);
                } else {
                    container.scrollTop = next;
                }

                if (progress < 1) {
                    requestAnimationFrame(step);
                } else if (typeof bodyScrollRef.value?.setScrollPosition === "function") {
                    // 结束时再对齐一次最终目标，确保展开动画完成后也能精确到位
                    bodyScrollRef.value.setScrollPosition(target);
                }
            };

            requestAnimationFrame(step);
        };

        const adjustRefsPositionAfterExpand = () => {
            const container = getBodyScrollContainer();
            const header = refsHeaderEl.value;
            const list = refsListEl.value;
            if (!container || !header || !list) {
                return;
            }

            const containerRect = container.getBoundingClientRect();
            const headerRect = header.getBoundingClientRect();
            const currentScrollTop = container.scrollTop;
            const viewportHeight = container.clientHeight;

            const headerTop = currentScrollTop + (headerRect.top - containerRect.top);
            const openPanelHeight = header.offsetHeight + list.scrollHeight;

            let targetScrollTop = currentScrollTop;
            if (openPanelHeight > viewportHeight) {
                // 引用区域很高：将 title 顶到可视区顶部
                targetScrollTop = headerTop;
            } else {
                // 引用区域不高：保证最后一条引用可见
                const panelBottom = headerTop + openPanelHeight + 24;
                const viewportBottom = currentScrollTop + viewportHeight;
                if (panelBottom > viewportBottom) {
                    targetScrollTop = panelBottom - viewportHeight;
                }
            }

            animateBodyScrollTo(targetScrollTop);
        };

        const findReferenceItemEl = (referenceIndex: string): HTMLElement | null => {
            const list = refsListEl.value;
            if (!list) {
                return null;
            }

            const items = Array.from(list.querySelectorAll<HTMLElement>(".mde-refs__item[data-reference-index]"));
            return items.find((item) => item.dataset.referenceIndex === referenceIndex) ?? null;
        };

        const scrollToReferenceItem = (referenceIndex: string) => {
            const container = getBodyScrollContainer();
            if (!container) {
                return;
            }

            const item = findReferenceItemEl(referenceIndex);
            if (!item) {
                adjustRefsPositionAfterExpand();
                return;
            }

            const containerRect = container.getBoundingClientRect();
            const itemRect = item.getBoundingClientRect();
            const currentScrollTop = container.scrollTop;
            const viewportHeight = container.clientHeight;

            const itemTop = currentScrollTop + (itemRect.top - containerRect.top);
            const itemBottom = itemTop + item.offsetHeight;
            const viewportTop = currentScrollTop;
            const viewportBottom = viewportTop + viewportHeight;

            const edgePadding = 20;
            let targetScrollTop = currentScrollTop;
            if (itemTop < viewportTop + edgePadding) {
                targetScrollTop = itemTop - edgePadding;
            } else if (itemBottom > viewportBottom - edgePadding) {
                targetScrollTop = itemBottom - viewportHeight + edgePadding;
            }

            animateBodyScrollTo(targetScrollTop);
        };

        const resolveReferenceIndexFromLabel = (label: string): string | null => {
            const normalizedLabel = label.trim();
            if (!normalizedLabel) {
                return null;
            }

            const exactMatch = props.references.find((reference) => String(reference.index) === normalizedLabel);
            if (exactMatch) {
                return String(exactMatch.index);
            }

            const numericLabel = Number.parseInt(normalizedLabel, 10);
            if (!Number.isNaN(numericLabel)) {
                const numericMatch = props.references.find((reference) => Number(reference.index) === numericLabel);
                if (numericMatch) {
                    return String(numericMatch.index);
                }
            }

            return null;
        };

        const jumpToReferenceByIndex = (referenceIndex: string) => {
            const doJump = () => {
                scrollToReferenceItem(referenceIndex);
                triggerReferenceHighlight(referenceIndex);
            };

            if (!refsOpen.value) {
                refsOpen.value = true;
                nextTick(() => {
                    requestAnimationFrame(() => {
                        adjustRefsPositionAfterExpand();
                        doJump();
                    });
                    // 等展开动画结束后再校准一次滚动位置
                    window.setTimeout(() => {
                        adjustRefsPositionAfterExpand();
                        doJump();
                    }, 260);
                });
                return;
            }

            nextTick(() => {
                requestAnimationFrame(doJump);
            });
        };

        const handleRefsToggle = () => {
            const willOpen = !refsOpen.value;
            refsOpen.value = willOpen;

            // 展开后根据引用区高度智能定位滚动位置
            if (willOpen) {
                nextTick(() => {
                    requestAnimationFrame(() => {
                        adjustRefsPositionAfterExpand();
                    });
                    // 引用区展开动画结束后再校准一次，保证最终位置符合规则
                    window.setTimeout(() => {
                        adjustRefsPositionAfterExpand();
                    }, 260);
                });
            }
        };

        // Auto-save state
        const saveTimer = ref<ReturnType<typeof setTimeout> | null>(null);
        const isSaving = ref(false);
        const isClosing = ref(false);
        // Track current editor content (updated on every change)
        const currentEditorContent = ref(props.content || "");
        // Initialize with current content to avoid false "unsaved" state
        const lastSavedContent = ref(props.content || "");
        const AUTO_SAVE_DELAY = 1000; // 1 second

        // 工具栏状态
        const isFullscreen = computed(() => extensionStore.panelFullscreen);
        // Initialize modifiedTime with the updatedAt prop from backend (or current time for new content)
        const modifiedTime = ref<string>(props.updatedAt || (props.content ? "" : ""));
        const activeStates = ref<Record<string, boolean>>({});

        // ─── 内容是否为空（用于禁用导出）───
        const isContentEmpty = computed(() => {
            // 去除所有空格、换行、制表符等空白字符后判断是否为空
            const content = currentEditorContent.value || "";
            return content.trim().length === 0;
        });

        // ─── 链接对话框状态 ───
        const linkDialogVisible = ref(false);
        const linkDialogInitialText = ref("");
        const linkHoverHref = ref("");
        const linkHoverVisible = ref(false);
        const linkHoverRect = ref<LinkHoverRect>({ top: 0, left: 0, width: 1, height: 1 });
        const hoveredLinkElRef = ref<HTMLAnchorElement | null>(null);
        const linkHoverMouseClientRef = ref<MouseClientPoint | null>(null);
        const linkHoverRafRef = ref<number | null>(null);
        const linkHoverShowTimerRef = ref<ReturnType<typeof setTimeout> | null>(null);
        const LINK_TOOLTIP_LINE_OFFSET = 8;
        // ─── 链接对话框处理函数 ───
        const handleLinkDialogCancel = () => {
            linkDialogVisible.value = false;
        };

        const handleLinkDialogConfirm = (payload: { text: string; url: string }) => {
            const editor = editorInstance.value;
            if (!editor) return;

            const { text, url } = payload;

            editor.action((ctx: any) => {
                const view = ctx.get(editorViewCtx);
                if (!view) return;

                const { schema } = view.state;
                const linkMark = schema.marks.link.create({ href: url });

                // 使用当前选区而非缓存的选区，避免 Race condition
                const currentSelection = view.state.selection;

                if (!currentSelection.empty) {
                    // 有选区：将现有文本转换为链接
                    const tr = view.state.tr;
                    tr.addMark(currentSelection.from, currentSelection.to, linkMark);
                    view.dispatch(tr);
                } else {
                    // 无选区：在光标处插入链接文本
                    const finalText = text || url;
                    const tr = view.state.tr;
                    const pos = currentSelection.from;
                    tr.insertText(finalText, pos, pos);
                    tr.addMark(pos, pos + finalText.length, linkMark);
                    view.dispatch(tr);
                }

                view.focus();
            });

            linkDialogVisible.value = false;
        };

        // 存储链接点击处理函数引用，用于清理
        const linkClickHandlerRef = ref<((event: MouseEvent) => void) | null>(null);
        // 存储正文脚注点击处理函数引用，用于清理
        const footnoteClickHandlerRef = ref<((event: MouseEvent) => void) | null>(null);
        // 存储链接 hover 处理函数引用，用于清理
        const linkHoverMoveHandlerRef = ref<((event: MouseEvent) => void) | null>(null);
        const linkHoverLeaveHandlerRef = ref<((event: MouseEvent) => void) | null>(null);
        const linkHoverWindowUpdateHandlerRef = ref<(() => void) | null>(null);
        // 存储键盘事件处理函数引用，用于清理
        const keydownHandlerRef = ref<((event: KeyboardEvent) => void) | null>(null);
        // 存储标题 Backspace 拦截处理函数引用，用于清理
        const headingBackspaceHandlerRef = ref<((event: KeyboardEvent) => void) | null>(null);
        // 存储右键菜单处理函数引用，用于清理
        const contextmenuHandlerRef = ref<((event: MouseEvent) => void) | null>(null);

        // 右键菜单相关状态
        const isContextMenuVisible = ref(false);
        const contextMenuItems = ref<MenuItem[]>([]);
        const contextMenuPosition = ref<{ x: number; y: number } | null>(null);

        // ─── 右键菜单：文字选中复制 ───
        const handleEditorContextMenu = async (event: MouseEvent) => {
            const selection = window.getSelection();
            const hasSelection = selection && !selection.isCollapsed && selection.toString().trim().length > 0;

            if (!hasSelection) return;

            event.preventDefault();
            contextMenuPosition.value = { x: event.clientX, y: event.clientY };
            contextMenuItems.value = [
                { type: "item", id: "copy-selection", label: backendStore.translate("Copy") },
            ];
            await nextTick();
            isContextMenuVisible.value = true;
        };

        const handleEditorMenuSelect = async (menuItem: MenuItem) => {
            if (menuItem.type !== "item" || menuItem.id !== "copy-selection") return;

            const selection = window.getSelection();
            if (!selection || selection.isCollapsed) return;

            try {
                const success = document.execCommand("copy");
                if (success) return;
            } catch (error) {
                console.error("execCommand copy failed:", error);
            }

            try {
                backendStore.requestSystem("copyToClipboard", selection.toString(), CopyDataType.CopyText);
            } catch (error) {
                console.error("Copy text failed:", error);
            }
        };

        const handleEditorMenuVisibleChange = (visible: boolean) => {
            isContextMenuVisible.value = visible;
        };

        const stopLinkHover = () => {
            if (linkHoverShowTimerRef.value !== null) {
                clearTimeout(linkHoverShowTimerRef.value);
                linkHoverShowTimerRef.value = null;
            }
            hoveredLinkElRef.value = null;
            linkHoverMouseClientRef.value = null;
            linkHoverVisible.value = false;
            linkHoverHref.value = "";
            if (linkHoverRafRef.value !== null) {
                cancelAnimationFrame(linkHoverRafRef.value);
                linkHoverRafRef.value = null;
            }
        };

        const updateLinkHoverRect = () => {
            const hoveredLink = hoveredLinkElRef.value;
            const mousePoint = linkHoverMouseClientRef.value;
            const layout = layoutEl.value;
            if (!hoveredLink || !mousePoint || !layout || !hoveredLink.isConnected || !layout.contains(hoveredLink)) {
                stopLinkHover();
                return;
            }

            const lineRects = Array.from(hoveredLink.getClientRects()).filter(
                (rect) => rect.width > 0 || rect.height > 0,
            );
            if (lineRects.length === 0) {
                stopLinkHover();
                return;
            }
            const lineRect =
                lineRects.find((rect) => mousePoint.y >= rect.top && mousePoint.y <= rect.bottom) ??
                lineRects.reduce((closest, rect) => {
                    const prevCenter = (closest.top + closest.bottom) / 2;
                    const currCenter = (rect.top + rect.bottom) / 2;
                    return Math.abs(currCenter - mousePoint.y) < Math.abs(prevCenter - mousePoint.y) ? rect : closest;
                });

            const layoutRect = layout.getBoundingClientRect();
            linkHoverRect.value = {
                // Y 轴按“当前行”定位，避免在行内随鼠标抖动
                top: lineRect.top - layoutRect.top + LINK_TOOLTIP_LINE_OFFSET,
                left: mousePoint.x - layoutRect.left,
                width: 1,
                height: 1,
            };
        };

        const scheduleLinkHoverRectUpdate = () => {
            if (!linkHoverVisible.value || linkHoverRafRef.value !== null) {
                return;
            }
            linkHoverRafRef.value = requestAnimationFrame(() => {
                linkHoverRafRef.value = null;
                updateLinkHoverRect();
            });
        };

        // ─── 关闭扩展面板 ───
        const handleClose = async () => {
            // Set closing flag first to prevent auto-save race condition
            isClosing.value = true;

            // Force immediate save before closing
            if (saveTimer.value) {
                clearTimeout(saveTimer.value);
                saveTimer.value = null;
            }

            // Use current editor content (not props.content which is initial value)
            const currentContent = currentEditorContent.value;
            if (currentContent !== lastSavedContent.value) {
                console.log("[MarkdownEditor] Saving before close...");
                const success = await conversationManagerStore.updateDocCardContent(
                    props.conversationId,
                    props.articleId,
                    currentContent,
                );

                if (success) {
                    lastSavedContent.value = currentContent;
                } else {
                    console.warn("[MarkdownEditor] Save failed, closing anyway");
                }
            }

            extensionPanelAPI?.closeExtensionPanel();
        };

        // ─── 全屏切换：通过 extension panel 的通用全屏能力，与编辑器解耦 ───
        const handleFullscreen = () => {
            extensionPanelAPI?.setPanelFullscreen(!isFullscreen.value);
        };

        // ─── 内容更新 ───
        const handleContentUpdate = (value: string) => {
            currentEditorContent.value = value;
            modifiedTime.value = new Date().toISOString();
            emit("update:content", value);

            // 内容变化时重新提取标题（节流：与内容更新一起触发无需额外延迟）
            extractHeadings();

            // Trigger auto-save
            scheduleAutoSave(value);
        };

        /**
         * Manual save triggered by Ctrl+S / Cmd+S
         */
        const handleManualSave = async () => {
            // Skip if already saving
            if (isSaving.value) {
                return;
            }

            const currentContent = currentEditorContent.value;

            // Cancel pending auto-save timer
            if (saveTimer.value) {
                clearTimeout(saveTimer.value);
                saveTimer.value = null;
            }

            // Show processing toast (will stay until manually closed)
            const processingToast = notifyStore.showToast({
                type: "processing",
                message: backendStore.translate("Saving..."),
            });
            const startTime = Date.now();

            // Execute save
            isSaving.value = true;
            const minDuration = 800;
            try {
                const success = await conversationManagerStore.updateDocCardContent(
                    props.conversationId,
                    props.articleId,
                    currentContent,
                );

                // Ensure processing toast shows at least 500ms
                const elapsed = Date.now() - startTime;
                if (elapsed < minDuration) {
                    await new Promise((resolve) => setTimeout(resolve, minDuration - elapsed));
                }

                // Close processing toast
                notifyStore._closeToast(processingToast.id);
                isSaving.value = false;

                if (success) {
                    lastSavedContent.value = currentContent;
                    // Update the modified time to reflect the manual save operation
                    modifiedTime.value = new Date().toISOString();
                    await notifyStore.showToast({
                        type: "success",
                        message: backendStore.translate("Saved successfully!"),
                        duration: 2000,
                    }).promise;
                    console.log("[MarkdownEditor] Manual save success:", props.articleId);
                } else {
                    await notifyStore.showToast({
                        type: "error",
                        message: backendStore.translate("Save failed"),
                        duration: 3000,
                    }).promise;
                    console.error("[MarkdownEditor] Manual save failed");
                }
            } catch (error) {
                // Ensure processing toast shows at least 300ms
                const elapsed = Date.now() - startTime;
                if (elapsed < minDuration) {
                    await new Promise((resolve) => setTimeout(resolve, minDuration - elapsed));
                }

                // Close processing toast
                notifyStore._closeToast(processingToast.id);

                await notifyStore.showToast({
                    type: "error",
                    message: backendStore.translate("Save failed"),
                    duration: 3000,
                }).promise;
                console.error("[MarkdownEditor] Manual save error:", error);
            }
        };

        /**
         * Debounced auto-save to backend
         */
        const scheduleAutoSave = async (content: string) => {
            // Don't save if panel is closing
            if (isClosing.value) {
                return;
            }

            // Clear existing timer
            if (saveTimer.value) {
                clearTimeout(saveTimer.value);
            }

            // Skip if content unchanged
            if (content === lastSavedContent.value) {
                return;
            }

            // Schedule save
            saveTimer.value = setTimeout(async () => {
                isSaving.value = true;

                try {
                    const success = await conversationManagerStore.updateDocCardContent(
                        props.conversationId,
                        props.articleId,
                        content,
                    );

                    if (success) {
                        lastSavedContent.value = content;
                        console.log("[MarkdownEditor] Auto-saved:", props.articleId);
                    } else {
                        console.error("[MarkdownEditor] Auto-save failed");
                    }
                } catch (error) {
                    console.error("[MarkdownEditor] Auto-save error:", error);
                } finally {
                    isSaving.value = false;
                }
            }, AUTO_SAVE_DELAY);
        };

        // Cleanup timer on unmount
        onBeforeUnmount(() => {
            // 清理宽度检测 observer
            resizeObserver?.disconnect();
            // 清理待执行的 resize rAF，防止组件卸载后回调仍然执行
            if (resizeRafId !== null) {
                cancelAnimationFrame(resizeRafId);
                resizeRafId = null;
            }
            // 清理编辑器横向内边距变量
            layoutEl.value?.style.removeProperty("--mde-editor-padding-x");
            // 清理 tooltip 宽度 CSS 变量
            document.documentElement.style.removeProperty("--mde-tooltip-max-width");

            // 清理悬浮目录定时器
            if (tocHoverTimer.value) {
                clearTimeout(tocHoverTimer.value);
            }

            if (saveTimer.value) {
                clearTimeout(saveTimer.value);
            }
            clearReferenceHighlight();

            // 清理链接点击监听器，避免内存泄漏
            if (linkClickHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        view.dom.removeEventListener("click", linkClickHandlerRef.value!);
                    }
                });
            }

            // 清理正文脚注点击监听器，避免内存泄漏
            if (footnoteClickHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        view.dom.removeEventListener("click", footnoteClickHandlerRef.value!);
                    }
                });
            }

            // 清理链接 hover 监听器，避免内存泄漏
            if (linkHoverMoveHandlerRef.value || linkHoverLeaveHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        if (linkHoverMoveHandlerRef.value) {
                            view.dom.removeEventListener("mousemove", linkHoverMoveHandlerRef.value);
                        }
                        if (linkHoverLeaveHandlerRef.value) {
                            view.dom.removeEventListener("mouseleave", linkHoverLeaveHandlerRef.value);
                        }
                    }
                });
            }

            if (linkHoverWindowUpdateHandlerRef.value) {
                window.removeEventListener("scroll", linkHoverWindowUpdateHandlerRef.value, true);
                window.removeEventListener("resize", linkHoverWindowUpdateHandlerRef.value);
            }
            stopLinkHover();

            // 清理键盘事件监听器，避免内存泄漏
            if (keydownHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        view.dom.removeEventListener("keydown", keydownHandlerRef.value!);
                    }
                });
            }

            // 清理标题 Backspace 拦截监听器，避免内存泄漏
            if (headingBackspaceHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        view.dom.removeEventListener("keydown", headingBackspaceHandlerRef.value!, true);
                    }
                });
            }

            // 清理右键菜单监听器，避免内存泄漏
            if (contextmenuHandlerRef.value) {
                editorInstance.value?.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (view?.dom) {
                        view.dom.removeEventListener("contextmenu", contextmenuHandlerRef.value!);
                    }
                });
            }
        });

        // ─── 提取 ProseMirror doc 中的标题 ───
        const extractHeadings = () => {
            const editor = editorInstance.value;
            if (!editor) return;
            try {
                editor.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (!view) return;
                    const items: TocHeading[] = [];
                    view.state.doc.descendants((node: any, pos: number) => {
                        if (node.type.name === "heading") {
                            items.push({
                                level: node.attrs.level as number,
                                text: node.textContent,
                                pos,
                            });
                            return false; // 不深入 heading 内部
                        }
                    });
                    tocHeadings.value = items;
                });
            } catch {
                /* ignore */
            }
        };

        // ─── 跳转到目录节点（直接操作 DOM，绕过 ProseMirror 内部滚动机制） ───
        const jumpToHeading = (pos: number) => {
            const editor = editorInstance.value;
            if (!editor) return;
            try {
                editor.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (!view) return;

                    // nodeDOM(pos) 返回 pos 处节点对应的 DOM 元素
                    const rawDom = view.nodeDOM(pos);
                    const el =
                        rawDom instanceof Element
                            ? rawDom
                            : rawDom instanceof Node
                              ? (rawDom as Node).parentElement
                              : null;

                    if (el) {
                        el.scrollIntoView({ behavior: "smooth", block: "start" });
                    }
                    view.focus();
                });
            } catch {
                /* 内容更新后 pos 可能过期，忽略跳转错误 */
            }
        };

        // ─── 编辑器就绪 ───
        const handleEditorReady = (editor: any) => {
            editorInstance.value = editor;
            // 首次就绪后提取标题
            setTimeout(extractHeadings, 100);

            try {
                editor.action((ctx: any) => {
                    const view = ctx.get(editorViewCtx);
                    if (!view) return;

                    // 将光标定位到文档末尾
                    // 使用 Selection.atEnd 而非 TextSelection.create(doc, doc.content.size)：
                    // doc.content.size 指向最后一个块节点关闭标签之后的「块级」位置，不是合法的文本位置。
                    // 若在该错误位置执行 insertText，ProseMirror 会新建段落，导致 addMark 范围
                    // 偏移 1 个单位，最终链接文本的最后一个字符无法被标记。
                    // Selection.atEnd 能正确解析到最后段落内的末尾文本位置，彻底规避此问题。
                    const { doc } = view.state;
                    const selection = Selection.atEnd(doc);
                    view.dispatch(view.state.tr.setSelection(selection));
                    view.focus();

                    const updateActiveStates = () => {
                        try {
                            editor.action((ctx2: any) => {
                                const v = ctx2.get(editorViewCtx);
                                if (!v) return;
                                const { from, to, empty } = v.state.selection;
                                const { doc } = v.state;
                                const states: Record<string, boolean> = {};

                                // 检测当前块节点类型（标题、列表、引用等）
                                const $from = v.state.selection.$from;
                                const currentNode = $from.parent;

                                // undo / redo 可用性
                                states["undo-disabled"] = undoDepth(v.state) === 0;
                                states["redo-disabled"] = redoDepth(v.state) === 0;

                                if (currentNode) {
                                    // 标题检测
                                    if (currentNode.type.name === "heading") {
                                        const level = currentNode.attrs.level || 1;
                                        states[`h${level}`] = true;
                                        states["heading"] = true;
                                    } else {
                                        // 正文格式：设置 text 和 heading 状态，让按钮显示高亮
                                        states["text"] = true;
                                        states["heading"] = true;
                                    }

                                    // 列表检测（需要向上遍历，光标在 paragraph 层，列表在更上层）
                                    for (let d = $from.depth; d > 0; d--) {
                                        const ancestor = $from.node(d);
                                        if (ancestor.type.name === "ordered_list") {
                                            states["ordered-list"] = true;
                                            break;
                                        } else if (ancestor.type.name === "bullet_list") {
                                            states["unordered-list"] = true;
                                            break;
                                        }
                                    }

                                    // 引用检测
                                    if (currentNode.type.name === "blockquote") {
                                        states["quote"] = true;
                                    }
                                }

                                // 检测选中的 mark（加粗、斜体、删除线、链接）
                                if (!empty) {
                                    doc.nodesBetween(from, to, (node: any) => {
                                        node.marks?.forEach((mark: any) => {
                                            const markName = mark.type.name;
                                            if (markName === "strong") states["bold"] = true;
                                            if (markName === "emphasis") states["italic"] = true;
                                            if (markName === "strike_through") states["strikethrough"] = true;
                                            if (markName === "link") states["link"] = true;
                                        });
                                    });
                                } else {
                                    // 光标状态下，检查光标位置的 marks
                                    const marks = $from.marks();
                                    marks.forEach((mark: any) => {
                                        const markName = mark.type.name;
                                        if (markName === "strong") states["bold"] = true;
                                        if (markName === "emphasis") states["italic"] = true;
                                        if (markName === "strike_through") states["strikethrough"] = true;
                                        if (markName === "link") states["link"] = true;
                                    });
                                }

                                activeStates.value = states;
                            });
                        } catch {
                            /* ignore */
                        }
                    };

                    // 关键修复：拦截 view.updateState 来捕获所有 ProseMirror 事务
                    // 这样无论是通过命令、按钮还是 API 调用触发的状态变化都会被捕获
                    const originalUpdateState = view.updateState.bind(view);
                    view.updateState = function (state: any) {
                        originalUpdateState(state);
                        // 在状态更新后立即更新工具栏状态
                        // 使用 setTimeout(0) 确保在 ProseMirror 完全处理完事务后再执行
                        setTimeout(updateActiveStates, 0);
                    };

                    // ─── 链接点击处理：Ctrl/Cmd + 点击打开链接 ───
                    const linkClickHandler = (event: MouseEvent) => {
                        // 检查是否按下了 Ctrl (Windows/Linux) 或 Cmd (Mac)
                        const isModifierPressed = event.ctrlKey || event.metaKey;
                        if (!isModifierPressed) {
                            return;
                        }

                        // 获取点击位置对应的文档位置
                        const pos = view.posAtCoords({ left: event.clientX, top: event.clientY });
                        if (!pos) {
                            return;
                        }

                        // 检查点击位置是否有链接 mark
                        const { doc } = view.state;
                        const resolvedPos = doc.resolve(pos.pos);

                        // 获取该位置的所有 marks
                        const marks = resolvedPos.marks();
                        const linkMark = marks.find((mark: any) => mark.type.name === "link");

                        if (linkMark) {
                            // 获取链接地址
                            const href = linkMark.attrs.href;
                            if (href) {
                                // 阻止默认行为
                                event.preventDefault();
                                event.stopPropagation();

                                // 通过后端在默认浏览器中打开链接
                                backendStore.requestSystem("openUrl", href);
                            }
                        }
                    };

                    view.dom.addEventListener("click", linkClickHandler);
                    linkClickHandlerRef.value = linkClickHandler;

                    // ─── 正文脚注点击处理：跳转到底部引用列表并闪烁高亮 ───
                    const resolveElementFromEventTarget = (target: EventTarget | null): Element | null => {
                        if (target instanceof Element) {
                            return target;
                        }
                        if (target instanceof Node) {
                            return target.parentElement;
                        }
                        return null;
                    };

                    const footnoteClickHandler = (event: MouseEvent) => {
                        if (props.references.length === 0) {
                            return;
                        }

                        const targetEl = resolveElementFromEventTarget(event.target);
                        if (!targetEl) {
                            return;
                        }

                        const footnoteRef = targetEl.closest('sup[data-type="footnote_reference"]');
                        if (!(footnoteRef instanceof HTMLElement)) {
                            return;
                        }

                        const label = footnoteRef.dataset.label?.trim();
                        if (!label) {
                            return;
                        }

                        const referenceIndex = resolveReferenceIndexFromLabel(label);
                        if (!referenceIndex) {
                            return;
                        }

                        event.preventDefault();
                        event.stopPropagation();
                        jumpToReferenceByIndex(referenceIndex);
                    };

                    view.dom.addEventListener("click", footnoteClickHandler);
                    footnoteClickHandlerRef.value = footnoteClickHandler;

                    // ─── 链接 hover：显示链接地址 tooltip ───
                    const resolveAnchorFromTarget = (target: EventTarget | null): HTMLAnchorElement | null => {
                        const element =
                            target instanceof Element ? target : target instanceof Node ? target.parentElement : null;
                        if (!element) return null;

                        const anchor = element.closest("a[href]");
                        if (!(anchor instanceof HTMLAnchorElement)) return null;
                        if (!view.dom.contains(anchor)) return null;
                        return anchor;
                    };

                    const linkHoverMoveHandler = (event: MouseEvent) => {
                        const anchor = resolveAnchorFromTarget(event.target);
                        if (!anchor) {
                            if (linkHoverVisible.value) {
                                stopLinkHover();
                            }
                            return;
                        }

                        const href = anchor.getAttribute("href")?.trim();
                        if (!href) {
                            stopLinkHover();
                            return;
                        }

                        linkHoverMouseClientRef.value = { x: event.clientX, y: event.clientY };
                        if (hoveredLinkElRef.value !== anchor || linkHoverHref.value !== href) {
                            // 切换到新链接时先取消上一个 pending 的显示计时器
                            if (linkHoverShowTimerRef.value !== null) {
                                clearTimeout(linkHoverShowTimerRef.value);
                                linkHoverShowTimerRef.value = null;
                            }
                            linkHoverVisible.value = false;
                            hoveredLinkElRef.value = anchor;
                            linkHoverHref.value = href;
                            // 不立即定位 span，让它停在 (0,0) 远离光标
                            // 120ms 后光标停下时再定位并显示，防止快速掠过闪烁
                            // 也避免 span mount 时恰好在光标下触发 el-tooltip 的 hover 检测
                            linkHoverShowTimerRef.value = setTimeout(() => {
                                linkHoverShowTimerRef.value = null;
                                updateLinkHoverRect(); // 用光标最终停止位置更新 rect
                                linkHoverVisible.value = true;
                            }, 300);
                            return;
                        }

                        scheduleLinkHoverRectUpdate();
                    };

                    const linkHoverLeaveHandler = () => {
                        stopLinkHover();
                    };

                    const linkHoverWindowUpdateHandler = () => {
                        if (!linkHoverVisible.value) return;
                        scheduleLinkHoverRectUpdate();
                    };

                    view.dom.addEventListener("mousemove", linkHoverMoveHandler);
                    view.dom.addEventListener("mouseleave", linkHoverLeaveHandler);
                    window.addEventListener("scroll", linkHoverWindowUpdateHandler, true);
                    window.addEventListener("resize", linkHoverWindowUpdateHandler);
                    linkHoverMoveHandlerRef.value = linkHoverMoveHandler;
                    linkHoverLeaveHandlerRef.value = linkHoverLeaveHandler;
                    linkHoverWindowUpdateHandlerRef.value = linkHoverWindowUpdateHandler;

                    // ─── 键盘快捷键：Ctrl+S / Cmd+S 手动保存 ───
                    const keydownHandler = (event: KeyboardEvent) => {
                        console.log(
                            "[MarkdownEditor] Keydown event:",
                            event.key,
                            "Code:",
                            event.code,
                            "Ctrl:",
                            event.ctrlKey,
                            "Meta:",
                            event.metaKey,
                        );
                        // Ctrl+S (Windows/Linux) 或 Cmd+S (Mac)
                        // 使用 event.code 检测物理按键，避免修饰键影响 event.key 的值
                        if ((event.ctrlKey || event.metaKey) && event.code === "KeyS") {
                            event.preventDefault();
                            event.stopPropagation();
                            handleManualSave();
                            console.log("[MarkdownEditor] Ctrl/Cmd+S detected, triggering manual save");
                        }
                    };
                    view.dom.addEventListener("keydown", keydownHandler);
                    keydownHandlerRef.value = keydownHandler;

                    // ─── 标题前 Backspace 直接转正文，而非降级 ───
                    // 使用捕获阶段（capture: true），确保在 ProseMirror 的冒泡阶段处理器之前拦截事件
                    const headingBackspaceHandler = (event: KeyboardEvent) => {
                        if (event.key !== "Backspace") return;
                        const { $from, empty } = view.state.selection;
                        // 仅在无选区且光标位于标题节点起始位置时拦截
                        if (empty && $from.parent.type.name === "heading" && $from.parentOffset === 0) {
                            event.stopImmediatePropagation();
                            event.preventDefault();
                            // 直接将当前标题转为正文，后续 Backspace 由 ProseMirror 默认处理（段落合并）
                            try {
                                editor.action(callCommand(turnIntoTextCommand.key));
                            } catch {
                                /* ignore */
                            }
                        }
                    };
                    view.dom.addEventListener("keydown", headingBackspaceHandler, true);
                    headingBackspaceHandlerRef.value = headingBackspaceHandler;

                    // ─── 右键菜单：文字选中复制 ───
                    view.dom.addEventListener("contextmenu", handleEditorContextMenu);
                    contextmenuHandlerRef.value = handleEditorContextMenu;

                    // 初始更新一次状态
                    setTimeout(updateActiveStates, 100);
                });
            } catch (error) {
                console.error("[MarkdownEditor] Setup listener error:", error);
            }
        };

        // ─── 工具栏动作分发 ───
        const handleToolbarAction = (id: string, payload?: any) => {
            const editor = editorInstance.value;
            if (
                !editor &&
                id !== "close" &&
                id !== "fullscreen" &&
                id !== "copy" &&
                id !== "export" &&
                id !== "export-pdf" &&
                id !== "export-markdown" &&
                id !== "export-word" &&
                id !== "print" &&
                id !== "share"
            ) {
                return;
            }

            const execCommand = (cmdKey: any, ...args: any[]) => {
                try {
                    editor!.action(callCommand(cmdKey, ...args));
                    // 保持焦点
                    editor!.action((ctx: any) => {
                        const view = ctx.get(editorViewCtx);
                        view?.focus();
                    });
                } catch (e) {
                    console.error(`[MarkdownEditor] ${id} error:`, e);
                }
            };

            switch (id) {
                // G5: 全屏、关闭
                case "close":
                    handleClose();
                    break;

                case "fullscreen":
                    handleFullscreen();
                    break;

                case "toc":
                    tocOpen.value = !tocOpen.value;
                    break;

                // G1: 撤销、重做
                case "undo":
                    execCommand(undoCommand.key);
                    break;

                case "redo":
                    execCommand(redoCommand.key);
                    break;

                // G2: 标题、加粗、斜体、删除线、链接
                case "heading":
                    if (payload) {
                        // payload: "text" | "h1" | "h2" | "h3" | "h4" | "h5" | "h6"
                        if (payload === "text") {
                            // 转为正文：使用 turnIntoTextCommand
                            execCommand(turnIntoTextCommand.key);
                        } else {
                            const level = parseInt(payload.slice(1));
                            execCommand(wrapInHeadingCommand.key, level);
                        }
                    }
                    break;

                case "bold":
                    execCommand(toggleStrongCommand.key);
                    break;

                case "italic":
                    execCommand(toggleEmphasisCommand.key);
                    break;

                case "strikethrough":
                    execCommand(toggleStrikethroughCommand.key);
                    break;

                case "link":
                    editor.action((ctx: any) => {
                        const view = ctx.get(editorViewCtx);
                        if (!view) return;

                        const { from, to, empty } = view.state.selection;
                        const { doc, schema } = view.state;
                        const linkMarkType = schema.marks.link;

                        // 选区内含有链接时，取消所有涉及的完整链接（含紧邻同链接段）
                        if (!empty && linkMarkType) {
                            const hrefs = new Set<string>();
                            doc.nodesBetween(from, to, (node: any) => {
                                if (node.isText) {
                                    node.marks.forEach((mark: any) => {
                                        if (mark.type === linkMarkType) {
                                            hrefs.add(mark.attrs.href as string);
                                        }
                                    });
                                }
                            });

                            if (hrefs.size > 0) {
                                const tr = view.state.tr;
                                hrefs.forEach((href) => {
                                    // 收集文档中所有该 href 的文本段位置（文档序）
                                    const segments: Array<[number, number]> = [];
                                    doc.descendants((node: any, pos: number) => {
                                        if (
                                            node.isText &&
                                            node.marks.some(
                                                (m: any) => m.type === linkMarkType && m.attrs.href === href,
                                            )
                                        ) {
                                            segments.push([pos, pos + node.nodeSize]);
                                        }
                                    });

                                    // 对每个与选区重叠的段，向两侧扩展到完整的紧邻连续段组
                                    const toRemove: Array<[number, number]> = [];
                                    segments.forEach((seg, idx) => {
                                        if (seg[1] <= from || seg[0] >= to) return; // 不重叠
                                        let groupStart = idx;
                                        while (
                                            groupStart > 0 &&
                                            segments[groupStart - 1]![1] === segments[groupStart]![0]
                                        ) {
                                            groupStart--;
                                        }
                                        let groupEnd = idx;
                                        while (
                                            groupEnd < segments.length - 1 &&
                                            segments[groupEnd]![1] === segments[groupEnd + 1]![0]
                                        ) {
                                            groupEnd++;
                                        }
                                        toRemove.push([segments[groupStart]![0], segments[groupEnd]![1]]);
                                    });

                                    // 合并重叠范围，避免重复 removeMark
                                    toRemove.sort((a, b) => a[0] - b[0]);
                                    const merged: Array<[number, number]> = [];
                                    toRemove.forEach(([s, e]) => {
                                        const last = merged[merged.length - 1];
                                        if (last && s <= last[1]) {
                                            last[1] = Math.max(last[1], e);
                                        } else {
                                            merged.push([s, e]);
                                        }
                                    });

                                    merged.forEach(([rangeFrom, rangeTo]) => {
                                        tr.removeMark(rangeFrom, rangeTo, linkMarkType);
                                    });
                                });

                                view.dispatch(tr);
                                view.focus();
                                return; // 不打开对话框
                            }
                        }

                        // 光标在链接内时（无选区），取消光标所在的完整链接（含紧邻同链接段）
                        if (empty && linkMarkType) {
                            const $from = view.state.selection.$from;
                            const cursorLinkMark = $from.marks().find((m: any) => m.type === linkMarkType);

                            if (cursorLinkMark) {
                                const href = cursorLinkMark.attrs.href as string;
                                const segments: Array<[number, number]> = [];
                                doc.descendants((node: any, pos: number) => {
                                    if (
                                        node.isText &&
                                        node.marks.some((m: any) => m.type === linkMarkType && m.attrs.href === href)
                                    ) {
                                        segments.push([pos, pos + node.nodeSize]);
                                    }
                                });

                                // 找到包含光标的段（光标在段末边界时退一个字符再找）
                                let hitIdx = segments.findIndex((seg) => seg[0] <= from && from < seg[1]);
                                if (hitIdx < 0) {
                                    hitIdx = segments.findIndex((seg) => seg[0] < from && from <= seg[1]);
                                }

                                if (hitIdx >= 0) {
                                    let groupStart = hitIdx;
                                    while (
                                        groupStart > 0 &&
                                        segments[groupStart - 1]![1] === segments[groupStart]![0]
                                    ) {
                                        groupStart--;
                                    }
                                    let groupEnd = hitIdx;
                                    while (
                                        groupEnd < segments.length - 1 &&
                                        segments[groupEnd]![1] === segments[groupEnd + 1]![0]
                                    ) {
                                        groupEnd++;
                                    }

                                    const tr = view.state.tr;
                                    tr.removeMark(segments[groupStart]![0], segments[groupEnd]![1], linkMarkType);
                                    view.dispatch(tr);
                                    view.focus();
                                    return; // 不打开对话框
                                }
                            }
                        }

                        // 选区内无链接 / 光标不在链接内 → 打开对话框添加链接
                        linkDialogInitialText.value = empty ? "" : doc.textBetween(from, to);
                        linkDialogVisible.value = true;
                    });
                    break;

                // G3: 有序列表、无序列表、缩进、引用、分割线
                case "ordered-list":
                case "unordered-list": {
                    const isOrdered = id === "ordered-list";
                    const targetListTypeName = isOrdered ? "ordered_list" : "bullet_list";
                    const otherListTypeName = isOrdered ? "bullet_list" : "ordered_list";
                    const targetCommand = isOrdered ? wrapInOrderedListCommand.key : wrapInBulletListCommand.key;

                    editor!.action((ctx: any) => {
                        const view = ctx.get(editorViewCtx);
                        if (!view) return;

                        const { state } = view;
                        const { $from } = state.selection;
                        const schema = state.schema;
                        const listItemType = schema.nodes.list_item;

                        // 向上查找最近的列表祖先节点及其位置
                        let foundListTypeName: string | null = null;
                        let listPos = -1;
                        for (let i = $from.depth; i > 0; i--) {
                            const nodeTypeName = $from.node(i).type.name;
                            if (nodeTypeName === targetListTypeName || nodeTypeName === otherListTypeName) {
                                foundListTypeName = nodeTypeName;
                                listPos = $from.before(i);
                                break;
                            }
                        }

                        if (foundListTypeName === targetListTypeName) {
                            // 已在目标类型列表中 → 用 liftListItem 取消列表（转为段落）
                            if (listItemType) {
                                liftListItem(listItemType)(state, view.dispatch);
                                view.focus();
                            }
                        } else if (foundListTypeName === otherListTypeName) {
                            // 在另一种列表中 → setNodeMarkup 改变列表节点类型，并同步更新所有 list_item 的 listType
                            const targetType = schema.nodes[targetListTypeName];
                            if (targetType && listPos >= 0) {
                                const listNode = $from.node(
                                    // 找到对应深度
                                    (() => {
                                        for (let i = $from.depth; i > 0; i--) {
                                            if ($from.node(i).type.name === otherListTypeName) return i;
                                        }
                                        return -1;
                                    })(),
                                );
                                const newListTypeAttr = isOrdered ? "ordered" : "bullet";
                                let tr = state.tr.setNodeMarkup(listPos, targetType, null);
                                // 同步更新每个 list_item 的 listType attr
                                if (listItemType && listNode) {
                                    let childOffset = 0;
                                    listNode.forEach((child: any) => {
                                        if (child.type === listItemType) {
                                            tr = tr.setNodeMarkup(listPos + 1 + childOffset, child.type, {
                                                ...child.attrs,
                                                listType: newListTypeAttr,
                                            });
                                        }
                                        childOffset += child.nodeSize;
                                    });
                                }
                                view.dispatch(tr);
                                view.focus();
                            }
                        } else {
                            // 不在任何列表中 → 创建新列表
                            ctx.get(commandsCtx).call(targetCommand);
                            view.focus();
                        }
                    });
                    break;
                }

                case "outdent":
                case "indent":
                    // 在光标所在段落开头增加/删除2个空格
                    editor.action((ctx: any) => {
                        const view = ctx.get(editorViewCtx);
                        if (!view) return;

                        const { state } = view;
                        const { $from } = state.selection;
                        // 当前块内容起始位置（段落/块的第一个字符处）
                        const blockStart = $from.start();

                        if (id === "indent") {
                            // 缩进：在段落开头插入2个空格
                            const tr = state.tr.insertText("  ", blockStart);
                            view.dispatch(tr);
                        } else {
                            // 减少缩进：删除段落开头最多2个空格
                            const blockEnd = $from.end();
                            if (blockEnd <= blockStart) return;
                            const checkLen = Math.min(2, blockEnd - blockStart);
                            const leadingText = state.doc.textBetween(blockStart, blockStart + checkLen);
                            if (leadingText.startsWith("  ")) {
                                view.dispatch(state.tr.delete(blockStart, blockStart + 2));
                            } else if (leadingText.startsWith(" ")) {
                                view.dispatch(state.tr.delete(blockStart, blockStart + 1));
                            }
                        }

                        view.focus();
                    });
                    break;

                case "quote": {
                    editor!.action((ctx: any) => {
                        const view = ctx.get(editorViewCtx);
                        if (!view) return;

                        const { state } = view;
                        const { $from } = state.selection;
                        const schema = state.schema;

                        // 检查是否在列表项中
                        let listItemPos = -1;
                        let listItemDepth = -1;
                        for (let d = $from.depth; d > 0; d--) {
                            const node = $from.node(d);
                            if (node.type.name === "list_item") {
                                listItemPos = $from.before(d);
                                listItemDepth = d;
                                break;
                            }
                        }

                        if (listItemPos >= 0 && listItemDepth > 0) {
                            // 在列表项中：找到列表容器，将其包裹在 blockquote 中
                            let listContainerPos = -1;
                            let listContainerDepth = -1;

                            for (let d = listItemDepth - 1; d > 0; d--) {
                                const node = $from.node(d);
                                if (node.type.name === "ordered_list" || node.type.name === "bullet_list") {
                                    listContainerPos = $from.before(d);
                                    listContainerDepth = d;
                                    break;
                                }
                            }

                            if (listContainerPos >= 0 && listContainerDepth > 0) {
                                const listContainer = $from.node(listContainerDepth);
                                const blockquoteType = schema.nodes.blockquote;

                                if (blockquoteType && listContainer) {
                                    // 创建一个 blockquote，包含整个列表
                                    const blockquoteNode = blockquoteType.create(null, listContainer);

                                    // 替换整个列表为包裹在 blockquote 中的列表
                                    const tr = state.tr.replaceWith(
                                        listContainerPos,
                                        listContainerPos + listContainer.nodeSize,
                                        blockquoteNode,
                                    );

                                    view.dispatch(tr);
                                    view.focus();
                                }
                            }
                        } else {
                            // 不在列表项中：使用标准命令
                            ctx.get(commandsCtx).call(wrapInBlockquoteCommand.key);
                            view.focus();
                        }
                    });
                    break;
                }

                case "hr":
                    editor.action(insert("\n\n---\n\n"));
                    break;

                // G4: 复制、导出、打印、分享
                case "copy": {
                    // 内容为空时不执行复制操作
                    if (isContentEmpty.value || !currentEditorContent.value) {
                        break;
                    }

                    const text = currentEditorContent.value;

                    const showSuccess = () => {
                        console.log("[MarkdownEditor] Copy successful");
                        void notifyStore.showToast({
                            type: "success",
                            message: backendStore.translate("Copy succeeded."),
                            duration: 2000,
                        });
                    };

                    const formatCopyError = (err: any) => {
                        if (err instanceof Error) {
                            return `${err.name}: ${err.message}`;
                        }
                        if (typeof err === "string") {
                            return err;
                        }
                        try {
                            return JSON.stringify(err);
                        } catch {
                            return String(err);
                        }
                    };

                    const showError = (err: any) => {
                        console.error(`[MarkdownEditor] Copy failed: ${formatCopyError(err)}`, err);
                        void notifyStore.showToast({
                            type: "error",
                            message: backendStore.translate("Copy failed. Please try again."),
                            duration: 3000,
                        });
                    };

                    const fallbackCopyWithExecCommand = () => {
                        const textarea = document.createElement("textarea");
                        textarea.value = text;
                        textarea.style.position = "fixed";
                        textarea.style.opacity = "0";
                        document.body.appendChild(textarea);
                        textarea.focus();
                        textarea.select();
                        textarea.setSelectionRange(0, textarea.value.length);

                        try {
                            // eslint-disable-next-line deprecation/deprecation
                            const successful = document.execCommand("copy");
                            if (!successful) {
                                throw new Error("execCommand returned false");
                            }
                            showSuccess();
                        } finally {
                            document.body.removeChild(textarea);
                        }
                    };

                    const fallbackCopyWithWebApi = () => {
                        if (!navigator.clipboard?.writeText) {
                            fallbackCopyWithExecCommand();
                            return;
                        }

                        navigator.clipboard.writeText(text).then(showSuccess).catch((err) => {
                            console.warn(`[MarkdownEditor] Clipboard API copy failed, fallback to execCommand: ${formatCopyError(err)}`, err);
                            try {
                                fallbackCopyWithExecCommand();
                            } catch (fallbackErr) {
                                showError(fallbackErr);
                            }
                        });
                    };

                    // Qt WebEngine 中浏览器剪贴板能力不稳定，优先走 native QClipboard 通道。
                    // copyToClipboard 是 C++ void slot，QWebChannel 不一定会回调 Promise，需直接派发调用。
                    const systemChannel = backendStore.systemChannel as any;
                    if (systemChannel && typeof systemChannel.copyToClipboard === "function") {
                        try {
                            systemChannel.copyToClipboard(text, CopyDataType.CopyText);
                            showSuccess();
                            break;
                        } catch (err) {
                            console.warn(`[MarkdownEditor] Native copy failed, fallback to Web API: ${formatCopyError(err)}`, err);
                        }
                    }

                    try {
                        fallbackCopyWithWebApi();
                    } catch (fallbackErr) {
                        showError(fallbackErr);
                    }
                    break;
                }

                case "export":
                case "export-pdf":
                case "export-markdown":
                case "export-word": {
                    const formatMap: Record<string, "pdf" | "markdown" | "word"> = {
                        "export-pdf": "pdf",
                        "export-markdown": "markdown",
                        "export-word": "word",
                    };
                    const format =
                        id === "export"
                            ? (typeof payload === "string" ? formatMap[payload] : undefined) || "markdown"
                            : formatMap[id] || "markdown";

                    console.log("[MarkdownEditor] export", format, "articleId:", props.articleId);

                    void conversationManagerStore
                        .exportWorkspaceArticle(props.conversationId, props.articleId, format)
                        .then((success) => {
                            if (success) {
                                void notifyStore.showToast({
                                    type: "success",
                                    message: backendStore.translate("Saved successfully!"),
                                    duration: 3000,
                                });
                            } else {
                                void notifyStore.showToast({
                                    type: "error",
                                    message: backendStore.translate("Failed to save, please try again."),
                                    duration: 3000,
                                });
                            }
                        })
                        .catch((error) => {
                            void notifyStore.showToast({
                                type: "error",
                                message: backendStore.translate("Failed to save, please try again."),
                                duration: 3000,
                            });
                            console.error("[MarkdownEditor] Export error:", error);
                        });
                    break;
                }

                case "print": {
                    // 内容为空时不执行打印操作
                    if (isContentEmpty.value) {
                        break;
                    }

                    if (printInProgress.value) {
                        console.warn("[MarkdownEditor] print is already in progress");
                        break;
                    }

                    const html = exportPrintableHtml(editorInstance.value, props.title || "Document");
                    if (!html) {
                        console.error("[MarkdownEditor] print html is empty");
                        break;
                    }

                    // 调试输出：点击打印时打印最终 HTML
                    console.log("[MarkdownEditor] print html:\n", html);

                    printInProgress.value = true;
                    backendStore
                        .requestConversation("printHTML", html, props.title || "Document")
                        .catch((error) => {
                            console.error("[MarkdownEditor] request printHTML failed:", error);
                        })
                        .finally(() => {
                            printInProgress.value = false;
                        });
                    break;
                }

                case "share":
                    // TODO: 调用分享接口
                    console.log("[MarkdownEditor] share", props.articleId);
                    break;

                default:
                    console.log("[MarkdownEditor] toolbar action:", id, payload);
            }
        };

        return {
            editorInstance,
            isFullscreen,
            modifiedTime,
            activeStates,
            isSaving,
            tocOpen,
            tocHeadings,
            tocHoverActive,
            refsOpen,
            highlightedRefIndex,
            setBodyScrollRef,
            setRefsHeaderEl,
            setRefsListEl,
            handleRefsToggle,
            jumpToHeading,
            handleClose,
            handleContentUpdate,
            handleEditorReady,
            handleToolbarAction,
            handleTocToggleMouseEnter,
            handleTocMouseLeave,
            handleTocMouseEnter,
            linkDialogVisible,
            linkDialogInitialText,
            linkHoverHref,
            linkHoverVisible,
            linkHoverRect,
            handleLinkDialogCancel,
            handleLinkDialogConfirm,
            layoutEl,
            isWide,
            isContentEmpty,
            backendStore,
            isEnableAdvancedCssFeatures: computed(() => backendStore.isEnableAdvancedCssFeatures),
            // 右键菜单相关
            isContextMenuVisible,
            contextMenuItems,
            contextMenuPosition,
            handleEditorMenuSelect,
            handleEditorMenuVisibleChange,
        };
    },

    render() {
        const showReferences = this.$props.references.length > 0 && !this.isContentEmpty;

        return (
            <div class={["markdown-editor", this.isFullscreen && "markdown-editor--fullscreen"]}>
                {/* 链接插入对话框 */}
                <MarkdownEditorLinkDialog
                    visible={this.linkDialogVisible}
                    initialText={this.linkDialogInitialText}
                    onConfirm={this.handleLinkDialogConfirm}
                    onCancel={this.handleLinkDialogCancel}
                />

                {/* 工具栏 */}
                <MarkdownEditorToolbar
                    modifiedTime={this.modifiedTime}
                    isFullscreen={this.isFullscreen}
                    activeStates={this.activeStates}
                    onAction={this.handleToolbarAction}
                    isSaving={this.isSaving}
                    hideFullscreenButton={this.$props.hideFullscreenButton}
                    isContentEmpty={this.isContentEmpty}
                />

                {/* 编辑器主体布局（包含 TOC + 编辑区） */}
                <div ref="layoutEl" class={["markdown-editor__layout", this.isWide && "markdown-editor__layout--wide"]}>
                    {/* 浮层模式遇罩：open 时覆盖编辑区，点击关闭目录（并列模式由 CSS 隐藏） */}
                    {/* 窄屏模式：tocOpen（点击）或 tocHoverActive（hover）时显示 */}
                    {(this.tocOpen || this.tocHoverActive) && !this.isWide && (
                        <div
                            class="mde-toc-mask"
                            onClick={() => {
                                this.tocOpen = false;
                                this.tocHoverActive = false;
                            }}
                        />
                    )}

                    {/* 编辑器链接 hover 提示：保持常驻 DOM，让 el-tooltip 的淡出动画正常工作 */}
                    {this.linkHoverHref && (
                        <Tooltip
                            content={this.linkHoverHref}
                            placement="top"
                            visible={this.linkHoverVisible}
                            popperClass="mde-link-hover-tooltip mde-tooltip"
                        >
                            <span
                                class="mde-link-tooltip-trigger"
                                style={{
                                    top: `${this.linkHoverRect.top}px`,
                                    left: `${this.linkHoverRect.left}px`,
                                    width: `${this.linkHoverRect.width}px`,
                                    height: `${this.linkHoverRect.height}px`,
                                }}
                            />
                        </Tooltip>
                    )}

                    {/* 浮动目录切换按钮：固定于 layout 左侧，浮在目录上方 */}
                    {/* 窄屏模式：hover 触发；宽屏模式：click 触发 */}
                    <Tooltip content={this.backendStore.translate("editor toc")} popperClass="mde-tooltip">
                        <div
                            class={["mde-toc-toggle", (this.tocOpen || this.tocHoverActive) && "mde-toc-toggle--open"]}
                            onClick={() => {
                                // 宽屏模式：click 切换
                                if (this.isWide) {
                                    this.tocOpen = !this.tocOpen;
                                }
                            }}
                            onMouseenter={this.handleTocToggleMouseEnter}
                            onMouseleave={this.handleTocMouseLeave}
                        >
                            <SvgIcon icon="mdeditor-toc-open" size={[16, 16]} />
                        </div>
                    </Tooltip>

                    {/* 目录抽屉（宽屏并列/窄屏浮层） */}
                    <div
                        class={[
                            "mde-toc",
                            (this.tocOpen || this.tocHoverActive) && "mde-toc--open",
                            this.isEnableAdvancedCssFeatures && "mde-toc--advanced-css"
                        ]}
                        onMouseenter={this.handleTocMouseEnter}
                        onMouseleave={this.handleTocMouseLeave}
                    >
                        {/* 占位元素，用于保持布局高度，把目录区域顶到目录按钮下 */}
                        <div class="mde-toc__placeholder"></div>
                        <ScrollBar class="mde-toc__list" momentum edgeBounce>
                            <div class="mde-toc__list-inner">
                                {(() => {
                                    return this.tocHeadings.length === 0 ? (
                                        <div class="mde-toc__empty">{this.backendStore.translate("No outline")}</div>
                                    ) : (
                                        this.tocHeadings.map((h) => (
                                            <div
                                                key={h.pos}
                                                class={["mde-toc__item", `mde-toc__item--h${h.level}`]}
                                                title={h.text}
                                                onClick={() => this.jumpToHeading(h.pos)}
                                            >
                                                <span class="mde-toc__item-text">{h.text}</span>
                                            </div>
                                        ))
                                    );
                                })()}
                            </div>
                        </ScrollBar>
                    </div>

                    {/* 编辑器区域 */}
                    <ScrollBar ref={this.setBodyScrollRef} class="markdown-editor__body" momentum edgeBounce>
                        <div
                            class={[
                                "markdown-editor__body-content",
                                showReferences && this.refsOpen && "markdown-editor__body-content--refs-open",
                            ]}
                        >
                            <MilkdownProvider>
                                <MarkdownEditorInner
                                    modelValue={this.$props.content || ""}
                                    onEditorReady={this.handleEditorReady}
                                    onUpdate:modelValue={this.handleContentUpdate}
                                />
                            </MilkdownProvider>

                            {/* 参考资料面板 */}
                            {showReferences && (
                                <div class={["mde-refs", this.refsOpen && "mde-refs--open"]}>
                                    <div
                                        ref={this.setRefsHeaderEl}
                                        class="mde-refs__header"
                                        onClick={this.handleRefsToggle}
                                    >
                                        <span class="mde-refs__title">
                                            {this.backendStore.translate("References")} ({this.$props.references.length}
                                            )
                                        </span>
                                        <SvgIcon
                                            icon="icon_arrow"
                                            size={[12, 12]}
                                            class={["mde-refs__arrow", this.refsOpen && "mde-refs__arrow--open"]}
                                        />
                                    </div>
                                    <div class="mde-refs__list-wrap">
                                        <div ref={this.setRefsListEl} class="mde-refs__list">
                                            {this.$props.references.map((ref) => (
                                                <Tooltip
                                                    key={ref.index}
                                                    content={ref.url}
                                                    placement="top"
                                                    popperClass="mde-tooltip"
                                                >
                                                    <div
                                                        class={[
                                                            "mde-refs__item",
                                                            this.highlightedRefIndex === String(ref.index) &&
                                                                "mde-refs__item--blink",
                                                        ]}
                                                        data-reference-index={String(ref.index)}
                                                        onClick={(e) => {
                                                            e.preventDefault();
                                                            this.backendStore.requestSystem("openUrl", ref.url);
                                                        }}
                                                    >
                                                        <span class="mde-refs__item-num">{`${ref.index}.`}</span>
                                                        <span class="mde-refs__item-icon-wrap">
                                                            {ref.icon ? (
                                                                <img
                                                                    class="mde-refs__item-icon"
                                                                    src={`file://${ref.icon}`}
                                                                    alt=""
                                                                    onError={(e) => {
                                                                        (e.target as HTMLElement).style.display =
                                                                            "none";
                                                                    }}
                                                                />
                                                            ) : (
                                                                <span class="mde-refs__item-icon-fallback" />
                                                            )}
                                                        </span>
                                                        <span class="mde-refs__item-title">{ref.title}</span>
                                                    </div>
                                                </Tooltip>
                                            ))}
                                        </div>
                                    </div>
                                </div>
                            )}
                        </div>
                    </ScrollBar>
                </div>

                {/* 右键菜单 - Teleport 到 body 避免 position: fixed 受祖先 transform 影响 */}
                <Teleport to="body">
                    <Menu
                        items={this.contextMenuItems}
                        visible={this.isContextMenuVisible}
                        position={this.contextMenuPosition}
                        onUpdateVisible={this.handleEditorMenuVisibleChange}
                        onSelectItem={this.handleEditorMenuSelect}
                    />
                </Teleport>
            </div>
        );
    },
});
