import {
    defineComponent,
    onMounted,
    onUnmounted,
    ref,
    computed,
    inject,
    onBeforeUpdate,
    onUpdated,
    type Ref,
} from "vue";
import SvgIcon from "@/components/SvgIcon";
import { useRecentDocs, type RecentDoc } from "./composables/useRecentDocs";
import {
    useWritingTemplates,
    type WritingTemplate,
    TemplateCategory,
    TEMPLATE_TABS,
} from "./composables/useWritingTemplates";
import { useChatInput } from "@/composables/useChatInput";
import { EXTENSION_PANEL_KEY } from "@/types/extension-panel";
import MarkdownEditor from "@/views/window/mainwindow/page/chat/components/markdownEditor/MarkdownEditor";
import ScrollBar from "@/components/ScrollBar";
import { useConversationManagerStore, useBackendStore } from "@/stores";
import { useThemeIcon } from "@/utils/loadThemeIcon";

// 布局常量
const MIN_CARD_WIDTH = 184; // 卡片最小宽度 184px
const CARD_GAP = 10; // 卡片间距 10px
const ROW_HEIGHT = 64; // 单行高度：54px 卡片高度 + 10px 间距，从 CSS 读取

/**
 * 动态卡片宽度计算 Hook
 * @param containerRef 容器 DOM 引用
 * @returns cardWidth - 动态计算的卡片宽度, columns - 当前列数
 */
function useDynamicCardWidth(containerRef: Ref<HTMLElement | undefined>) {
    const cardWidth = ref(MIN_CARD_WIDTH);
    const columns = ref(1);

    // 计算卡片宽度
    const calculateLayout = () => {
        const el = containerRef.value;
        if (!el) {
            cardWidth.value = MIN_CARD_WIDTH;
            columns.value = 1;
            return;
        }

        const containerWidth = el.getBoundingClientRect().width;

        // 计算能容纳的最大列数：n * minWidth + (n-1) * gap <= containerWidth
        // => n <= (containerWidth + gap) / (minWidth + gap)
        const maxColumns = Math.floor((containerWidth + CARD_GAP) / (MIN_CARD_WIDTH + CARD_GAP));
        columns.value = Math.max(maxColumns, 1);

        // 计算实际卡片宽度，填满整行：cardWidth = (containerWidth - (n-1) * gap) / n
        const actualCardWidth = (containerWidth - (columns.value - 1) * CARD_GAP) / columns.value;
        cardWidth.value = Math.max(actualCardWidth, MIN_CARD_WIDTH);
    };

    // 监听容器尺寸变化
    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
        setTimeout(() => {
            calculateLayout();
            if (containerRef.value) {
                resizeObserver = new ResizeObserver(() => {
                    calculateLayout();
                });
                resizeObserver.observe(containerRef.value);
            }
        }, 100);
    });

    onUnmounted(() => {
        if (resizeObserver) {
            resizeObserver.disconnect();
            resizeObserver = null;
        }
    });

    return {
        cardWidth,
        columns,
    };
}

/**
 * 计算卡片左右 margin
 * @param index 当前卡片索引
 * @param columns 当前列数
 * @param totalCount 总卡片数量
 * @returns CSS 样式对象
 */
function getCardMargin(index: number, columns: number, totalCount: number) {
    const isFirstInRow = index % columns === 0;
    const isLastInRow = (index + 1) % columns === 0 || index === totalCount - 1;

    return {
        marginLeft: isFirstInRow ? "0" : `${CARD_GAP / 2}px`,
        marginRight: isLastInRow ? "0" : `${CARD_GAP / 2}px`,
    };
}

/**
 * 模板卡片列表容器：切换 tab 时高度变化会带平滑过渡动画
 * 单独抽成 Composition API 组件，以便使用 onBeforeUpdate / onUpdated
 */
