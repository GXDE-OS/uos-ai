import { defineComponent, ref, onMounted, onBeforeUnmount, inject, nextTick, computed, Teleport } from "vue";
import type { PropType } from "vue";
import type { DocCardData } from "@/types/conversation";
import { EXTENSION_PANEL_KEY } from "@/types/extension-panel";
import type { MenuItem } from "@/types/menu";
import SvgIcon from "@/components/SvgIcon";
import Menu from "@/components/menu/Menu";
import MarkdownEditor from "@/views/window/mainwindow/page/chat/components/markdownEditor/MarkdownEditor";
import { useConversationManagerStore, useBackendStore } from "@/stores";

/** 导出格式类型 */
export type ExportFormat = "pdf" | "markdown" | "word";
type ExportStatus = "idle" | "saving" | "success" | "failed";

const MIN_SAVING_DISPLAY_MS = 500;
const EXPORT_RESULT_DISPLAY_MS = 3000;

/** 统一创建时间显示格式 */
function formatCreatedTime(timestampMs: number): string {
    const date = new Date(timestampMs);
    const pad2 = (value: number) => String(value).padStart(2, "0");
    const year = date.getFullYear();
    const month = pad2(date.getMonth() + 1);
    const day = pad2(date.getDate());
    const hour = pad2(date.getHours());
    const minute = pad2(date.getMinutes());
    const backend = useBackendStore(); // Ensure backend store is available for translation
    return `${backend.translate("Created")} ${month}-${day} ${hour}:${minute}`;
}

/** 兼容 number/string 类型的时间输入
 * 返回 0 表示无效/未加载时间，正数表示有效的时间戳
 */
function normalizeCreatedAt(raw: number | string | null | undefined): number {
    if (typeof raw === "number" && Number.isFinite(raw)) {
        const normalized = raw > 1e12 ? raw : raw * 1000;
        return normalized > 0 ? normalized : 0;
    }
    if (typeof raw === "string" && raw.trim() !== "") {
        const numeric = Number(raw);
        if (Number.isFinite(numeric) && numeric > 0) {
            return numeric > 1e12 ? numeric : numeric * 1000;
        }
        const parsed = Date.parse(raw);
        if (Number.isFinite(parsed) && parsed > 0) {
            return parsed;
        }
    }
    // Return 0 for invalid/missing input (not Date.now(), so we can distinguish "not loaded")
    return 0;
}

/**
 * 文档卡片组件
 * 布局：左侧文件信息（图标 + 文档名 + 生成时间），右侧导出按钮
 */
