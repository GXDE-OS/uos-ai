/**
 * MarkdownEditorToolbar 组件
 *
 * 工具栏布局（从左到右，一维线性）：
 * [修改时间] | [撤销 重做] | [段落样式 加粗 斜体 删除线 链接] | [有序 无序 取消缩进 缩进 引用 分割线] | [复制 导出 更多] | [全屏 关闭]
 *
 * 响应式折叠策略（ResizeObserver 监听容器宽度）：
 *   宽→窄：
 *     ① 先隐藏修改时间；
 *     ② 依次将 导出→复制→HR→引用→缩进→取消缩进→无序→有序→链接 折叠进更多菜单；
 *   窄→宽：逆序恢复。
 *   分割线跟随相邻元素，不单独处理（G3 全空时 D3 同步消失）。
 */
import {
    defineComponent,
    ref,
    onMounted,
    onBeforeUnmount,
    computed,
    type PropType,
} from "vue";
import SvgIcon from "@/components/SvgIcon";
import Menu from "@/components/menu/Menu";
import Tooltip from "@/components/Tooltip";
import type { MenuItem } from "@/types/menu";
import { useBackendStore } from "@/stores";

// ─────────────── 工具栏按钮类型 ───────────────

interface ToolbarBtn {
    id: string;
    icon: string;
    tooltipKey: string;
    special?: "heading-dropdown";
}

// G1: 撤销 / 重做（始终显示）
const G1_ITEMS: ToolbarBtn[] = [
    { id: "undo", icon: "mdeditor-undo", tooltipKey: "Undo" },
    { id: "redo", icon: "mdeditor-redo", tooltipKey: "Redo" },
];

// G2: 段落样式 / 加粗 / 斜体 / 删除线 / 链接
const G2_ITEMS: ToolbarBtn[] = [
    { id: "heading",       icon: "mdeditor-h1",            tooltipKey: "Heading",         special: "heading-dropdown" },
    { id: "bold",          icon: "mdeditor-bold",          tooltipKey: "Bold"             },
    { id: "italic",        icon: "mdeditor-italic",        tooltipKey: "Italic"           },
    { id: "strikethrough", icon: "mdeditor-strikethrough", tooltipKey: "Strikethrough"    },
    { id: "link",          icon: "mdeditor-link",          tooltipKey: "Link Text"        },
];

// G3: 有序 / 无序 / 取消缩进 / 缩进 / 引用 / 分割线（工具栏原始顺序，同时用于 More 菜单展示）
const G3_ITEMS: ToolbarBtn[] = [
    { id: "ordered-list",   icon: "mdeditor-ordered",   tooltipKey: "Numbered List"   },
    { id: "unordered-list", icon: "mdeditor-unordered", tooltipKey: "Bulleted List"   },
    { id: "outdent",        icon: "mdeditor-outdent",    tooltipKey: "Decrease Indent" },
    { id: "indent",         icon: "mdeditor-indent",     tooltipKey: "Increase Indent" },
    { id: "quote",          icon: "mdeditor-quote",      tooltipKey: "Blockquote"      },
    { id: "hr",             icon: "mdeditor-hr",         tooltipKey: "Divider"         },
];

// 折叠优先级顺序（先折叠的在前，collapsedCount 表示前 N 项已折叠进 More）
const COLLAPSE_ORDER: string[] = [
    "export",         // 0
    "copy",           // 1
    "hr",             // 2
    "quote",          // 3
    "indent",         // 4
    "outdent",        // 5
    "unordered-list", // 6
    "ordered-list",   // 7 ← 折叠后 G3 全空，D3 同时消失（节省 51px，非常规 40px）
    "link",           // 8
];

// ─── 布局宽度常量（px，与 CSS .mde-toolbar > * + * { margin-left: 10px } 对应）───
const W_BTN  = 30;  // 普通按钮（CSS .mde-tb-btn width:30px）
const W_HEAD = 40;  // heading 按钮（CSS .mde-tb-btn--heading width:40px）
const W_DIV  = 1;   // 分割线（CSS .mde-tb-divider width:1px）
const W_TIME = 120; // 修改时间区域估算宽度（实际由内容决定，CSS无max-width；典型内容 "Edited on 2026-04-28" 或 "Edited on 14:30"）
const GAP    = 10;  // .mde-toolbar 相邻子元素 margin-left