const TemplatesGrid = defineComponent({
    name: "TemplatesGrid",
    props: {
        templates: {
            type: Array as () => WritingTemplate[],
            required: true,
        },
        onCardClick: {
            type: Function as unknown as () => (template: WritingTemplate) => void,
            required: true,
        },
    },
    setup(props) {
        const gridEl = ref<HTMLElement>();
        let heightBeforeUpdate = 0;

        // 使用动态卡片宽度 Hook
        const { cardWidth, columns } = useDynamicCardWidth(gridEl as Ref<HTMLElement | undefined>);

        // 更新前记录当前高度
        onBeforeUpdate(() => {
            if (gridEl.value) {
                heightBeforeUpdate = gridEl.value.offsetHeight;
            }
        });

        // 更新后：从旧高度平滑过渡到新高度
        onUpdated(() => {
            const el = gridEl.value;
            if (!el || !heightBeforeUpdate) return;
            const newH = el.scrollHeight;
            if (heightBeforeUpdate === newH) return;

            el.style.transition = "";
            el.style.height = `${heightBeforeUpdate}px`;
            el.style.overflow = "hidden";
            void el.offsetHeight; // 强制回流，确保初始状态生效
            el.style.transition = "height 0.25s ease";
            el.style.height = `${newH}px`;
            el.addEventListener(
                "transitionend",
                () => {
                    el.style.height = "";
                    el.style.overflow = "";
                    el.style.transition = "";
                },
                { once: true },
            );
        });

        return () => (
            <div ref={gridEl} class="writing-assistant__templates-grid">
                {props.templates.map((template, index) => {
                    const { marginLeft, marginRight } = getCardMargin(index, columns.value, props.templates.length);

                    return (
                        <div
                            key={template.id}
                            class="writing-assistant__template-card"
                            style={{
                                width: `${cardWidth.value}px`,
                                minWidth: `${MIN_CARD_WIDTH}px`,
                                marginLeft,
                                marginRight,
                                marginBottom: "10px",
                            }}
                            onClick={() => props.onCardClick(template)}
                        >
                            {/* 第一行：图标 + 名称 */}
                            <div class="template--header">
                                <div class="template--icon"></div>
                                <span class="template--name">{template.name}</span>
                            </div>
                            {/* 第二行：描述 */}
                            <span class="template--desc">{template.description}</span>
                        </div>
                    );
                })}
            </div>
        );
    },
});

/**
 * 写作助手 (UOS_AI_WRITING) 的欢迎页个性化组件
 * 上半部分展示最近创作的文档列表（md 内容存于数据库，通过 id 查询）
 * 下半部分展示写作模板列表（分类 Tab + 模板卡片）
 * 文档列表为空时整个组件不渲染
 */