export default defineComponent({
    name: "DocCard",

    components: { Menu, SvgIcon },

    props: {
        data: {
            type: Object as PropType<DocCardData>,
            required: true,
        },
        /** 会话 ID */
        conversationId: {
            type: String,
            default: "",
        },
        /** 消息 ID */
        messageId: {
            type: String,
            default: "",
        },
        /** 是否为流式新生成的卡片，自动打开编辑器（历史恢复时不携带此标记） */
        isNew: {
            type: Boolean,
            default: false,
        },
        /** 可选：创建时间（毫秒/秒时间戳或可解析日期字符串） */
        createdAt: {
            type: [Number, String] as PropType<number | string>,
            default: "",
        },
    },

    emits: {
        /** 点击卡片主区域时发出，后续留用 */
        cardClick: (data: DocCardData) => !!data,
        /** 打开编辑器后发出，用于清理一次性标记 */
        cardOpened: () => true,
    },

    setup(props, { emit }) {
        const menuVisible = ref(false);
        const actionsRef = ref<HTMLElement | null>(null);
        const conversationManagerStore = useConversationManagerStore();
        const backend = useBackendStore();
        const docConversionSupported = ref(false);
        const createdAtMs = ref<number>(normalizeCreatedAt(props.createdAt));
        const exportStatus = ref<ExportStatus>("idle");
        const statusResetTimer = ref<ReturnType<typeof setTimeout> | null>(null);
        const exportRequestToken = ref(0);

        const extensionPanelAPI = inject(EXTENSION_PANEL_KEY);

        // 国际化文案
        const untitledDocText = computed(() => backend.translate("Untitled Document"));
        const exportDocText = computed(() => backend.translate("Export Document"));
        const savingText = computed(() => backend.translate("Saving..."));
        const saveSuccessText = computed(() => backend.translate("Saved successfully!"));
        const saveFailedText = computed(() => backend.translate("Failed to save, please try again."));

        const statusText = computed(() => {
            if (exportStatus.value === "saving") return savingText.value;
            if (exportStatus.value === "success") return saveSuccessText.value;
            if (exportStatus.value === "failed") return saveFailedText.value;
            // Show placeholder text if creation time is not loaded yet
            if (createdAtMs.value === 0) return backend.translate("Loading...") || "";
            return formatCreatedTime(createdAtMs.value);
        });

        const statusIcon = computed(() => {
            if (exportStatus.value === "saving") return "loading";
            if (exportStatus.value === "success") return "toast-done";
            if (exportStatus.value === "failed") return "toast-warning";
            return "";
        });

        const shouldShowStatusIcon = computed(() => exportStatus.value !== "idle");
        const isSavingStatus = computed(() => exportStatus.value === "saving");

        const statusClass = computed(() => {
            return {
                "doc-card__status--success": exportStatus.value === "success",
                "doc-card__status--failed": exportStatus.value === "failed",
            };
        });

        const clearStatusResetTimer = () => {
            if (statusResetTimer.value) {
                clearTimeout(statusResetTimer.value);
                statusResetTimer.value = null;
            }
        };

        const scheduleStatusReset = () => {
            clearStatusResetTimer();
            statusResetTimer.value = setTimeout(() => {
                exportStatus.value = "idle";
                statusResetTimer.value = null;
            }, EXPORT_RESULT_DISPLAY_MS);
        };

        /** 导出菜单项 */
        const exportMenuItems = computed<MenuItem[]>(() => {
            const items: MenuItem[] = [
                {
                    type: "item",
                    id: "export-markdown",
                    label: backend.translate("Save as Markdown"),
                    themeIcon: "text-markdown",
                },
            ];
            if (docConversionSupported.value) {
                items.unshift({
                    type: "item",
                    id: "export-pdf",
                    label: backend.translate("Save as PDF"),
                    themeIcon: "application-pdf",
                });
                items.push({
                    type: "item",
                    id: "export-word",
                    label: backend.translate("Save as Word"),
                    themeIcon: "application-wps-office.docx",
                });
            }
            return items;
        });

        /** 点击 actions 容器外部时关闭菜单 */
        const handleDocumentClick = (e: MouseEvent) => {
                if (menuVisible.value && actionsRef.value && !actionsRef.value.contains(e.target as Node)) {
                    menuVisible.value = false;
                }
        };

        /** 鼠标滚动时关闭菜单 */
        const handleWheel = () => {
            menuVisible.value = false;
        };

        /** 处理菜单项选择 */
        const handleMenuItemSelect = (item: MenuItem) => {
            // 根据菜单项 id 判断导出格式
            const formatMap: Record<string, ExportFormat> = {
                "export-pdf": "pdf",
                "export-markdown": "markdown",
                "export-word": "word",
            };
            const format = formatMap[item.id || ""];
            if (format) {
                handleExport(format);
            }
        };

        onMounted(async () => {
            document.addEventListener("click", handleDocumentClick, true);
            document.addEventListener("wheel", handleWheel);

            // If creation time is not available from history, try to load it from article
            if (createdAtMs.value === 0) {
                try {
                    const article = await conversationManagerStore.getWorkspaceArticle(
                        props.conversationId,
                        props.data.id,
                        props.data.version,
                    );
                    if (article?.created_at) {
                        const parsed = Date.parse(article.created_at);
                        if (Number.isFinite(parsed) && parsed > 0) {
                            createdAtMs.value = parsed;
                        }
                    }
                } catch (e) {
                    // Silently fail - the card will show placeholder text
                    console.warn("[DocCard] Failed to load article creation time:", e);
                }
            }

            // 流式新生成的 docCard（isNew=true）直接自动打开编辑器；历史恢复时 isNew 为 false
            if (props.isNew) {
                nextTick(() => handleCardClick());
            }
        });

        onBeforeUnmount(() => {
            document.removeEventListener("click", handleDocumentClick, true);
            document.removeEventListener("wheel", handleWheel);
            clearStatusResetTimer();
        });

        /** 导出按钮点击（阻止事件冒泡，避免触发 cardClick） */
        const handleExportBtnClick = async (e: MouseEvent) => {
            e.stopPropagation();
            if (!menuVisible.value) {
                docConversionSupported.value = await backend.requestServiceConfig("checkDocumentConversionCapability");
            }
            menuVisible.value = !menuVisible.value;
        };

        /** 阻止 actions 区域的事件冒泡 */
        const handleActionsClick = (e: MouseEvent) => {
            e.stopPropagation();
        };

        /** 处理导出选项 */
        const handleExport = async (format: ExportFormat) => {
            menuVisible.value = false;

            exportRequestToken.value += 1;
            const requestToken = exportRequestToken.value;
            clearStatusResetTimer();
            exportStatus.value = "saving";

            const startMs = Date.now();
            const success = await conversationManagerStore.exportWorkspaceArticle(props.conversationId, props.data.id, format);
            const elapsedMs = Date.now() - startMs;
            if (elapsedMs < MIN_SAVING_DISPLAY_MS) {
                await new Promise((resolve) => setTimeout(resolve, MIN_SAVING_DISPLAY_MS - elapsedMs));
            }

            // 若用户期间触发了新的导出请求，忽略旧请求结果
            if (requestToken !== exportRequestToken.value) {
                return;
            }

            exportStatus.value = success ? "success" : "failed";
            scheduleStatusReset();
        };

        /** 点击卡片主体 - 从 workspace 加载文章后打开编辑器 */
        const handleCardClick = async () => {
            emit("cardClick", props.data);

            if (!extensionPanelAPI) {
                console.warn("[DocCard] ExtensionPanel API not available.");
                return;
            }

            const articleId = props.data.id;
            const article = await conversationManagerStore.getWorkspaceArticle(
                props.conversationId,
                articleId,
                props.data.version,
            );

            if (!article) {
                console.error("[DocCard] Failed to load article:", articleId);
                return;
            }

            extensionPanelAPI.openExtensionPanel(() => (
                <MarkdownEditor
                    key={articleId}
                    content={article.content}
                    articleId={articleId}
                    title={article.title}
                    conversationId={props.conversationId}
                    updatedAt={article.updated_at}
                    references={article.references ?? []}
                />
            ));

            emit("cardOpened");
        };

        return {
            menuVisible,
            actionsRef,
            handleExportBtnClick,
            handleActionsClick,
            handleMenuItemSelect,
            handleCardClick,
            exportMenuItems,
            statusText,
            statusIcon,
            shouldShowStatusIcon,
            isSavingStatus,
            statusClass,
            // 暴露给 render 函数使用的更新方法
            setMenuVisible: (visible: boolean) => { menuVisible.value = visible; },
            // 国际化
            untitledDocText,
            exportDocText,
        };
    },

    render() {
        return (
            <div class="doc-card" onClick={this.handleCardClick}>
                {/* 左侧：文件信息区 */}
                <div class="doc-card__info">
                    {/* 上部：图标 + 文档名 */}
                    <div class="doc-card__title-row">
                        <SvgIcon icon="doc-generated" class="doc-card__icon" size={[16, 16]} />
                        <span class="doc-card__title">{this.$props.data.title || this.untitledDocText}</span>
                    </div>
                    {/* 下部：状态（创建时间、导出结果等） */}
                    <div class={["doc-card__status", this.statusClass]}>
                        {this.shouldShowStatusIcon && (
                            <span class={["doc-card__status-icon-wrap", { "is-spinning": this.isSavingStatus }]}>
                                <SvgIcon icon={this.statusIcon} class="doc-card__status-icon" size={[16, 16]} />
                            </span>
                        )}
                        <span class="doc-card__status-text">{this.statusText}</span>
                    </div>
                </div>

                {/* 右侧：导出按钮 + 下拉菜单 */}
                <div class="doc-card__actions" ref="actionsRef" onClick={this.handleActionsClick}>
                    <div class="doc-card__export-btn" onClick={this.handleExportBtnClick} title={this.exportDocText}>
                        <SvgIcon icon="mdeditor-save-as" size={[16, 16]} />
                    </div>
                </div>

                {/* 菜单传送到 body 层级，避免被父容器裁剪 */}
                <Teleport to="body">
                    <Menu
                        visible={this.menuVisible}
                        items={this.exportMenuItems}
                        triggerRef={this.actionsRef}
                        placement="bottom"
                        onSelectItem={this.handleMenuItemSelect}
                        onUpdateVisible={this.setMenuVisible}
                    />
                </Teleport>
            </div>
        );
    },
});
