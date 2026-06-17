import { computed, defineComponent, nextTick, onBeforeUnmount, onMounted, ref, watch } from "vue";
import type { PropType } from "vue";
import ScrollBar from "@/components/ScrollBar";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";
import type { FileChangeApproveData } from "@/types/conversation";
import { InteractiveCompStatus } from "@/types/conversation";
import { useBackendStore } from "@/stores/backend";

type ChangeKind = "created" | "modified" | "deleted";
type FileApproveTextKind = "name" | "path";
type FileChangeItem = FileChangeApproveData["changes"][number];
type VirtualFileChangeItem = {
    change: FileChangeItem;
    index: number;
};

// Keep in sync with .file-approve__item; fixed row height keeps virtualization cheap.
const VIRTUAL_ITEM_HEIGHT = 30;
const VIRTUAL_LIST_PADDING_TOP = 6;
const VIRTUAL_LIST_PADDING_BOTTOM = 6;
const VIRTUAL_OVERSCAN_COUNT = 4;
const VIRTUAL_VIEWPORT_FALLBACK_HEIGHT = 180;

const CHANGE_KIND_META: Record<ChangeKind, { label: string; className: string }> = {
    created: {
        label: "Added",
        className: "file-approve__change-kind--created",
    },
    modified: {
        label: "Modified",
        className: "file-approve__change-kind--modified",
    },
    deleted: {
        label: "Deleted",
        className: "file-approve__change-kind--deleted",
    },
};

const normalizeChangeKind = (kind: string): ChangeKind => {
    const normalizedKind = kind.trim().toLowerCase();

    if (normalizedKind === "created" || normalizedKind === "modified" || normalizedKind === "deleted") {
        return normalizedKind;
    }

    return "modified";
};

const getFileNameFromPath = (filePath: string): string => {
    if (!filePath) {
        return "";
    }

    const normalizedPath = filePath.replace(/[\\/]+$/, "");
    const parts = normalizedPath.split(/[\\/]/).filter(Boolean);

    if (parts.length === 0) {
        return filePath;
    }

    return parts[parts.length - 1];
};