export default defineComponent({
    name: "WritingAssistant",

    setup() {
        const { docs, loading, load } = useRecentDocs();
        const backend = useBackendStore();

        // 国际化文案
        const recentTitle = computed(() => backend.translate("Recent Creations"));
        const viewAllText = computed(() => backend.translate("View All"));
        const collapseText = computed(() => backend.translate("Collapse"));
        const docIconName = ref("text-markdown");
        const docIconUrl = useThemeIcon(docIconName, { width: 24, height: 24 });
        const {
            templates: allTemplates,
            loading: templatesLoading,
            load: loadTemplates,
            filterByCategory,
        } = useWritingTemplates();
        const expanded = ref(false);
        const activeCategory = ref<string>(TemplateCategory.ALL);

        // 获取 Input 填充能力
        const { fillInput, focusInput } = useChatInput();

        const extensionPanelAPI = inject(EXTENSION_PANEL_KEY);
        const conversationManagerStore = useConversationManagerStore();

        const recentScrollRef = ref<InstanceType<typeof ScrollBar>>();
        const recentGridRef = ref<HTMLElement>();

        // 使用动态卡片宽度 Hook
        const { cardWidth, columns } = useDynamicCardWidth(recentGridRef as Ref<HTMLElement | undefined>);

        // 动态展开高度
        const expandedHeight = computed(() => {
            // 计算展开后需要的行数（最多 3 行，或根据实际文档数量）
            const totalRows = Math.ceil(docs.value.length / columns.value);
            const maxRows = Math.min(totalRows, 3);
            return maxRows * ROW_HEIGHT;
        });

        onMounted(() => {
            load();
            loadTemplates();
        });

        const handleDocClick = async (doc: RecentDoc) => {
            if (!extensionPanelAPI) {
                console.warn("[WritingAssistant] ExtensionPanel API not available.");
                return;
            }

            const article = await conversationManagerStore.getWorkspaceArticle(doc.conversation_id, doc.id);
            if (!article) {
                console.error("[WritingAssistant] Failed to load article:", doc.id);
                return;
            }

            extensionPanelAPI.openExtensionPanelWithOptions({
                content: () => (
                    <MarkdownEditor
                        key={doc.id}
                        content={article.content}
                        articleId={doc.id}
                        title={article.title}
                        conversationId={doc.conversation_id}
                        updatedAt={article.updated_at}
                        references={article.references ?? []}
                        hideFullscreenButton={true}
                    />
                ),
                autoFullscreen: true,
            });
        };

        const handleTemplateClick = (template: WritingTemplate) => {
            console.log("Template clicked:", template);
            // 填充模板内容到输入框（替换模式）
            fillInput(template.content, "replace");
            // 自动聚焦输入框
            focusInput();
        };

        /** 根据当前分类过滤的模板列表 */
        const filteredTemplates = computed(() => filterByCategory(activeCategory.value));

        /** 切换分类 */
        const handleTabClick = (category: string) => {
            activeCategory.value = category;
        };

        return {
            docs,
            loading,
            expanded,
            docIconUrl,
            handleDocClick,
            allTemplates,
            templatesLoading,
            filteredTemplates,
            activeCategory,
            TEMPLATE_TABS,
            handleTemplateClick,
            handleTabClick,
            recentScrollRef,
            recentGridRef,
            columns,
            expandedHeight,
            cardWidth,
            // 国际化
            recentTitle,
            viewAllText,
            collapseText,
        };
    },

    render() {
        // 没有最近创作的文档时，整个模块不显示
        if (!this.docs.length) return null;

        const showMoreBtn = this.columns > 0 && this.docs.length > this.columns;

        return (
            <div class="writing-assistant">
                {/* ===== 最近创作模块 ===== */}
                <div class="writing-assistant__recent">
                    {/* 标题行：左侧「最近创作」，右侧「查看全部 + 箭头图标」（条目>4时才显示） */}
                    <div class="writing-assistant__recent-header">
                        <span class="writing-assistant__recent-title">{this.recentTitle}</span>
                        {showMoreBtn && (
                            <span
                                class="writing-assistant__recent-more"
                                onClick={() => {
                                    const isCollapsing = this.expanded;
                                    this.expanded = !this.expanded;
                                    if (isCollapsing && this.recentScrollRef) {
                                        this.recentScrollRef.setScrollPosition(0);
                                    }
                                }}
                            >
                                {this.expanded ? this.collapseText : this.viewAllText}
                                <SvgIcon
                                    icon="icon_arrow"
                                    size={[12, 12]}
                                    class={["writing-assistant__arrow-icon", { "is-expanded": this.expanded }]}
                                />
                            </span>
                        )}
                    </div>

                    {/* 文档卡片折叠区改为 ScrollBar 封装滚动 */}
                    <ScrollBar
                        ref="recentScrollRef"
                        class="writing-assistant__recent-scroll"
                        style={{ height: `${this.expanded ? this.expandedHeight : ROW_HEIGHT}px` }}
                        momentum
                        edgeBounce
                    >
                        <div ref="recentGridRef" class="writing-assistant__recent-grid">
                            {this.docs.map((doc, index) => {
                                const { marginLeft, marginRight } = getCardMargin(
                                    index,
                                    this.columns,
                                    this.docs.length,
                                );

                                return (
                                    <div
                                        key={doc.id}
                                        class="writing-assistant__doc-card"
                                        style={{
                                            width: `${this.cardWidth}px`,
                                            minWidth: `${MIN_CARD_WIDTH}px`,
                                            marginLeft,
                                            marginRight,
                                            marginBottom: "10px",
                                        }}
                                        onClick={() => this.handleDocClick(doc)}
                                    >
                                        <div class="writing-assistant__doc-icon">
                                            {this.docIconUrl ? (
                                                <img src={this.docIconUrl} class="writing-assistant__doc-icon-img" />
                                            ) : (
                                                <SvgIcon icon="doccard_text" />
                                            )}
                                        </div>
                                        <div class="writing-assistant__doc-info">
                                            <span class="writing-assistant__doc-name">{doc.name}</span>
                                            <span class="writing-assistant__doc-time">{doc.updated_at}</span>
                                        </div>
                                    </div>
                                );
                            })}
                        </div>
                    </ScrollBar>
                </div>

                {/* ===== 写作模板模块 ===== 暂时屏蔽掉，非p0需求 */}
                {false && this.filteredTemplates.length > 0 && (
                    <div class="writing-assistant__templates">
                        {/* 分类 Tab 行 */}
                        <div class="writing-assistant__templates-tabs">
                            {this.TEMPLATE_TABS.map((tab) => (
                                <span
                                    key={tab.key}
                                    class={`writing-assistant__templates-tab ${this.activeCategory === tab.key ? "is-active" : ""}`}
                                    onClick={() => this.handleTabClick(tab.key)}
                                >
                                    {tab.label}
                                </span>
                            ))}
                        </div>

                        {/* 模板卡片 Grid 布局（高度变化时有平滑动画） */}
                        <TemplatesGrid templates={this.filteredTemplates} onCardClick={this.handleTemplateClick} />
                    </div>
                )}
            </div>
        );
    },
});
