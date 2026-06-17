import { defineComponent, ref, watch, nextTick, inject, onMounted, onUnmounted, TransitionGroup, computed, provide } from "vue";
import type { PropType } from "vue";
import type { OutlineData, OutlineParagraph } from "@/types/conversation";
import OutlineTitle from "./OutlineTitle";
import OutlineSection from "./OutlineSection";
import SvgIcon from "@/components/SvgIcon";
import { QUICK_INPUT_KEY } from "@/types/chat-input";
import { useBackendStore } from "@/stores";

export default defineComponent({
    name: "OutlineViewer",

    components: {
        OutlineTitle,
        OutlineSection,
        SvgIcon,
    },

    props: {
        // 大纲数据
        data: {
            type: Object as PropType<OutlineData>,
            required: true,
        },
        // 是否可编辑
        editable: {
            type: Boolean,
            default: false,
        },
        // 是否是最后一条消息（只有最后一条大纲消息才显示快速输入按钮）
        isLastMessage: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        // 删除大章节事件
        deleteParagraph: (index: number) => typeof index === "number",
        // 删除小章节事件
        deleteSubsection: (paragraphIndex: number, subsectionIndex: number) =>
            typeof paragraphIndex === "number" && typeof subsectionIndex === "number",
        // 大纲内容发生变化（用于持久化编辑结果）
        outlineChange: (data: OutlineData) => data !== null && typeof data === "object",
    },

    setup(props, { emit }) {
        const localData = ref<OutlineData>(JSON.parse(JSON.stringify(props.data)) as OutlineData);
        const backend = useBackendStore();
        const paragraphKeyMap = new WeakMap<OutlineParagraph, string>();
        let paragraphKeySeed = 0;

        // 从初始数据检测并缓存序号风格，供后续新增章节无参考时兜底
        const cachedParagraphStyle = ref<ParagraphStyle | null>(null);
        const cachedSubsectionStyle = ref<SubsectionStyle | null>(null);

        // 国际化文案
        const generateDocFromOutlineText = computed(() => backend.translate("Generate document from outline"));
        const enterChapterTitleText = computed(() => backend.translate("Enter chapter title"));
        const addChapterText = computed(() => backend.translate("Add chapter"));

        // 注入快速输入上下文，大纲渲染完成后触发"基于大纲生成文档"按钮
        const quickInputContext = inject(QUICK_INPUT_KEY, null);

        // 全局拖拽状态追踪（包括小章节拖拽）
        const globalDraggingState = ref(0); // 使用计数器，支持多个拖拽同时存在
        const setGlobalDragging = (dragging: boolean) => {
            if (dragging) {
                globalDraggingState.value += 1;
            } else if (globalDraggingState.value > 0) {
                globalDraggingState.value -= 1;
            }
        };
        // 向子组件提供设置全局拖拽状态的方法
        provide("setOutlineGlobalDragging", setGlobalDragging);

        const tryShowButton = () => {
            if (!props.isLastMessage || !quickInputContext) return;
            quickInputContext.showQuickInput("ai-star", generateDocFromOutlineText.value, "outline-quick-input");
        };

        const getParagraphKey = (paragraph: OutlineParagraph) => {
            const cachedKey = paragraphKeyMap.get(paragraph);
            if (cachedKey) return cachedKey;

            paragraphKeySeed += 1;
            const nextKey = `outline-paragraph-${paragraphKeySeed}`;
            paragraphKeyMap.set(paragraph, nextKey);
            return nextKey;
        };

        const settleSectionGhost = async (ghost: HTMLElement, paragraphKey: string, onSettled?: () => void) => {
            await nextTick();

            const target = document.querySelector<HTMLElement>(`[data-outline-paragraph-key="${paragraphKey}"]`);
            if (!target) {
                onSettled?.();
                ghost.remove();
                return;
            }

            const rect = target.getBoundingClientRect();
            ghost.classList.add("outline-section--ghost-settling");

            await new Promise<void>((resolve) => {
                const finish = () => {
                    ghost.removeEventListener("transitionend", handleTransitionEnd);
                    resolve();
                };

                const handleTransitionEnd = (event: TransitionEvent) => {
                    if (event.target !== ghost) return;
                    finish();
                };

                ghost.addEventListener("transitionend", handleTransitionEnd);
                requestAnimationFrame(() => {
                    ghost.style.top = `${rect.top}px`;
                    ghost.style.left = `${rect.left}px`;
                    ghost.style.width = `${rect.width}px`;
                    ghost.style.height = `${rect.height}px`;
                });
                window.setTimeout(finish, 220);
            });

            onSettled?.();

            ghost.style.opacity = "0";
            await new Promise<void>((resolve) => {
                window.setTimeout(resolve, 120);
            });

            ghost.remove();
        };

        const moveParagraph = (from: number, to: number) => {
            if (from === to) return;
            const paragraphs = localData.value.paragraphs;
            const [item] = paragraphs.splice(from, 1);
            if (!item) return;
            paragraphs.splice(to, 0, item);
        };

        // 初次挂载时触发
        onMounted(() => tryShowButton());

        // 组件卸载时，若当前正在显示按钮，则隐藏（切换消息时发生）
        onUnmounted(() => {
            if (props.isLastMessage && quickInputContext) {
                quickInputContext.hideQuickInput();
            }
        });

        // 切换兄弟消息时，isLastMessage 会变化，需要同步按钮状态
        watch(
            () => props.isLastMessage,
            (isLast) => {
                if (!quickInputContext) return;
                if (isLast) {
                    quickInputContext.showQuickInput(
                        "ai-star",
                        generateDocFromOutlineText.value,
                        "outline-quick-input",
                    );
                } else {
                    quickInputContext.hideQuickInput();
                }
            },
        );

        type NumberingType = "arabic" | "chinese";
        type ParagraphStyle = {
            kind: "paren" | "delim" | "space" | "default";
            numberingType: NumberingType;
            open?: string;
            close?: string;
            delimiter?: string;
            hasSpace?: boolean;
        };
        type SubsectionStyle =
            | { kind: "compound"; separator: "." | "．"; suffix: string; hasSpace: boolean }
            | { kind: "paren"; numberingType: NumberingType; open: string; close: string; hasSpace: boolean }
            | { kind: "delim"; numberingType: NumberingType; delimiter: string; hasSpace: boolean }
            | { kind: "space"; numberingType: NumberingType; hasSpace: boolean };

        const CHINESE_NUM_RE = /^[零一二三四五六七八九十百千万]+$/;

        const toChineseNumber = (num: number) => {
            const digits = ["零", "一", "二", "三", "四", "五", "六", "七", "八", "九"];
            if (num <= 0) return String(num);
            if (num < 10) return digits[num];
            if (num < 20) return num === 10 ? "十" : `十${digits[num % 10]}`;
            if (num < 100) {
                const ten = Math.floor(num / 10);
                const one = num % 10;
                return `${digits[ten]}十${one ? digits[one] : ""}`;
            }

            // 当前大纲序号通常不会超过 99；超过时退化为阿拉伯数字，避免错误中文读法。
            return String(num);
        };

        const formatNumberByType = (num: number, type: NumberingType) => {
            if (type === "chinese") return toChineseNumber(num);
            return String(num);
        };

        const detectNumberingType = (token: string): NumberingType => {
            return CHINESE_NUM_RE.test(token) ? "chinese" : "arabic";
        };

        const detectParagraphStyle = (title: string): ParagraphStyle => {
            const text = (title || "").trimStart();

            const mParen = text.match(/^([（(])\s*(\d+|[零一二三四五六七八九十百千万]+)\s*([）)])(\s*)/);
            if (mParen) {
                return {
                    kind: "paren",
                    numberingType: detectNumberingType(mParen[2] || ""),
                    open: mParen[1] || "(",
                    close: mParen[3] || ")",
                    hasSpace: (mParen[4] || "").length > 0,
                };
            }

            const mDelim = text.match(/^(\d+|[零一二三四五六七八九十百千万]+)\s*([、.．:：)）\-－—])(\s*)/);
            if (mDelim) {
                // "5.4青年节" 类日期格式：点号后紧跟数字且无空格 → 不识别为序号
                if ((mDelim[2] === "." || mDelim[2] === "．") && !mDelim[3] && /\d/.test(text[mDelim[0].length] || "")) {
                    // fall through
                } else {
                    return {
                        kind: "delim",
                        numberingType: detectNumberingType(mDelim[1] || ""),
                        delimiter: mDelim[2] || ".",
                        hasSpace: (mDelim[3] || "").length > 0,
                    };
                }
            }

            const mSpace = text.match(/^(\d+|[零一二三四五六七八九十百千万]+)\s+/);
            if (mSpace) {
                return {
                    kind: "space",
                    numberingType: detectNumberingType(mSpace[1] || ""),
                    hasSpace: true,
                };
            }

            return {
                kind: "default",
                numberingType: "arabic",
                delimiter: ".",
                hasSpace: true,
            };
        };

        const detectParagraphStyleFromContext = (index: number): ParagraphStyle => {
            const self = localData.value.paragraphs[index]?.title;
            if (self) {
                const style = detectParagraphStyle(self);
                if (style.kind !== "default") return style;
            }

            for (let i = 0; i < localData.value.paragraphs.length; i += 1) {
                if (i === index) continue;
                const style = detectParagraphStyle(localData.value.paragraphs[i]?.title || "");
                if (style.kind !== "default") return style;
            }

            return cachedParagraphStyle.value || detectParagraphStyle("");
        };

        const detectSubsectionStyle = (title: string): SubsectionStyle | null => {
            const text = (title || "").trimStart();

            // compound: 1.1 / 1．1（compound 后必须跟分隔符或空格，排除 "5.4青年节" 日期格式）
            const mCompound = text.match(/^(\d+)\s*([.．])\s*(\d+)(?:\s*([、.．:：)）\-－—]))?(\s*)/);
            if (mCompound) {
                if (!mCompound[4] && !mCompound[5] && text.slice(mCompound[0].length)) {
                    // compound 数字后无分隔符无空格且有后续内容 → 可能是日期，跳过
                } else {
                    return {
                        kind: "compound",
                        separator: (mCompound[2] as "." | "．") || ".",
                        suffix: mCompound[4] || "",
                        hasSpace: (mCompound[5] || "").length > 0,
                    };
                }
            }

            // simple: （1）/ (1) / 一、/ 1、 / 1. 等（与章节相同的简单序号模式）
            const mParen = text.match(/^([（(])\s*(\d+|[零一二三四五六七八九十百千万]+)\s*([）)])(\s*)/);
            if (mParen) {
                return {
                    kind: "paren",
                    numberingType: detectNumberingType(mParen[2] || ""),
                    open: mParen[1] || "(",
                    close: mParen[3] || ")",
                    hasSpace: (mParen[4] || "").length > 0,
                };
            }

            const mDelim = text.match(/^(\d+|[零一二三四五六七八九十百千万]+)\s*([、.．:：)）\-－—])(\s*)/);
            if (mDelim) {
                if ((mDelim[2] === "." || mDelim[2] === "．") && !mDelim[3] && /\d/.test(text[mDelim[0].length] || "")) {
                    // fall through
                } else {
                    return {
                        kind: "delim",
                        numberingType: detectNumberingType(mDelim[1] || ""),
                        delimiter: mDelim[2] || ".",
                        hasSpace: (mDelim[3] || "").length > 0,
                    };
                }
            }

            const mSpace = text.match(/^(\d+|[零一二三四五六七八九十百千万]+)\s+/);
            if (mSpace) {
                return {
                    kind: "space",
                    numberingType: detectNumberingType(mSpace[1] || ""),
                    hasSpace: true,
                };
            }

            return null;
        };

        const detectSubsectionStyleFromContext = (
            paragraphIndex: number,
            subsectionIndex: number,
        ): SubsectionStyle | null => {
            const siblings = localData.value.paragraphs[paragraphIndex]?.content || [];
            for (let i = 0; i < siblings.length; i += 1) {
                if (i === subsectionIndex) continue;
                const style = detectSubsectionStyle(siblings[i]?.title || "");
                if (style) return style;
            }
            // 包含自身，无参考时用缓存的初始风格兜底
            const current = siblings[subsectionIndex]?.title || "";
            return detectSubsectionStyle(current) || cachedSubsectionStyle.value;
        };

        const refreshCachedStyles = (data: OutlineData) => {
            const firstPara = data.paragraphs[0];
            if (firstPara) {
                const ps = detectParagraphStyle(firstPara.title);
                if (ps.kind !== "default") cachedParagraphStyle.value = ps;
                if (firstPara.content?.[0]) {
                    const ss = detectSubsectionStyle(firstPara.content[0].title);
                    if (ss) cachedSubsectionStyle.value = ss;
                }
            }
        };

        refreshCachedStyles(localData.value);

        const stripParagraphNumberPrefix = (title: string) => {
            const text = (title || "").trim();

            // 最小化修复：分隔符或空白是必须的，避免 "2025年规划" 被误剥离；点号后紧跟数字且无空格（如 "5.4青年节"）视为日期/小数，不剥离。
            // 后续完善方案：将用户输入视为标题全文，自动序号根据条目数自动分配，后端返回数据中可携带序号风格或提供配置入口。
            const stripped = text
                .replace(/^\s*[（(]\s*(\d+|[零一二三四五六七八九十百千万]+)\s*[)）]\s*/, "")
                .replace(
                    /^\s*(\d+|[零一二三四五六七八九十百千万]+)\s*([.．、:：)）\-－—])\s*/,
                    (m, _n, d) => ((d === "." || d === "．") && m.endsWith(d) && /\d/.test(text[m.length] || "")) ? m : "",
                )
                .replace(/^\s*(\d+|[零一二三四五六七八九十百千万]+)\s+/, "");

            return stripped.trim();
        };

        const stripSubsectionNumberPrefix = (title: string) => {
            const text = (title || "").trim();

            // compound 后必须跟分隔符或空格，避免 "5.4青年节" 被误剥离
            const stripped = text
                .replace(/^\s*\d+\s*[.．]\s*\d+(?:[.．、:：)）\-－—]|\s)\s*/, "")
                .replace(/^\s*[（(]\s*\d+\s*[)）]\s*/, "")
                .replace(
                    /^\s*\d+\s*([.．、:：)）\-－—])\s*/,
                    (m, d) => ((d === "." || d === "．") && m.endsWith(d) && /\d/.test(text[m.length] || "")) ? m : "",
                )
                .replace(/^\s*\d+\s+/, "");

            return stripped.trim();
        };

        const formatParagraphTitle = (index: number, title: string) => {
            const content = stripParagraphNumberPrefix(title);
            const style = detectParagraphStyleFromContext(index);
            const num = formatNumberByType(index + 1, style.numberingType);

            let prefix = `${index + 1}. `;
            if (style.kind === "paren") {
                prefix = `${style.open || "("}${num}${style.close || ")"}${style.hasSpace ? " " : ""}`;
            } else if (style.kind === "delim") {
                prefix = `${num}${style.delimiter || "."}${style.hasSpace ? " " : ""}`;
            } else if (style.kind === "space") {
                prefix = `${num} `;
            } else {
                prefix = `${index + 1}.${style.hasSpace === false ? "" : " "}`;
            }

            return content ? `${prefix}${content}` : prefix;
        };

        const formatSubsectionTitle = (paragraphIndex: number, subsectionIndex: number, title: string) => {
            const content = stripSubsectionNumberPrefix(title);
            const style = detectSubsectionStyleFromContext(paragraphIndex, subsectionIndex);

            let prefix: string;
            if (!style) {
                // 无可识别格式，直接保留原文
                return title;
            } else if (style.kind === "compound") {
                // 复合格式：段落序号.子章节序号
                prefix = `${paragraphIndex + 1}${style.separator}${subsectionIndex + 1}${style.suffix}${style.hasSpace ? " " : ""}`;
            } else if (style.kind === "paren") {
                const num = formatNumberByType(subsectionIndex + 1, style.numberingType);
                prefix = `${style.open}${num}${style.close}${style.hasSpace ? " " : ""}`;
            } else if (style.kind === "delim") {
                const num = formatNumberByType(subsectionIndex + 1, style.numberingType);
                prefix = `${num}${style.delimiter}${style.hasSpace ? " " : ""}`;
            } else {
                // space
                const num = formatNumberByType(
                    subsectionIndex + 1,
                    (style as { numberingType: NumberingType }).numberingType,
                );
                prefix = `${num} `;
            }

            return content ? `${prefix}${content}` : prefix;
        };

        watch(
            () => props.data,
            (value) => {
                localData.value = JSON.parse(JSON.stringify(value)) as OutlineData;
                refreshCachedStyles(localData.value);
                // 注意：此处是外部 props 变化导致的更新，不需要再向上通知
            },
            { deep: true },
        );

        // 所有用户操作修改大纲后，调用此函数通知父组件进行持久化
        const notifyChange = () => {
            emit("outlineChange", JSON.parse(JSON.stringify(localData.value)) as OutlineData);
        };

        // 处理大章节删除
        const handleParagraphDelete = (index: number) => {
            // 先删除章节
            localData.value.paragraphs.splice(index, 1);

            // 仅对章节标题重新编号，不触碰子章节
            const hasNumbering = localData.value.paragraphs.some(
                (p) => detectParagraphStyle(p.title).kind !== "default",
            );
            if (hasNumbering) {
                localData.value.paragraphs.forEach((paragraph, i) => {
                    paragraph.title = formatParagraphTitle(i, paragraph.title);
                });
            }

            emit("deleteParagraph", index);
            notifyChange();
        };

        // 处理小章节删除
        const handleSubsectionDelete = (paragraphIndex: number, subsectionIndex: number) => {
            const paragraph = localData.value.paragraphs[paragraphIndex];
            if (!paragraph) return;

            paragraph.content.splice(subsectionIndex, 1);

            // 若剩余子章节有可识别的序号格式，则重新编号
            const hasNumbering = paragraph.content.some((s) => detectSubsectionStyle(s.title) !== null);
            if (hasNumbering) {
                paragraph.content.forEach((subsection, j) => {
                    subsection.title = formatSubsectionTitle(paragraphIndex, j, subsection.title);
                });
            }

            emit("deleteSubsection", paragraphIndex, subsectionIndex);
            notifyChange();
        };

        const handleTitleUpdate = (value: string) => {
            localData.value.title = value;
            notifyChange();
        };

        const handleParagraphTitleUpdate = (index: number, value: string) => {
            if (!localData.value.paragraphs[index]) return;
            localData.value.paragraphs[index].title = formatParagraphTitle(index, value);
            notifyChange();
        };

        const handleSubsectionTitleUpdate = (paragraphIndex: number, subsectionIndex: number, value: string) => {
            const paragraph = localData.value.paragraphs[paragraphIndex];
            if (!paragraph?.content[subsectionIndex]) return;
            paragraph.content[subsectionIndex].title = formatSubsectionTitle(paragraphIndex, subsectionIndex, value);
            notifyChange();
        };

        const handleSubsectionAdd = (paragraphIndex: number, rawTitle: string) => {
            const paragraph = localData.value.paragraphs[paragraphIndex];
            if (!paragraph) return;

            const newIndex = paragraph.content.length;
            // formatSubsectionTitle 会先剥离用户输入中已有的序号，再按检测到的格式补齐
            const title = formatSubsectionTitle(paragraphIndex, newIndex, rawTitle);
            paragraph.content.push({ title });
            notifyChange();
        };

        // 子章节拖拽完成后重新编号
        const handleReorderSubsections = (paragraphIndex: number) => {
            const paragraph = localData.value.paragraphs[paragraphIndex];
            if (!paragraph) return;
            const hasNumbering = paragraph.content.some((s) => detectSubsectionStyle(s.title) !== null);
            if (hasNumbering) {
                paragraph.content.forEach((s, j) => {
                    s.title = formatSubsectionTitle(paragraphIndex, j, s.title);
                });
            }
            notifyChange();
        };

        // ── 新增章节 ──
        const isAddingSection = ref(false);
        const newSectionTitle = ref("");
        const addSectionInputRef = ref<HTMLInputElement | null>(null);
        const skipSectionBlur = ref(false);

        const handleAddSectionClick = () => {
            isAddingSection.value = true;
            newSectionTitle.value = "";
            nextTick(() => {
                addSectionInputRef.value?.focus();
            });
        };

        const commitSectionAdd = () => {
            const rawTitle = newSectionTitle.value.trim();
            isAddingSection.value = false;
            newSectionTitle.value = "";
            if (!rawTitle) return;

            const newIndex = localData.value.paragraphs.length;
            const title = formatParagraphTitle(newIndex, rawTitle);
            localData.value.paragraphs.push({ title, content: [] });
            notifyChange();
        };

        const cancelSectionAdd = () => {
            isAddingSection.value = false;
            newSectionTitle.value = "";
        };

        const handleSectionAddBlur = () => {
            if (skipSectionBlur.value) {
                skipSectionBlur.value = false;
                return;
            }
            commitSectionAdd();
        };

        const handleSectionAddKeyDown = (event: KeyboardEvent) => {
            if (event.key === "Enter") {
                event.preventDefault();
                skipSectionBlur.value = true;
                commitSectionAdd();
                return;
            }
            if (event.key === "Escape") {
                event.preventDefault();
                skipSectionBlur.value = true;
                cancelSectionAdd();
                addSectionInputRef.value?.blur();
            }
        };

        // ── 章节拖拽排序 ──
        const sectionDragState = ref<{
            draggingParagraph: OutlineParagraph;
            draggingKey: string;
            currentIndex: number;
            originalIndex: number;      // 拖拽开始时的原始位置，用于越界回退
            moved: boolean;
            ghost: HTMLElement | null;
            offsetY: number;
            viewerEl: HTMLElement | null; // 所属 viewer 元素，用于检测是否拖出容器
            lockedNeighborIndex: number; // 迟滞：交换后锁定被换走的邻居，ghost 脱离其交集后才解锁
            lockedGhostMidY: number;    // 锁定时刻 ghost 的中心 Y，用于检测主动变向
            lockedSwapDirection: 1 | -1; // 交换方向：1=向下，-1=向上；只有反向移动才触发变向解锁
        } | null>(null);

        const isParagraphDragging = (paragraph: OutlineParagraph) => {
            return sectionDragState.value?.draggingParagraph === paragraph;
        };

        // 判断是否有任何拖拽正在进行（用于禁用底部按钮的 hover 效果）
        const isAnyDragging = computed(() => {
            return globalDraggingState.value > 0;
        });

        const handleSectionDragStart = (event: MouseEvent, index: number) => {
            if (!props.editable) return;
            event.preventDefault();

            const target = event.currentTarget as HTMLElement;
            const sectionEl = target.closest(".outline-section") as HTMLElement | null;
            if (!sectionEl) return;

            const rect = sectionEl.getBoundingClientRect();
            const ghost = sectionEl.cloneNode(true) as HTMLElement;
            ghost.querySelector(".outline-drag-handle--section")?.remove();
            ghost.classList.add("outline-section--ghost");
            ghost.style.width = `${rect.width}px`;
            ghost.style.height = `${rect.height}px`;
            ghost.style.top = `${rect.top}px`;
            ghost.style.left = `${rect.left}px`;
            // fixed 元素使用视口坐标，挂到 body 避免祖先 transform 影响定位参考系
            document.body.appendChild(ghost);

            sectionDragState.value = {
                draggingParagraph: localData.value.paragraphs[index] as OutlineParagraph,
                draggingKey: getParagraphKey(localData.value.paragraphs[index] as OutlineParagraph),
                currentIndex: index,
                originalIndex: index,
                moved: false,
                ghost,
                offsetY: event.clientY - rect.top,
                viewerEl: sectionEl.closest<HTMLElement>(".outline-viewer"),
                lockedNeighborIndex: -1,
                lockedGhostMidY: 0,
                lockedSwapDirection: 1,
            };

            // 更新全局拖拽状态
            setGlobalDragging(true);

            // ── 自动滚动：拖拽到顶/底端时缓慢滚动聊天消息容器 ──
            const AUTO_SCROLL_ZONE = 80; // 触发区域高度（px）
            const MAX_SCROLL_SPEED = 8;  // 最大每帧滚动速度（px）
            const scrollContainer = sectionEl.closest<HTMLElement>(".scroll-bar-content");
            let autoScrollRafId: number | null = null;
            let currentScrollSpeed = 0;

            const runAutoScroll = () => {
                if (scrollContainer && currentScrollSpeed !== 0) {
                    scrollContainer.scrollTop += currentScrollSpeed;
                }
                autoScrollRafId = requestAnimationFrame(runAutoScroll);
            };

            const startAutoScroll = () => {
                if (autoScrollRafId === null) {
                    autoScrollRafId = requestAnimationFrame(runAutoScroll);
                }
            };

            const stopAutoScroll = () => {
                if (autoScrollRafId !== null) {
                    cancelAnimationFrame(autoScrollRafId);
                    autoScrollRafId = null;
                }
                currentScrollSpeed = 0;
            };

            const onMouseMove = (e: MouseEvent) => {
                const state = sectionDragState.value;
                if (!state || !state.ghost) return;

                // 计算新的 top 位置
                let newTop = e.clientY - state.offsetY;

                // 边界检查：限制 ghost 不超出 chat-view 容器顶部
                const chatView = document.querySelector<HTMLElement>(".chat-view");
                if (chatView) {
                    const containerRect = chatView.getBoundingClientRect();
                    const minTop = containerRect.top; // fixed + 视口坐标系
                    if (newTop < minTop) {
                        newTop = minTop; // clamp 到容器顶部
                    }
                }

                state.ghost.style.top = `${newTop}px`;

                // 检测鼠标是否进入顶/底自动滚动触发区
                if (scrollContainer) {
                    const cr = scrollContainer.getBoundingClientRect();
                    const relY = e.clientY - cr.top;
                    const ch = cr.height;
                    if (relY < AUTO_SCROLL_ZONE) {
                        // 顶部区域：越靠近顶端速度越快
                        const intensity = 1 - relY / AUTO_SCROLL_ZONE;
                        currentScrollSpeed = -Math.ceil(intensity * MAX_SCROLL_SPEED);
                        startAutoScroll();
                    } else if (relY > ch - AUTO_SCROLL_ZONE) {
                        // 底部区域：越靠近底端速度越快
                        const intensity = (relY - (ch - AUTO_SCROLL_ZONE)) / AUTO_SCROLL_ZONE;
                        currentScrollSpeed = Math.ceil(intensity * MAX_SCROLL_SPEED);
                        startAutoScroll();
                    } else {
                        stopAutoScroll();
                    }
                }

                // 计算插入位置
                const container = document.querySelector(".outline-viewer__sections");
                if (!container) return;
                const items = Array.from(container.querySelectorAll<HTMLElement>(".outline-section"));

                // 过滤掉 ghost 本身（克隆自 sectionEl，也有 outline-section 类）
                const ghostRect = state.ghost.getBoundingClientRect();
                const validItems = items.filter((el) => !el.classList.contains("outline-section--ghost"));
                const cur = state.currentIndex;

                // 迟滞锁：交换后锁定被换走的邻居元素，满足以下任一条件才解锁：
                // 1. ghost 完全脱离其矩形（正常继续同向拖动）
                // 2. ghost 向反方向（相对于交换方向）移动超过 20px（用户主动变向）
                // 注意：必须用有符号位移而非绝对距离，否则继续同向移动也会误解锁
                const ghostMidY = (ghostRect.top + ghostRect.bottom) / 2;
                if (state.lockedNeighborIndex >= 0) {
                    const lockedEl = validItems[state.lockedNeighborIndex];
                    let unlock = !lockedEl;
                    if (lockedEl) {
                        const r = lockedEl.getBoundingClientRect();
                        const overlap = Math.min(ghostRect.bottom, r.bottom) - Math.max(ghostRect.top, r.top);
                        if (overlap <= 0) {
                            unlock = true; // 条件1：已脱离交集
                        } else {
                            // 条件2：反向移动超过 20px
                            // 向下交换(dir=1)：ghostMidY 减小才算反向；向上交换(dir=-1)：ghostMidY 增大才算反向
                            const reverseDelta = (ghostMidY - state.lockedGhostMidY) * state.lockedSwapDirection;
                            if (reverseDelta < -20) unlock = true;
                            else return; // 同向或未达阈值 → 阻止触发
                        }
                    }
                    if (unlock) state.lockedNeighborIndex = -1;
                }

                // 分别检查向下覆盖和向上覆盖，选覆盖量更大的方向触发让位
                let targetIndex = cur;
                let bestOverlap = 40; // 触发阈值（px）

                // 向下：ghost 底部压入下方紧邻元素超过阈值
                const nextEl = validItems[cur + 1];
                if (nextEl) {
                    const r = nextEl.getBoundingClientRect();
                    const downOverlap = ghostRect.bottom - r.top;
                    if (downOverlap > bestOverlap) {
                        bestOverlap = downOverlap;
                        targetIndex = cur + 1;
                    }
                }

                // 向上：ghost 顶部压入上方紧邻元素超过阈值（覆盖量更大时优先）
                const prevEl = validItems[cur - 1];
                if (prevEl) {
                    const r = prevEl.getBoundingClientRect();
                    const upOverlap = r.bottom - ghostRect.top;
                    if (upOverlap > bestOverlap) {
                        targetIndex = cur - 1;
                    }
                }

                if (targetIndex !== cur) {
                    moveParagraph(cur, targetIndex);
                    state.currentIndex = targetIndex;
                    state.moved = true;
                    state.lockedNeighborIndex = cur;
                    state.lockedGhostMidY = ghostMidY;
                    state.lockedSwapDirection = targetIndex > cur ? 1 : -1;
                }
            };

            const onMouseUp = async (e: MouseEvent) => {
                stopAutoScroll();
                document.removeEventListener("mousemove", onMouseMove);
                document.removeEventListener("mouseup", onMouseUp);

                const state = sectionDragState.value;
                if (!state) return;

                // 检测释放点是否在 viewer 容器外部
                const isOutsideViewer = state.viewerEl
                    ? (() => {
                          const r = state.viewerEl.getBoundingClientRect();
                          return e.clientX < r.left || e.clientX > r.right ||
                                 e.clientY < r.top  || e.clientY > r.bottom;
                      })()
                    : false;

                if (isOutsideViewer && state.moved) {
                    // 拖拽到 viewer 外部释放：回退到原始位置，不排序、不持久化
                    moveParagraph(state.currentIndex, state.originalIndex);
                    if (state.ghost) {
                        await settleSectionGhost(state.ghost, state.draggingKey, () => {
                            sectionDragState.value = null;
                            setGlobalDragging(false);
                        });
                    } else {
                        sectionDragState.value = null;
                        setGlobalDragging(false);
                    }
                    return;
                }

                if (state.moved) {
                    const arr = localData.value.paragraphs;
                    const hasNumbering = arr.some((p) => detectParagraphStyle(p.title).kind !== "default");
                    if (hasNumbering) {
                        arr.forEach((p, i) => {
                            p.title = formatParagraphTitle(i, p.title);
                        });
                    }

                    if (state.ghost) {
                        await settleSectionGhost(state.ghost, state.draggingKey, () => {
                            sectionDragState.value = null;
                            setGlobalDragging(false);
                        });
                    } else {
                        sectionDragState.value = null;
                        setGlobalDragging(false);
                    }

                    notifyChange();
                    return;
                }

                if (state.ghost) {
                    state.ghost.remove();
                }
                sectionDragState.value = null;
                setGlobalDragging(false);
            };

            document.addEventListener("mousemove", onMouseMove);
            document.addEventListener("mouseup", onMouseUp);
        };

        return {
            localData,
            handleParagraphDelete,
            handleSubsectionDelete,
            handleTitleUpdate,
            handleParagraphTitleUpdate,
            handleSubsectionTitleUpdate,
            handleSubsectionAdd,
            handleReorderSubsections,
            isAddingSection,
            newSectionTitle,
            addSectionInputRef,
            handleAddSectionClick,
            handleSectionAddBlur,
            handleSectionAddKeyDown,
            handleSectionDragStart,
            // 国际化
            enterChapterTitleText,
            addChapterText,
            getParagraphKey,
            isParagraphDragging,
            isAnyDragging,
        };
    },

    render() {
        return (
            <div class={{ "outline-viewer": true, "outline-viewer--dragging": this.isAnyDragging }}>
                <OutlineTitle
                    title={this.localData.title}
                    editable={this.$props.editable}
                    onUpdateTitle={this.handleTitleUpdate}
                />
                <div class="outline-viewer__sections">
                    <TransitionGroup name="section-list" tag="div">
                        {this.localData.paragraphs.map((paragraph, index) => (
                            <OutlineSection
                                key={this.getParagraphKey(paragraph)}
                                domKey={this.getParagraphKey(paragraph)}
                                paragraph={paragraph}
                                editable={this.$props.editable}
                                dragging={this.isParagraphDragging(paragraph)}
                                index={index}
                                onDelete={this.handleParagraphDelete}
                                onDeleteSubsection={this.handleSubsectionDelete}
                                onUpdateTitle={this.handleParagraphTitleUpdate}
                                onUpdateSubsectionTitle={this.handleSubsectionTitleUpdate}
                                onAddSubsection={this.handleSubsectionAdd}
                                onReorderSubsections={this.handleReorderSubsections}
                                onDragStart={(e: MouseEvent) => this.handleSectionDragStart(e, index)}
                            />
                        ))}
                    </TransitionGroup>
                    {this.$props.editable && this.isAddingSection && (
                        <div class="outline-section--adding">
                            <input
                                ref="addSectionInputRef"
                                class="outline-section__add-input"
                                value={this.newSectionTitle}
                                placeholder={this.enterChapterTitleText}
                                onInput={(event) => {
                                    this.newSectionTitle = (event.target as HTMLInputElement).value;
                                }}
                                onBlur={this.handleSectionAddBlur}
                                onKeydown={this.handleSectionAddKeyDown}
                            />
                        </div>
                    )}
                    <div
                        class={{
                            "outline-viewer__add-section": true,
                            disabled: !this.$props.editable,
                        }}
                        onClick={this.handleAddSectionClick}
                    >
                        <SvgIcon icon="add" />
                        <span>{this.addChapterText}</span>
                    </div>
                </div>
            </div>
        );
    },
});