export default defineComponent({
    name: "FileChangeApprove",

    components: {
        ScrollBar,
        SvgIcon,
    },

    props: {
        data: {
            type: Object as PropType<FileChangeApproveData>,
            required: true,
        },
        onSubmit: {
            type: Function as PropType<(action: Record<string, unknown>) => void>,
            required: true,
        },
    },

    setup(props) {
        const backendStore = useBackendStore();
        const fileListScrollBar = ref<InstanceType<typeof ScrollBar> | null>(null);
        const textOverflowState = ref<Record<string, boolean>>({});
        const scrollTop = ref(0);
        const viewportHeight = ref(VIRTUAL_VIEWPORT_FALLBACK_HEIGHT);
        const textElements = new Map<string, HTMLElement>();
        let resizeObserver: ResizeObserver | null = null;
        let viewportResizeObserver: ResizeObserver | null = null;
        let overflowUpdateFrame: number | null = null;

        const changeSummary = computed(() => {
            const summary = {
                total: props.data.changes.length,
                created: 0,
                modified: 0,
                deleted: 0,
            };

            props.data.changes.forEach((change) => {
                summary[normalizeChangeKind(change.kind)] += 1;
            });

            return summary;
        });

        const changeCount = computed(() => changeSummary.value.total);

        const headerTitle = computed(() =>
            backendStore
                .translate("%1 file changes (%2 added, %3 modified, %4 deleted)")
                .replace("%1", String(changeCount.value))
                .replace("%2", String(changeSummary.value.created))
                .replace("%3", String(changeSummary.value.modified))
                .replace("%4", String(changeSummary.value.deleted)),
        );

        const applyText = computed(() => backendStore.translate("Apply"));
        const rejectText = computed(() => backendStore.translate("Reject"));
        const approvedText = computed(() => backendStore.translate("Applied"));
        const rejectedText = computed(() => backendStore.translate("Rejected"));

        const isPending = computed(() => props.data.status === InteractiveCompStatus.PENDING);
        const statusText = computed(() =>
            props.data.status === InteractiveCompStatus.APPROVED ? approvedText.value : rejectedText.value,
        );

        const getKindLabel = (kind: string) => {
            return backendStore.translate(CHANGE_KIND_META[normalizeChangeKind(kind)].label);
        };

        const getKindClass = (kind: string) => {
            return CHANGE_KIND_META[normalizeChangeKind(kind)].className;
        };

        const virtualListHeight = computed(() => {
            if (changeCount.value === 0) {
                return 0;
            }

            return VIRTUAL_LIST_PADDING_TOP + changeCount.value * VIRTUAL_ITEM_HEIGHT + VIRTUAL_LIST_PADDING_BOTTOM;
        });

        const virtualViewportHeight = computed(() =>
            Math.min(virtualListHeight.value, VIRTUAL_VIEWPORT_FALLBACK_HEIGHT),
        );

        const maxVirtualScrollTop = computed(() => Math.max(0, virtualListHeight.value - viewportHeight.value));

        const visibleStartIndex = computed(() => {
            const clampedScrollTop = Math.min(scrollTop.value, maxVirtualScrollTop.value);
            const firstVisibleIndex = Math.floor(
                Math.max(0, clampedScrollTop - VIRTUAL_LIST_PADDING_TOP) / VIRTUAL_ITEM_HEIGHT,
            );

            return Math.max(0, firstVisibleIndex - VIRTUAL_OVERSCAN_COUNT);
        });

        const visibleEndIndex = computed(() => {
            const visibleCount = Math.ceil(viewportHeight.value / VIRTUAL_ITEM_HEIGHT);
            const overscannedCount = visibleCount + VIRTUAL_OVERSCAN_COUNT * 2 + 2;

            return Math.min(changeCount.value, visibleStartIndex.value + overscannedCount);
        });

        const visibleChanges = computed<VirtualFileChangeItem[]>(() =>
            props.data.changes.slice(visibleStartIndex.value, visibleEndIndex.value).map((change, offset) => ({
                change,
                index: visibleStartIndex.value + offset,
            })),
        );

        const virtualOffsetY = computed(() => VIRTUAL_LIST_PADDING_TOP + visibleStartIndex.value * VIRTUAL_ITEM_HEIGHT);

        const getTextKey = (kind: FileApproveTextKind, index: number) => {
            return `${kind}-${index}`;
        };

        const isElementOverflowing = (element: HTMLElement) => {
            return element.scrollWidth - element.clientWidth > 1;
        };

        const updateTextOverflowState = (key: string) => {
            const element = textElements.get(key);
            const isOverflowing = element ? isElementOverflowing(element) : false;

            if (textOverflowState.value[key] === isOverflowing) {
                return;
            }

            textOverflowState.value = {
                ...textOverflowState.value,
                [key]: isOverflowing,
            };
        };

        const updateAllTextOverflowStates = () => {
            const nextState: Record<string, boolean> = {};

            textElements.forEach((element, key) => {
                nextState[key] = isElementOverflowing(element);
            });

            const hasSameKeys =
                Object.keys(nextState).length === Object.keys(textOverflowState.value).length &&
                Object.keys(nextState).every((key) => textOverflowState.value[key] === nextState[key]);

            if (hasSameKeys) {
                return;
            }

            textOverflowState.value = nextState;
        };

        const scheduleTextOverflowUpdate = () => {
            if (overflowUpdateFrame !== null) {
                return;
            }

            overflowUpdateFrame = window.requestAnimationFrame(() => {
                overflowUpdateFrame = null;
                updateAllTextOverflowStates();
            });
        };

        const setTextRef = (key: string, element: Element | { $el?: Element } | null) => {
            const previousElement = textElements.get(key);
            const htmlElement = ((element as { $el?: Element } | null)?.$el ?? element) as HTMLElement | null;

            if (previousElement === htmlElement) {
                return;
            }

            if (previousElement) {
                resizeObserver?.unobserve(previousElement);
                textElements.delete(key);
            }

            if (!htmlElement) {
                return;
            }

            textElements.set(key, htmlElement);
            resizeObserver?.observe(htmlElement);
        };

        const isTextTooltipDisabled = (key: string, content: string) => {
            return !content || !textOverflowState.value[key];
        };

        const handleTextMouseEnter = (key: string) => {
            updateTextOverflowState(key);
        };

        const handleAccept = (event: MouseEvent) => {
            event.stopPropagation();
            props.onSubmit({
                request_id: props.data.id,
                type: props.data.ic_type,
                approve: true,
            });
        };

        const handleReject = (event: MouseEvent) => {
            event.stopPropagation();
            props.onSubmit({
                request_id: props.data.id,
                type: props.data.ic_type,
                approve: false,
            });
        };

        const handleFileListWheel = (event: WheelEvent) => {
            const container = fileListScrollBar.value?.scrollContainerRef;
            if (!container) {
                return;
            }

            const { scrollTop, scrollHeight, clientHeight } = container;
            const isAtTop = scrollTop <= 0;
            const isAtBottom = scrollTop + clientHeight >= scrollHeight - 1;

            if ((event.deltaY > 0 && !isAtBottom) || (event.deltaY < 0 && !isAtTop)) {
                event.stopPropagation();
            }
        };

        const syncVirtualViewportHeight = () => {
            const container = fileListScrollBar.value?.scrollContainerRef;
            const nextHeight = container?.clientHeight ?? VIRTUAL_VIEWPORT_FALLBACK_HEIGHT;
            viewportHeight.value = nextHeight > 0 ? nextHeight : VIRTUAL_VIEWPORT_FALLBACK_HEIGHT;
        };

        const syncVirtualScrollState = () => {
            const container = fileListScrollBar.value?.scrollContainerRef;
            scrollTop.value = container?.scrollTop ?? 0;
            syncVirtualViewportHeight();
        };

        const handleVirtualListScroll = (position: number) => {
            scrollTop.value = position;
            syncVirtualViewportHeight();

            nextTick(() => {
                scheduleTextOverflowUpdate();
            });
        };

        onMounted(() => {
            nextTick(() => {
                syncVirtualScrollState();

                const scrollContainer = fileListScrollBar.value?.scrollContainerRef;
                if (typeof ResizeObserver !== "undefined" && scrollContainer) {
                    viewportResizeObserver = new ResizeObserver(() => {
                        syncVirtualViewportHeight();
                        scheduleTextOverflowUpdate();
                    });
                    viewportResizeObserver.observe(scrollContainer);
                }

                if (typeof ResizeObserver !== "undefined") {
                    resizeObserver = new ResizeObserver(() => {
                        scheduleTextOverflowUpdate();
                    });
                    textElements.forEach((element) => resizeObserver?.observe(element));
                }

                updateAllTextOverflowStates();
            });
        });

        onBeforeUnmount(() => {
            resizeObserver?.disconnect();
            resizeObserver = null;
            viewportResizeObserver?.disconnect();
            viewportResizeObserver = null;

            if (overflowUpdateFrame !== null) {
                window.cancelAnimationFrame(overflowUpdateFrame);
                overflowUpdateFrame = null;
            }

            textElements.clear();
        });

        watch(
            () =>
                props.data.changes
                    .map((change) => `${change.path}\u0000${change.kind}\u0000${change.is_dir}`)
                    .join("\u0001"),
            () => {
                nextTick(() => {
                    syncVirtualScrollState();
                    scheduleTextOverflowUpdate();
                });
            },
        );

        return {
            fileListScrollBar,
            headerTitle,
            applyText,
            rejectText,
            isPending,
            statusText,
            visibleChanges,
            virtualListHeight,
            virtualViewportHeight,
            virtualOffsetY,
            getFileNameFromPath,
            getKindClass,
            getKindLabel,
            getTextKey,
            handleAccept,
            handleReject,
            handleFileListWheel,
            handleVirtualListScroll,
            handleTextMouseEnter,
            isTextTooltipDisabled,
            setTextRef,
        };
    },

    render() {
        return (
            <div class="file-approve">
                <div class="file-approve__header">
                    <div class="file-approve__header-info">
                        <span class="file-approve__header-icon">
                            <SvgIcon icon="bash-file" size={[16, 16]} />
                        </span>
                        <span class="file-approve__header-title">{this.headerTitle}</span>
                    </div>

                    <div class="file-approve__header-actions">
                        {this.isPending ? (
                            <>
                                <div class="file-approve__btn file-approve__btn--accept" onClick={this.handleAccept}>
                                    <span>{this.applyText}</span>
                                </div>
                                <div class="file-approve__btn file-approve__btn--reject" onClick={this.handleReject}>
                                    <span>{this.rejectText}</span>
                                </div>
                            </>
                        ) : (
                            <span class="file-approve__status">{this.statusText}</span>
                        )}
                    </div>
                </div>

                <div class="file-approve__list-shell" onWheel={this.handleFileListWheel}>
                    <ScrollBar
                        ref="fileListScrollBar"
                        class="file-approve__list-scroll"
                        style={{ height: `${this.virtualViewportHeight}px` }}
                        onScroll={this.handleVirtualListScroll}
                    >
                        <div
                            class="file-approve__list file-approve__list--virtual"
                            style={{ height: `${this.virtualListHeight}px` }}
                        >
                            <div
                                class="file-approve__virtual-items"
                                style={{ transform: `translateY(${this.virtualOffsetY}px)` }}
                            >
                                {this.visibleChanges.map(({ change, index }) => {
                                    const kindClass = this.getKindClass(change.kind);
                                    const kindLabel = this.getKindLabel(change.kind);
                                    const fileName = this.getFileNameFromPath(change.path);
                                    const fileNameKey = this.getTextKey("name", index);
                                    const filePathKey = this.getTextKey("path", index);

                                    return (
                                        <div key={`${change.path}-${index}`} class="file-approve__item">
                                            {change.is_dir && (
                                                <span class="file-approve__file-icon">
                                                    <SvgIcon icon="icon_open_dir" size={[16, 16]} />
                                                </span>
                                            )}
                                            <Tooltip
                                                content={fileName}
                                                placement="top"
                                                disabled={this.isTextTooltipDisabled(fileNameKey, fileName)}
                                                popperClass="file-approve__text-tooltip"
                                            >
                                                <span
                                                    ref={(element) => this.setTextRef(fileNameKey, element)}
                                                    class="file-approve__file-name"
                                                    onMouseenter={() => this.handleTextMouseEnter(fileNameKey)}
                                                >
                                                    {fileName}
                                                </span>
                                            </Tooltip>
                                            <Tooltip
                                                content={change.path}
                                                placement="top"
                                                disabled={this.isTextTooltipDisabled(filePathKey, change.path)}
                                                popperClass="file-approve__text-tooltip"
                                            >
                                                <span
                                                    ref={(element) => this.setTextRef(filePathKey, element)}
                                                    class="file-approve__file-path"
                                                    onMouseenter={() => this.handleTextMouseEnter(filePathKey)}
                                                >
                                                    {change.path}
                                                </span>
                                            </Tooltip>
                                            <span class={["file-approve__change-kind", kindClass]}>{kindLabel}</span>
                                        </div>
                                    );
                                })}
                            </div>
                        </div>
                    </ScrollBar>
                </div>
            </div>
        );
    },
});