// 工具栏完整 slot 列表（从左到右顺序；spacer 不占固定宽度，不在此列表）
interface Slot { id: string; w: number }
const ALL_SLOTS: Slot[] = [
    { id: "time",          w: W_TIME },
    { id: "undo",          w: W_BTN  },
    { id: "redo",          w: W_BTN  },
    { id: "D2",            w: W_DIV  },
    { id: "heading",       w: W_HEAD },
    { id: "bold",          w: W_BTN  },
    { id: "italic",        w: W_BTN  },
    { id: "strikethrough", w: W_BTN  },
    { id: "link",          w: W_BTN  },
    { id: "D3",            w: W_DIV  },
    { id: "ordered-list",  w: W_BTN  },
    { id: "unordered-list",w: W_BTN  },
    { id: "outdent",       w: W_BTN  },
    { id: "indent",        w: W_BTN  },
    { id: "quote",         w: W_BTN  },
    { id: "hr",            w: W_BTN  },
    { id: "D4",            w: W_DIV  },
    { id: "copy",          w: W_BTN  },
    { id: "export",        w: W_BTN  },
    { id: "more",          w: W_BTN  },
    { id: "fullscreen",    w: W_BTN  },
    { id: "close",         w: W_BTN  },
];

/** 根据隐藏 id 集合，计算可见 slot 的总宽度（自身宽度之和 + slot 间 gap）*/
function calcWidth(hidden: Set<string>): number {
    const vis = ALL_SLOTS.filter(s => !hidden.has(s.id));
    return vis.length === 0 ? 0 : vis.reduce((acc, s) => acc + s.w, 0) + (vis.length - 1) * GAP;
}

// ─────────────── 组件 ───────────────

export default defineComponent({
    name: "MarkdownEditorToolbar",

    props: {
        /** 修改时间（ISO string 或空） */
        modifiedTime: {
            type: String,
            default: "",
        },
        /** 是否全屏 */
        isFullscreen: {
            type: Boolean,
            default: false,
        },
        /** 各按钮 active 状态 */
        activeStates: {
            type: Object as PropType<Record<string, boolean>>,
            default: () => ({}),
        },
        onAction: {
            type: Function as PropType<(id: string, payload?: any) => void>,
            default: undefined,
        },
        /** 是否正在保存 */
        isSaving: {
            type: Boolean,
            default: false,
        },
        /** 隐藏全屏按钮（用于自动全屏模式） */
        hideFullscreenButton: {
            type: Boolean,
            default: false,
        },
        /** 内容是否为空（用于禁用复制/打印/导出等操作） */
        isContentEmpty: {
            type: Boolean,
            default: false,
        },
    },

    setup(props) {
        const backend = useBackendStore();
        const docConversionSupported = ref(false);

        // 工具栏 DOM 引用
        const toolbarRef = ref<HTMLElement | null>(null);
        // 修改时间区 DOM 引用（用于实测宽度）
        const timeRef = ref<HTMLElement | null>(null);
        // 当前可用宽度（ResizeObserver 实时更新）
        const availableWidth = ref(9999);
        // 修改时间区的实测宽度（ResizeObserver 实时更新，初始给保守上界）
        const timeWidth = ref(W_TIME);

        // 标题下拉菜单
        const headingMenuVisible = ref(false);
        const headingBtnRef = ref<HTMLElement | null>(null);
        // 标题菜单 hover 延迟关闭定时器
        let headingHideTimer: ReturnType<typeof setTimeout> | null = null;

        // 导出下拉菜单（工具栏中的导出按钮，仅在 export 未折叠时使用）
        const exportMenuVisible = ref(false);
        const exportBtnRef = ref<HTMLElement | null>(null);

        // 更多菜单（汇总所有折叠项 + Print）
        const moreMenuVisible = ref(false);
        const moreMenuRef = ref<HTMLElement | null>(null);

        // 根据光标位置的 activeStates 自动映射当前标题图标
        const currentHeadingIcon = computed(() => {
            const s = props.activeStates;
            if (s["h1"]) return "mdeditor-h1";
            if (s["h2"]) return "mdeditor-h2";
            if (s["h3"]) return "mdeditor-h3";
            if (s["h4"]) return "mdeditor-h4";
            if (s["h5"]) return "mdeditor-h5";
            if (s["h6"]) return "mdeditor-h6";
            return "mdeditor-text";
        });

        // ─── ResizeObserver ───
        let ro: ResizeObserver | null = null;
        let roTime: ResizeObserver | null = null;
        // rAF 节流句柄：同一帧内多次回调合并为一次，避免 collapseState 每帧重算 + Vue re-render
        let roRafId: number | null = null;
        let roTimeRafId: number | null = null;
        // 累积最新宽度值（避免节流期间丢弃后续 callback）
        let pendingWidth: number | null = null;
        let pendingTimeWidth: number | null = null;

        onMounted(() => {
            if (!toolbarRef.value) return;
            ro = new ResizeObserver((entries) => {
                // 记录最新的宽度（取整避免亚像素抖动）
                const latestWidth = Math.round(entries[entries.length - 1]?.contentRect.width ?? 0);
                pendingWidth = latestWidth;

                // rAF 节流：同一帧内多次 resize 只触发一次 Vue 更新
                if (roRafId !== null) return;
                roRafId = requestAnimationFrame(() => {
                    roRafId = null;
                    if (pendingWidth !== null && pendingWidth !== availableWidth.value) {
                        availableWidth.value = pendingWidth;
                    }
                    pendingWidth = null;
                });
            });
            ro.observe(toolbarRef.value);
        });

        // timeRef 挂载后才开始观测（v-if 控制，初次 mount 后触发）
        const observeTimeEl = (el: HTMLElement | null) => {
            roTime?.disconnect();
            // 切换元素时清理旧的待执行 rAF
            if (roTimeRafId !== null) {
                cancelAnimationFrame(roTimeRafId);
                roTimeRafId = null;
            }
            pendingTimeWidth = null;
            if (!el) return;
            roTime = new ResizeObserver((entries) => {
                // 记录最新的宽度（取整避免亚像素抖动）
                const latestWidth = Math.round(entries[entries.length - 1]?.contentRect.width ?? 0);
                pendingTimeWidth = latestWidth;

                // rAF 节流：同一帧内多次 resize 只触发一次 Vue 更新
                if (roTimeRafId !== null) return;
                roTimeRafId = requestAnimationFrame(() => {
                    roTimeRafId = null;
                    if (pendingTimeWidth !== null && pendingTimeWidth !== timeWidth.value) {
                        timeWidth.value = pendingTimeWidth;
                    }
                    pendingTimeWidth = null;
                });
            });
            roTime.observe(el);
        };

        onBeforeUnmount(() => {
            ro?.disconnect();
            roTime?.disconnect();
            if (roRafId !== null) {
                cancelAnimationFrame(roRafId);
                roRafId = null;
            }
            if (roTimeRafId !== null) {
                cancelAnimationFrame(roTimeRafId);
                roTimeRafId = null;
            }
            pendingWidth = null;
            pendingTimeWidth = null;
            if (headingHideTimer !== null) {
                clearTimeout(headingHideTimer);
                headingHideTimer = null;
            }
        });

        // ─── 折叠状态计算（核心逻辑）───
        /**
         * 返回 { hideModifiedTime, collapsedCount }
         * 宽度由 calcWidth(hidden) 精确计算，D3 随 G3 全空自动联动隐藏。
         */
        const collapseState = computed(() => {
            const w = availableWidth.value - 20; // -20: 避免在拖动调整宽度的过程中，工具条被挤压到右边界外，搞点余量。

            /** 根据参数构建隐藏集合 */
            const build = (hideTime: boolean, count: number): Set<string> => {
                const hidden = new Set<string>();
                if (props.hideFullscreenButton) hidden.add("fullscreen");
                if (hideTime) { hidden.add("time"); }
                COLLAPSE_ORDER.slice(0, count).forEach(id => hidden.add(id));
                if (G3_ITEMS.every(btn => hidden.has(btn.id))) hidden.add("D3");
                // D4 随 copy 和 export 同时被折叠而隐藏
                if (hidden.has("copy") && hidden.has("export")) hidden.add("D4");
                return hidden;
            };

            // 用实测宽度替换 ALL_SLOTS 中 time slot 的估算值
            const calcWidthAdjusted = (hidden: Set<string>): number => {
                const vis = ALL_SLOTS
                    .filter(s => !hidden.has(s.id))
                    .map(s => s.id === "time" ? { ...s, w: timeWidth.value } : s);
                return vis.length === 0 ? 0 : vis.reduce((acc, s) => acc + s.w, 0) + (vis.length - 1) * GAP;
            };

            if (calcWidthAdjusted(build(false, 0)) <= w) return { hideModifiedTime: false, collapsedCount: 0 };
            if (calcWidthAdjusted(build(true,  0)) <= w) return { hideModifiedTime: true,  collapsedCount: 0 };
            for (let i = 1; i <= COLLAPSE_ORDER.length; i++) {
                if (calcWidthAdjusted(build(true, i)) <= w) return { hideModifiedTime: true, collapsedCount: i };
            }
            return { hideModifiedTime: true, collapsedCount: COLLAPSE_ORDER.length };
        });

        // ─── 工具函数 ───

        /** 判断某按钮 id 是否已折叠进 More 菜单 */
        const isCollapsed = (id: string): boolean => {
            const idx = COLLAPSE_ORDER.indexOf(id);
            return idx !== -1 && idx < collapseState.value.collapsedCount;
        };

        /** G3 是否有至少一个按钮仍在工具栏中（决定 D3 和 G3 组是否渲染） */
        const g3HasVisible = computed(() => G3_ITEMS.some(btn => !isCollapsed(btn.id)));

        // ─── 时间格式化 ───
        const formattedTime = computed(() => {
            if (props.isSaving) return backend.translate("Saving...");
            if (!props.modifiedTime) return backend.translate("Unsaved");
            try {
                const d = new Date(props.modifiedTime);
                const now = new Date();
                const isSameDate =
                    d.getFullYear() === now.getFullYear() &&
                    d.getMonth() === now.getMonth() &&
                    d.getDate() === now.getDate();
                const formatted = isSameDate
                    ? `${String(d.getHours()).padStart(2, "0")}:${String(d.getMinutes()).padStart(2, "0")}`
                    : `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`;
                return `${backend.translate("Edited on")} ${formatted}`;
            } catch {
                return props.modifiedTime;
            }
        });

        // ─── 菜单管理 ───
        const closeAllMenus = (except?: string) => {
            if (except !== "heading") headingMenuVisible.value = false;
            if (except !== "export")  exportMenuVisible.value = false;
            if (except !== "more")    moreMenuVisible.value = false;
        };

        // 标题按钮 wrapper hover：鼠标进入展开，离开延迟 120ms 后关闭
        const handleHeadingWrapperEnter = () => {
            if (headingHideTimer !== null) {
                clearTimeout(headingHideTimer);
                headingHideTimer = null;
            }
            headingMenuVisible.value = true;
            closeAllMenus("heading");
        };

        const handleHeadingWrapperLeave = () => {
            headingHideTimer = setTimeout(() => {
                headingMenuVisible.value = false;
                headingHideTimer = null;
            }, 120);
        };

        // 点击外部关闭所有菜单
        const handleDocClick = (e: MouseEvent) => {
            const target = e.target as Node;
            if (
                headingBtnRef.value?.contains(target) ||
                exportBtnRef.value?.contains(target) ||
                moreMenuRef.value?.contains(target)
            ) return;
            closeAllMenus();
        };

        onMounted(() => document.addEventListener("click", handleDocClick));
        onBeforeUnmount(() => document.removeEventListener("click", handleDocClick));

        // ─── 动作派发 ───
        const dispatch = (id: string, payload?: any) => {
            props.onAction?.(id, payload);
        };

        // ─── 按钮禁用状态 ───
        const isButtonDisabled = (btn: ToolbarBtn): boolean => {
            let disabled = props.activeStates[btn.id + "-disabled"] ?? false;
            if (btn.id === "copy" && props.isContentEmpty) disabled = true;
            return disabled;
        };

        const handleBtnClick = (btn: ToolbarBtn) => {
            if (isButtonDisabled(btn)) return;
            if (btn.special === "heading-dropdown") {
                headingMenuVisible.value = !headingMenuVisible.value;
                if (headingMenuVisible.value) closeAllMenus("heading");
                return;
            }
            dispatch(btn.id);
        };

        // ─── 菜单数据 ───

        /** 导出菜单项工厂（根据 docConversionSupported 决定是否含 PDF/Word） */
        const createExportMenuItems = (): MenuItem[] => {
            const items: MenuItem[] = [
                { type: "item", id: "export-markdown", label: backend.translate("Save as Markdown"), themeIcon: "text-markdown", disabled: props.isContentEmpty },
            ];
            if (docConversionSupported.value) {
                items.unshift({ type: "item", id: "export-pdf", label: backend.translate("Save as PDF"), themeIcon: "application-pdf", disabled: props.isContentEmpty });
                items.push({ type: "item", id: "export-word", label: backend.translate("Save as Word"), themeIcon: "application-wps-office.docx", disabled: props.isContentEmpty });
            }
            return items;
        };

        // 标题下拉菜单项（checkable）
        const headingMenuItems = computed<MenuItem[]>(() => [
            { type: "item", id: "text", label: backend.translate("Body Text"), icon: "mdeditor-text", checked: !["h1","h2","h3","h4","h5","h6"].some(h => props.activeStates[h]) },
            { type: "item", id: "h1",  label: backend.translate("Heading %1").replace("%1", "1"), icon: "mdeditor-h1", checked: props.activeStates["h1"] },
            { type: "item", id: "h2",  label: backend.translate("Heading %1").replace("%1", "2"), icon: "mdeditor-h2", checked: props.activeStates["h2"] },
            { type: "item", id: "h3",  label: backend.translate("Heading %1").replace("%1", "3"), icon: "mdeditor-h3", checked: props.activeStates["h3"] },
            { type: "item", id: "h4",  label: backend.translate("Heading %1").replace("%1", "4"), icon: "mdeditor-h4", checked: props.activeStates["h4"] },
            { type: "item", id: "h5",  label: backend.translate("Heading %1").replace("%1", "5"), icon: "mdeditor-h5", checked: props.activeStates["h5"] },
            { type: "item", id: "h6",  label: backend.translate("Heading %1").replace("%1", "6"), icon: "mdeditor-h6", checked: props.activeStates["h6"] },
        ]);

        // 工具栏中导出按钮的下拉菜单项
        const exportMenuItems = computed<MenuItem[]>(() => createExportMenuItems());

        /**
         * 更多菜单项：折叠项按工具栏从左到右顺序（G2→G3→G4），组间用 separator 隔开，Print 始终最后
         */
        const moreMenuItems = computed<MenuItem[]>(() => {
            const cc = collapseState.value.collapsedCount;
            const result: MenuItem[] = [];

            // G2 折叠项（仅 Link，工具栏最左侧可折叠项）
            if (COLLAPSE_ORDER.indexOf("link") < cc) {
                result.push({ type: "item", id: "link", icon: "mdeditor-link", label: backend.translate("Link Text"), checked: props.activeStates["link"] });
            }

            // G3 折叠项（按工具栏原始顺序：有序→无序→取消缩进→缩进→引用→分割线）
            const g3Collapsed: MenuItem[] = G3_ITEMS
                .filter(btn => COLLAPSE_ORDER.indexOf(btn.id) < cc)
                .map(btn => ({
                    type: "item" as const, id: btn.id, icon: btn.icon,
                    label: backend.translate(btn.tooltipKey),
                    checked: props.activeStates[btn.id],
                }));
            if (g3Collapsed.length > 0) {
                if (result.length > 0) result.push({ type: "separator" });
                result.push(...g3Collapsed);
            }

            // G4 折叠项（Copy 在前，Export 在后，与工具栏顺序一致）
            const g4Collapsed: MenuItem[] = [];
            if (COLLAPSE_ORDER.indexOf("copy") < cc) {
                g4Collapsed.push({ type: "item", id: "copy", icon: "mdeditor-copy", label: backend.translate("Copy Full Text"), disabled: props.isContentEmpty });
            }
            if (COLLAPSE_ORDER.indexOf("export") < cc) {
                g4Collapsed.push({ type: "submenu", id: "export-submenu", icon: "mdeditor-save-as", label: backend.translate("Export Document"), children: createExportMenuItems(), disabled: props.isContentEmpty });
            }
            if (g4Collapsed.length > 0) {
                if (result.length > 0) result.push({ type: "separator" });
                result.push(...g4Collapsed);
            }

            // Print 始终在最后
            if (result.length > 0) result.push({ type: "separator" });
            result.push({ type: "item", id: "print", icon: "mdeditor-print", label: backend.translate("Print Document"), disabled: props.isContentEmpty });

            return result;
        });

        // ─── 事件处理回调 ───
        const handleHeadingSelect = (item: MenuItem) => {
            headingMenuVisible.value = false;
            dispatch("heading", item.id);
        };

        const handleExportSelect = (item: MenuItem) => {
            exportMenuVisible.value = false;
            if (item.id) dispatch(item.id);
        };

        const handleMoreSelect = (item: MenuItem) => {
            moreMenuVisible.value = false;
            if (item.id) dispatch(item.id);
        };

        // ─── 渲染：单个工具按钮 ───
        const renderBtn = (btn: ToolbarBtn, btnRef?: (el: any) => void) => {
            const isActive   = props.activeStates[btn.id] ?? false;
            const isDisabled = isButtonDisabled(btn);
            return (
                <Tooltip content={backend.translate(btn.tooltipKey)} popperClass="mde-tooltip">
                    <div
                        ref={btnRef}
                        class={[
                            "mde-tb-btn",
                            isActive   && "mde-tb-btn--active",
                            isDisabled && "mde-tb-btn--disabled",
                            btn.special === "heading-dropdown" && "mde-tb-btn--heading",
                        ]}
                        onClick={() => handleBtnClick(btn)}
                    >
                        <SvgIcon icon={btn.special === "heading-dropdown" ? currentHeadingIcon.value : btn.icon} size={[16, 16]} />
                        {btn.special === "heading-dropdown" && (
                            <SvgIcon icon="mdeditor-h-downarrow" class="mde-tb-btn__arrow" size={[8, 8]} />
                        )}
                    </div>
                </Tooltip>
            );
        };

        // ─── 渲染：分割线 ───
        const renderDivider = () => <div class="mde-tb-divider" />;

        return () => {
            const { hideModifiedTime } = collapseState.value;

            return (
                <div class="mde-toolbar" ref={toolbarRef}>

                    {/* 修改时间（可隐藏；ref 回调驱动 ResizeObserver 实测宽度）*/}
                    {!hideModifiedTime && (
                        <div
                            class="mde-tb-modified-time"
                            ref={(el) => { timeRef.value = el as HTMLElement; observeTimeEl(el as HTMLElement); }}
                        >
                            <span class="mde-tb-time-text">{formattedTime.value}</span>
                        </div>
                    )}

                    {/* spacer：按钮区整体右对齐 */}
                    <div class="mde-tb-spacer" />

                    {/* 撤销 / 重做（始终显示）*/}
                    {renderBtn(G1_ITEMS[0]!)}
                    {renderBtn(G1_ITEMS[1]!)}

                    {/* D2 */}
                    {renderDivider()}

                    {/* 段落样式（heading，带 hover 下拉）*/}
                    <div
                        class="mde-tb-heading-wrapper"
                        onMouseenter={handleHeadingWrapperEnter}
                        onMouseleave={handleHeadingWrapperLeave}
                    >
                        {renderBtn(G2_ITEMS[0]!, (el) => { headingBtnRef.value = el; })}
                        <Menu
                            items={headingMenuItems.value}
                            visible={headingMenuVisible.value}
                            triggerRef={headingBtnRef.value}
                            checkable={true}
                            onUpdateVisible={(v: boolean) => { headingMenuVisible.value = v; }}
                            onSelectItem={handleHeadingSelect}
                        />
                    </div>
                    {/* 加粗 / 斜体 / 删除线 */}
                    {renderBtn(G2_ITEMS[1]!)}
                    {renderBtn(G2_ITEMS[2]!)}
                    {renderBtn(G2_ITEMS[3]!)}
                    {/* 链接（可折叠）*/}
                    {!isCollapsed("link") && renderBtn(G2_ITEMS[4]!)}

                    {/* D3（G3 有可见项时）*/}
                    {g3HasVisible.value && renderDivider()}

                    {/* G3：有序 / 无序 / 取消缩进 / 缩进 / 引用 / 分割线（各自可折叠）*/}
                    {G3_ITEMS.map(btn => !isCollapsed(btn.id) ? renderBtn(btn) : null)}

                    {/* D4：copy 或 export 至少有一个可见时才显示，避免紧挨更多按钮 */}
                    {(!isCollapsed("copy") || !isCollapsed("export")) && renderDivider()}

                    {/* 复制（可折叠）*/}
                    {!isCollapsed("copy") && renderBtn({ id: "copy", icon: "mdeditor-copy", tooltipKey: "Copy Full Text" })}

                    {/* 导出（可折叠，带下拉菜单）*/}
                    {!isCollapsed("export") && (
                        <div class="mde-tb-group-more-wrapper">
                            <Tooltip content={backend.translate("Export Document")} popperClass="mde-tooltip">
                                <div
                                    ref={(el) => { exportBtnRef.value = el as HTMLElement; }}
                                    class={[
                                        "mde-tb-btn",
                                        exportMenuVisible.value && "mde-tb-btn--active",
                                        props.isContentEmpty    && "mde-tb-btn--disabled",
                                    ]}
                                    onClick={async () => {
                                        if (props.isContentEmpty) return;
                                        if (!exportMenuVisible.value) {
                                            docConversionSupported.value = !!(await backend.requestServiceConfig("checkDocumentConversionCapability"));
                                        }
                                        exportMenuVisible.value = !exportMenuVisible.value;
                                        closeAllMenus("export");
                                    }}
                                >
                                    <SvgIcon icon="mdeditor-save-as" size={[16, 16]} />
                                </div>
                            </Tooltip>
                            <Menu
                                items={exportMenuItems.value}
                                visible={exportMenuVisible.value}
                                triggerRef={exportBtnRef.value}
                                onUpdateVisible={(v: boolean) => { if (!v) exportMenuVisible.value = false; }}
                                onSelectItem={handleExportSelect}
                            />
                        </div>
                    )}

                    {/* 更多（始终显示，汇总折叠项 + Print；无右侧分割线）*/}
                    <div class="mde-tb-group-more-wrapper">
                        <Tooltip content={backend.translate("More")} popperClass="mde-tooltip">
                            <div
                                ref={(el) => { moreMenuRef.value = el as HTMLElement; }}
                                class={["mde-tb-btn", moreMenuVisible.value && "mde-tb-btn--active"]}
                                onClick={async () => {
                                    if (!moreMenuVisible.value) {
                                        docConversionSupported.value = !!(await backend.requestServiceConfig("checkDocumentConversionCapability"));
                                    }
                                    moreMenuVisible.value = !moreMenuVisible.value;
                                    closeAllMenus("more");
                                }}
                            >
                                <SvgIcon icon="mdeditor-more" size={[16, 16]} />
                            </div>
                        </Tooltip>
                        <Menu
                            items={moreMenuItems.value}
                            visible={moreMenuVisible.value}
                            triggerRef={moreMenuRef.value}
                            onUpdateVisible={(v: boolean) => { if (!v) moreMenuVisible.value = false; }}
                            onSelectItem={handleMoreSelect}
                        />
                    </div>

                    {/* 更多 与 全屏/关闭 之间的分割线 */}
                    {renderDivider()}

                    {/* 全屏（可能隐藏）*/}
                    {!props.hideFullscreenButton && (
                        <Tooltip content={props.isFullscreen ? backend.translate("Exit Full Screen") : backend.translate("Full Screen")} popperClass="mde-tooltip">
                            <div
                                class="mde-tb-btn"
                                onClick={() => dispatch("fullscreen")}
                            >
                                <SvgIcon
                                    icon={props.isFullscreen ? "mdeditor-exit-fullscreen" : "mdeditor-fullscreen"}
                                    size={[16, 16]}
                                />
                            </div>
                        </Tooltip>
                    )}

                    {/* 关闭（始终显示）*/}
                    <Tooltip content={backend.translate("Close")} popperClass="mde-tooltip">
                        <div
                            class="mde-tb-btn mde-tb-btn--close"
                            onClick={() => dispatch("close")}
                        >
                            <SvgIcon icon="mdeditor-close" size={[16, 16]} />
                        </div>
                    </Tooltip>

                </div>
            );
        };
    },
});
