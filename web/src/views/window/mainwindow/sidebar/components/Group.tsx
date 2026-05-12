import { defineComponent, ref, computed, h, onBeforeUnmount, TransitionGroup, Teleport, watch } from "vue";
import type { PropType } from "vue";
import type { SidebarGroup, SidebarItem } from "@/views/window/mainwindow/sidebar/types";
import ExpandableContainer from "@/views/window/mainwindow/sidebar/components/ExpandableContainer";
import GroupHeader from "@/views/window/mainwindow/sidebar/components/GroupHeader";
import ToggleExpand from "@/views/window/mainwindow/sidebar/components/ToggleExpand";
import BaseItem from "@/views/window/mainwindow/sidebar/components/BaseItem";
import "@/assets/styles/window/mainwindow/sidebar/components/Group.css";

type RenderToken = { kind: "item"; item: SidebarItem; ghost?: boolean };
// 拖拽预览需要把“展开/收起”按钮也当作一个占位 token，才能在展开态下统一计算排序位置。
type DragPreviewToken =
    | { kind: "item"; item: SidebarItem; ghost?: boolean; hidden?: boolean }
    | { kind: "toggle"; key: string; hiddenCount: number };
type DropArea = "visible" | "hidden";

type DragState = {
    itemId: string;
    startClientY: number;
    originMovableIndex: number;
    originItemIndex: number;
    originPersistentVisibleCount: number;
    originDisplayVisibleCount: number;
    originIsPersistentlyVisible: boolean;
    startClientTop: number;
    fixedClientLeft: number;
    width: number;
};

const TEMPORARY_VISIBLE_ITEM_KEY = "__sidebar-temporary-visible-assistant__";
const TEMPORARY_HIDDEN_ITEM_KEY_PREFIX = "__sidebar-temporary-hidden-assistant__";

export default defineComponent({
    name: "Group",
    components: {
        BaseItem,
    },
    props: {
        group: {
            type: Object as PropType<SidebarGroup>,
            required: true,
        },
        headerOrder: {
            type: Number,
            required: false,
            default: 0,
        },
        headerDomId: {
            type: String,
            required: false,
        },
        headerHidden: {
            type: Boolean,
            required: false,
            default: false,
        },
    },
    emits: ["item-click", "reorder", "rightButtonIconClick", "rightButtonClick"],
    setup(props, { emit }) {
        const expanded = ref(false);
        const groupRef = ref<HTMLElement | null>(null);

        const dragState = ref<DragState | null>(null);
        const draggedItemId = ref<string | null>(null); // 鼠标移动超过阈值后才认为真正进入拖拽。
        const dropIndex = ref<number | null>(null); // 插入到 movableItemsWithoutDragged 的位置。
        const dropArea = ref<DropArea | null>(null);
        const sortableItemElements = ref<HTMLElement[]>([]);
        const dragOverlayRef = ref<HTMLElement | null>(null);
        // 临时插入项切换时关闭 TransitionGroup 动画，避免同一助手在常驻区和隐藏区之间闪动。
        const suppressReorderTransition = ref(false);
        let dragTranslateY = 0;
        let dragOverlayFrame: number | null = null;
        let pendingDropClientY: number | null = null;
        let dropIndexFrame: number | null = null;
        let transitionRestoreFrame: number | null = null;

        const clampVisibleCount = (count: number, maxCount = props.group.items.length) => {
            const normalizedCount = Number.isFinite(count) ? Math.trunc(count) : props.group.items.length;
            return Math.max(0, Math.min(normalizedCount, maxCount, props.group.items.length));
        };
        const defaultVisibleCount = computed(() => props.group.defaultVisibleCount ?? props.group.items.length);
        const maxVisibleCount = computed(() =>
            clampVisibleCount(props.group.maxVisibleCount ?? props.group.items.length),
        );
        const committedVisibleCount = computed(() => clampVisibleCount(defaultVisibleCount.value));
        const persistentVisibleCount = computed(() =>
            clampVisibleCount(props.group.persistentVisibleCount ?? committedVisibleCount.value, maxVisibleCount.value),
        );
        // 拖拽跨过常驻/隐藏边界时，只在预览态调整常驻数量，真正落库等 mouseup 后完成。
        const previewPersistentVisibleCount = computed(() => {
            const ds = dragState.value;
            if (!ds || !draggedItemId.value || !dropArea.value) {
                return persistentVisibleCount.value;
            }

            if (ds.originIsPersistentlyVisible && dropArea.value === "hidden") {
                return Math.max(0, ds.originPersistentVisibleCount - 1);
            }

            if (!ds.originIsPersistentlyVisible && dropArea.value === "visible") {
                return clampVisibleCount(ds.originPersistentVisibleCount + 1, maxVisibleCount.value);
            }

            return ds.originPersistentVisibleCount;
        });
        // 展示数量 = 当前渲染数量 + 预览中的常驻数量变化，用于实时展示跨区拖拽效果。
        const visibleCount = computed(() => {
            const ds = dragState.value;
            if (!ds || !draggedItemId.value) {
                return committedVisibleCount.value;
            }

            const visibleCountDelta = previewPersistentVisibleCount.value - ds.originPersistentVisibleCount;
            return clampVisibleCount(ds.originDisplayVisibleCount + visibleCountDelta);
        });
        const reorderable = computed(() => props.group.reorderable ?? false);
        const showHeader = computed(() => props.group.showHeader !== false);
        const temporarilyInsertedItemId = computed(
            () => props.group.items.find((item) => item.reorderDisabled)?.id ?? null,
        );
        // 未拖拽时给隐藏区使用槽位 key，避免临时露出的同一助手在两个区域复用同一个 key。
        const shouldUseHiddenSlotKeys = computed(() => !draggedItemId.value);

        // 临时露出的助手固定在展示槽位中，除非它本身正在被拖拽。
        const isFixedDuringDrag = (item: SidebarItem) => Boolean(item.reorderDisabled && item.id !== draggedItemId.value);

        const getMovableItems = (items: SidebarItem[]) => items.filter((item) => !isFixedDuringDrag(item));

        const getMovableItemIndex = (items: SidebarItem[], itemId: string) =>
            getMovableItems(items).findIndex((item) => item.id === itemId);

        const moveItemKeepingFixedSlots = (
            items: SidebarItem[],
            draggedId: string,
            insertionIndex: number,
        ): SidebarItem[] => {
            // 先抽出可移动项排序，再把固定项放回原槽位，避免预览态项目被写进真实顺序。
            const fixedItemsByIndex = new Map<number, SidebarItem>();
            const movableItems: SidebarItem[] = [];

            for (const [index, item] of items.entries()) {
                if (isFixedDuringDrag(item)) {
                    fixedItemsByIndex.set(index, item);
                    continue;
                }
                movableItems.push(item);
            }

            const draggedIndex = movableItems.findIndex((item) => item.id === draggedId);
            if (draggedIndex === -1) {
                return items;
            }

            const [draggedItem] = movableItems.splice(draggedIndex, 1);
            const nextInsertionIndex = Math.max(0, Math.min(insertionIndex, movableItems.length));
            movableItems.splice(nextInsertionIndex, 0, draggedItem);

            let movableIndex = 0;
            return items.map((item, index) => {
                const fixedItem = fixedItemsByIndex.get(index);
                if (fixedItem) {
                    return fixedItem;
                }

                const movableItem = movableItems[movableIndex];
                movableIndex += 1;
                return movableItem ?? item;
            });
        };

        const previewItems = computed(() => {
            if (!draggedItemId.value) {
                return props.group.items;
            }

            const draggedIndex = getMovableItemIndex(props.group.items, draggedItemId.value);
            if (draggedIndex === -1) {
                return props.group.items;
            }

            const nextInsertionIndex = Math.max(
                0,
                Math.min(dropIndex.value ?? draggedIndex, movableItemsWithoutDragged.value.length),
            );
            return moveItemKeepingFixedSlots(props.group.items, draggedItemId.value, nextInsertionIndex);
        });

        const movableItemsWithoutDragged = computed(() => {
            if (!draggedItemId.value) {
                return getMovableItems(props.group.items);
            }
            return getMovableItems(props.group.items).filter((item) => item.id !== draggedItemId.value);
        });

        const normalizedDropIndex = computed(() => {
            if (!draggedItemId.value) {
                return null;
            }
            const fallbackIndex = getMovableItemIndex(props.group.items, draggedItemId.value);
            const nextIndex = dropIndex.value ?? fallbackIndex;
            return Math.max(0, Math.min(nextIndex, movableItemsWithoutDragged.value.length));
        });

        const renderTokens = computed((): RenderToken[] => {
            return previewItems.value.map((item) => ({
                kind: "item",
                item,
                ghost: item.id === draggedItemId.value,
            }));
        });

        // 临时露出的助手始终拼在常驻区末尾，隐藏区只切分真实可排序项。
        const temporaryVisibleToken = computed(() =>
            renderTokens.value.find((token) => token.item.reorderDisabled) ?? null,
        );
        const movableRenderTokens = computed(() => renderTokens.value.filter((token) => !token.item.reorderDisabled));
        const movableVisibleCount = computed(() =>
            Math.max(0, visibleCount.value - (temporaryVisibleToken.value ? 1 : 0)),
        );
        const visibleTokens = computed(() => {
            const tokens = movableRenderTokens.value.slice(0, movableVisibleCount.value);
            return temporaryVisibleToken.value ? [...tokens, temporaryVisibleToken.value] : tokens;
        });
        const hiddenTokens = computed(() => movableRenderTokens.value.slice(movableVisibleCount.value));
        const showToggleExpand = computed(() => hiddenTokens.value.length > 0);
        const shouldUseUnifiedDragPreview = computed(
            () => Boolean(draggedItemId.value) && expanded.value && showToggleExpand.value,
        );
        // 展开态拖拽时用一份统一列表渲染常驻区、展开按钮和隐藏区，便于用 DOM 中点计算落点。
        const dragPreviewTokens = computed((): DragPreviewToken[] => {
            if (!showToggleExpand.value) {
                return renderTokens.value.map((token) => ({
                    kind: "item" as const,
                    item: token.item,
                    ghost: token.ghost,
                    hidden: false,
                }));
            }

            return [
                ...visibleTokens.value.map((token) => ({
                    kind: "item" as const,
                    item: token.item,
                    ghost: token.ghost,
                    hidden: false,
                })),
                {
                    kind: "toggle" as const,
                    key: "__sidebar-toggle-expand__",
                    hiddenCount: hiddenTokens.value.length,
                },
                ...hiddenTokens.value.map((token) => ({
                    kind: "item" as const,
                    item: token.item,
                    ghost: token.ghost,
                    hidden: true,
                })),
            ];
        });

        const handleItemClick = (item: SidebarItem) => {
            emit("item-click", item);
        };

        const handleToggle = () => {
            expanded.value = !expanded.value;
        };

        const clampDropIndex = (index: number) =>
            Math.max(0, Math.min(index, movableItemsWithoutDragged.value.length));

        const collectRenderedSortableItemElements = () => {
            const groupElement = groupRef.value;
            if (!groupElement) {
                return [];
            }

            // 只收集真正可排序的 DOM 节点，临时展示项和当前拖拽项不参与落点计算。
            return Array.from(groupElement.querySelectorAll<HTMLElement>("[data-sidebar-sortable-item='true']")).filter(
                (element) => element.dataset.itemId !== draggedItemId.value,
            );
        };

        watch(
            [draggedItemId, dropIndex, dropArea, expanded, visibleCount],
            () => {
                if (!draggedItemId.value) {
                    sortableItemElements.value = [];
                    return;
                }

                sortableItemElements.value = collectRenderedSortableItemElements();
            },
            { flush: "post" },
        );

        const scheduleReorderTransitionRestore = () => {
            if (transitionRestoreFrame !== null) {
                cancelAnimationFrame(transitionRestoreFrame);
            }

            // 等 Vue 完成两帧布局更新后再恢复动画，避免过早开启导致列表跳动。
            transitionRestoreFrame = requestAnimationFrame(() => {
                transitionRestoreFrame = requestAnimationFrame(() => {
                    transitionRestoreFrame = null;
                    suppressReorderTransition.value = false;
                });
            });
        };

        watch(temporarilyInsertedItemId, (nextId, prevId) => {
            if (nextId === prevId) {
                return;
            }

            suppressReorderTransition.value = true;
            scheduleReorderTransitionRestore();
        });

        const computeInsertionIndexFromClientY = (clientY: number): number => {
            const itemElements = sortableItemElements.value.length
                ? sortableItemElements.value
                : collectRenderedSortableItemElements();
            if (itemElements.length === 0) {
                const ds = dragState.value;
                return Math.max(0, Math.min(ds?.originMovableIndex ?? 0, movableItemsWithoutDragged.value.length));
            }

            for (const [index, element] of itemElements.entries()) {
                const rect = element.getBoundingClientRect();
                // 鼠标位于某一项上半区时插到该项前面，否则继续向后查找。
                if (clientY < rect.top + rect.height / 2) {
                    return index;
                }
            }

            return itemElements.length;
        };

        const computeDropAreaFromClientY = (clientY: number, insertionIndex: number): DropArea => {
            const groupElement = groupRef.value;
            const toggleElement = groupElement?.querySelector<HTMLElement>(".group__drag-toggle-row");
            if (toggleElement) {
                // 展开态下，展开/收起按钮就是常驻区和隐藏区的视觉分界线。
                return clientY >= toggleElement.getBoundingClientRect().top ? "hidden" : "visible";
            }

            const ds = dragState.value;
            const boundary = ds?.originPersistentVisibleCount ?? persistentVisibleCount.value;
            if (ds?.originIsPersistentlyVisible) {
                // 常驻项拖到原边界之后，预览为移入隐藏区。
                return insertionIndex >= boundary ? "hidden" : "visible";
            }

            // 隐藏项拖到边界之前或边界处，预览为移入常驻区。
            return insertionIndex <= boundary ? "visible" : "hidden";
        };

        const cleanupPointerListeners = () => {
            window.removeEventListener("mousemove", handleWindowPointerMove as any);
            window.removeEventListener("mouseup", handleWindowPointerUp as any);
        };

        const updateDragOverlayPosition = () => {
            const ds = dragState.value;
            const overlayElement = dragOverlayRef.value;
            if (!ds || !overlayElement) {
                return;
            }

            const top = ds.startClientTop + dragTranslateY;
            overlayElement.style.transform = `translate3d(${ds.fixedClientLeft}px, ${top}px, 0)`;
        };

        const scheduleDragOverlayPositionUpdate = () => {
            if (dragOverlayFrame !== null) {
                return;
            }

            // 鼠标移动很频繁，浮层位置合并到下一帧更新，减少布局压力。
            dragOverlayFrame = requestAnimationFrame(() => {
                dragOverlayFrame = null;
                updateDragOverlayPosition();
            });
        };

        const cancelDropIndexUpdate = () => {
            if (dropIndexFrame !== null) {
                cancelAnimationFrame(dropIndexFrame);
                dropIndexFrame = null;
            }
            pendingDropClientY = null;
        };

        const applyDropStateFromClientY = (clientY: number) => {
            const nextIndex = clampDropIndex(computeInsertionIndexFromClientY(clientY));
            const nextDropArea = computeDropAreaFromClientY(clientY, nextIndex);
            if (nextIndex !== dropIndex.value) {
                dropIndex.value = nextIndex;
            }
            if (nextDropArea !== dropArea.value) {
                dropArea.value = nextDropArea;
            }
        };

        const scheduleDropIndexUpdate = (clientY: number) => {
            if (!draggedItemId.value) {
                return;
            }

            pendingDropClientY = clientY;
            if (dropIndexFrame !== null) {
                return;
            }

            // 落点计算依赖多个 DOM rect，同样合并到动画帧内执行。
            dropIndexFrame = requestAnimationFrame(() => {
                dropIndexFrame = null;
                if (pendingDropClientY !== null) {
                    applyDropStateFromClientY(pendingDropClientY);
                    pendingDropClientY = null;
                }
            });
        };

        const resetDrag = () => {
            if (dragOverlayFrame !== null) {
                cancelAnimationFrame(dragOverlayFrame);
                dragOverlayFrame = null;
            }
            cancelDropIndexUpdate();
            sortableItemElements.value = [];
            dragOverlayRef.value = null;
            dragTranslateY = 0;
            dragState.value = null;
            draggedItemId.value = null;
            dropIndex.value = null;
            dropArea.value = null;
        };

        const finalizeDrop = () => {
            if (!draggedItemId.value || normalizedDropIndex.value === null) {
                return;
            }
            const reorderedItems = moveItemKeepingFixedSlots(
                props.group.items,
                draggedItemId.value,
                normalizedDropIndex.value,
            );
            const payload: { groupId: string; newItems: string[]; visibleCount?: number } = {
                groupId: props.group.id,
                newItems: reorderedItems.map((item) => item.id),
            };
            if (
                props.group.persistentVisibleCount !== undefined &&
                previewPersistentVisibleCount.value !== persistentVisibleCount.value
            ) {
                payload.visibleCount = previewPersistentVisibleCount.value;
            }
            emit("reorder", payload);
        };

        const handleWindowPointerMove = (e: MouseEvent) => {
            const ds = dragState.value;
            if (!ds) {
                return;
            }
            e.preventDefault();

            const deltaY = e.clientY - ds.startClientY;
            dragTranslateY = deltaY; // 侧边栏只支持纵向排序，因此浮层只跟随 Y 轴移动。
            scheduleDragOverlayPositionUpdate();

            // 超过很小的移动阈值后才激活拖拽，避免普通点击被误判为排序。
            if (!draggedItemId.value) {
                if (Math.abs(deltaY) < 3) {
                    return;
                }
                draggedItemId.value = ds.itemId;
                dropIndex.value = ds.originMovableIndex;
                dropArea.value = ds.originIsPersistentlyVisible ? "visible" : "hidden";
            }

            scheduleDropIndexUpdate(e.clientY);
        };

        const handleWindowPointerUp = (e: MouseEvent) => {
            const ds = dragState.value;
            if (!ds) {
                return;
            }
            e.preventDefault();
            cleanupPointerListeners();

            const wasDragging = Boolean(draggedItemId.value);
            if (wasDragging) {
                cancelDropIndexUpdate();
                applyDropStateFromClientY(e.clientY);
                finalizeDrop();
                resetDrag();
                return;
            }

            // 如果从未超过拖拽阈值，则按普通点击处理。
            const clickedItem = props.group.items.find((it) => it.id === ds.itemId);
            resetDrag();
            if (clickedItem) {
                handleItemClick(clickedItem);
            }
        };

        const handleItemPointerDown = (item: SidebarItem, e: MouseEvent) => {
            if (!reorderable.value) {
                return;
            }
            if (e.button !== 0) {
                return;
            }

            e.preventDefault();

            const targetEl = e.currentTarget as HTMLElement | null;
            if (!targetEl) {
                return;
            }

            const itemRect = targetEl.getBoundingClientRect();

            // 拖拽落点基于“可移动项”的索引，真实渲染索引仍用于判断是否来自常驻区。
            const originMovableIndex = getMovableItemIndex(props.group.items, item.id);
            if (originMovableIndex === -1) {
                return;
            }
            const originItemIndex = props.group.items.findIndex((it) => it.id === item.id);
            if (originItemIndex === -1) {
                return;
            }

            dragState.value = {
                itemId: item.id,
                startClientY: e.clientY,
                originMovableIndex,
                originItemIndex,
                originPersistentVisibleCount: persistentVisibleCount.value,
                originDisplayVisibleCount: committedVisibleCount.value,
                originIsPersistentlyVisible: originItemIndex < persistentVisibleCount.value,
                startClientTop: itemRect.top,
                fixedClientLeft: itemRect.left,
                width: itemRect.width,
            };

            cleanupPointerListeners();
            window.addEventListener("mousemove", handleWindowPointerMove as any, { passive: false });
            window.addEventListener("mouseup", handleWindowPointerUp as any, { passive: false });
        };

        const handleRightButtonIconClick = (params: Record<string, any>) => {
            emit("rightButtonIconClick", params);
        };

        const handleRightButtonClick = (params: { item: SidebarItem; event: MouseEvent }) => {
            emit("rightButtonClick", params);
        };

        onBeforeUnmount(() => {
            cleanupPointerListeners();
            if (transitionRestoreFrame !== null) {
                cancelAnimationFrame(transitionRestoreFrame);
                transitionRestoreFrame = null;
            }
            resetDrag();
        });

        const itemClickHandlers = computed(() => {
            const handlers = new Map<string, () => void>();
            for (const item of props.group.items) {
                handlers.set(item.id, () => handleItemClick(item));
            }
            return handlers;
        });

        const itemMousedownHandlers = computed(() => {
            const handlers = new Map<string, (e: MouseEvent) => void>();
            if (reorderable.value) {
                for (const item of props.group.items) {
                    // 被临时插到常驻区的助手只允许点击切换，不参与拖拽排序。
                    if (item.reorderDisabled) {
                        continue;
                    }
                    handlers.set(item.id, (e: MouseEvent) => handleItemPointerDown(item, e));
                }
            }
            return handlers;
        });

        const renderToken = (token: RenderToken, keyOverride?: string) => {
            const item = token.item;
            return h(BaseItem, {
                item,
                // 临时展示项在常驻区使用固定 key；隐藏区渲染时可由外层传入槽位 key 避免重复。
                key: keyOverride ?? (item.reorderDisabled ? TEMPORARY_VISIBLE_ITEM_KEY : item.id),
                draggable: false,
                dragging: false,
                ghost: token.ghost ?? false,
                suppressHover: Boolean(draggedItemId.value),
                "data-sidebar-sortable-item": item.reorderDisabled ? undefined : "true",
                "data-item-id": item.id,
                // 可排序项走 mousedown 拖拽；禁排项仍保留普通点击行为。
                onClick: reorderable.value && !item.reorderDisabled ? undefined : itemClickHandlers.value.get(item.id),
                onMousedown:
                    reorderable.value && !item.reorderDisabled ? itemMousedownHandlers.value.get(item.id) : undefined,
                onRightButtonClick: handleRightButtonClick,
            });
        };

        const draggedItem = computed(() => {
            if (!draggedItemId.value) {
                return null;
            }
            return props.group.items.find((it) => it.id === draggedItemId.value) ?? null;
        });

        const setGroupRef = (el: Element | { $el?: Element } | null) => {
            groupRef.value = ((el as { $el?: Element } | null)?.$el ?? el) as HTMLElement | null;
        };

        const setDragOverlayRef = (el: Element | { $el?: Element } | null) => {
            dragOverlayRef.value = ((el as { $el?: Element } | null)?.$el ?? el) as HTMLElement | null;
            updateDragOverlayPosition();
        };

        const dragOverlayStyle = computed(() => {
            const ds = dragState.value;
            if (!ds || !draggedItemId.value) {
                return null;
            }
            return {
                transform: `translate3d(${ds.fixedClientLeft}px, ${ds.startClientTop + dragTranslateY}px, 0)`,
                width: `${ds.width}px`,
            } as Record<string, string>;
        });

        const renderDragPreviewToken = (token: DragPreviewToken) => {
            if (token.kind === "toggle") {
                // 统一拖拽预览里的展开按钮只做视觉分界，不响应点击。
                return (
                    <div key={token.key} class="group__drag-toggle-row">
                        <ToggleExpand expanded={expanded.value} hiddenCount={token.hiddenCount} />
                    </div>
                );
            }

            return (
                <div
                    key={token.item.id}
                    class={["group__drag-preview-item", token.hidden && "group__drag-preview-item--hidden"]}
                >
                    {renderToken(token)}
                </div>
            );
        };

        return {
            expanded,
            visibleTokens,
            hiddenTokens,
            showToggleExpand,
            shouldUseUnifiedDragPreview,
            dragPreviewTokens,
            showHeader,
            reorderable,
            suppressReorderTransition,
            shouldUseHiddenSlotKeys,
            handleItemClick,
            handleToggle,
            handleRightButtonIconClick,
            handleRightButtonClick,
            renderToken,
            renderDragPreviewToken,
            draggedItem,
            dragOverlayStyle,
            setGroupRef,
            setDragOverlayRef,
        };
    },
    render() {
        const visibleChildren = this.visibleTokens.map((token) => this.renderToken(token));
        const hiddenChildrenFixed = this.hiddenTokens.map((token, index) =>
            this.renderToken(
                token,
                // 非拖拽态隐藏区用槽位 key，避免临时露出助手回到隐藏区时触发重复 key。
                this.shouldUseHiddenSlotKeys ? `${TEMPORARY_HIDDEN_ITEM_KEY_PREFIX}${index}` : undefined,
            ),
        );
        const dragPreviewChildren = this.dragPreviewTokens.map((token) => this.renderDragPreviewToken(token));
        // 在抑制动画时退回普通 div，避免 TransitionGroup 在临时插入项变化时补过渡。
        const renderReorderList = (className: string, children: any[]) =>
            this.suppressReorderTransition ? (
                <div class={className}>{children}</div>
            ) : (
                <TransitionGroup tag="div" name="sidebar-reorder" class={className}>
                    {children}
                </TransitionGroup>
            );
        const groupStyle = this.showHeader
            ? ({
                  "--group-sticky-order": String(this.headerOrder),
              } as Record<string, string>)
            : undefined;

        return (
            <div
                id={`group-${this.group.id}`}
                class={["group", this.suppressReorderTransition && "group--reorder-transition-suppressed"]}
                style={groupStyle}
                ref={this.setGroupRef}
            >
                {this.showHeader && (
                    <GroupHeader
                        groupName={this.group.name}
                        groupId={this.group.id}
                        {...(this.headerDomId && { headerDomId: this.headerDomId })}
                        hidden={this.headerHidden}
                        {...(this.group.rightButtonIcon && { rightButtonIcon: this.group.rightButtonIcon })}
                        {...(this.group.rightButtonTooltip && { rightButtonTooltip: this.group.rightButtonTooltip })}
                        onRightButtonIconClick={this.handleRightButtonIconClick}
                    />
                )}

                {this.shouldUseUnifiedDragPreview ? (
                    renderReorderList("group__drag-preview-list", dragPreviewChildren)
                ) : (
                    <>
                        {renderReorderList("group__items", visibleChildren)}

                        {this.showToggleExpand && (
                            <>
                                <ToggleExpand
                                    expanded={this.expanded}
                                    hiddenCount={this.hiddenTokens.length}
                                    onToggle={this.handleToggle}
                                />

                                <ExpandableContainer
                                    expanded={this.expanded}
                                    disableTransition={this.suppressReorderTransition}
                                >
                                    {renderReorderList("group__hidden-items", hiddenChildrenFixed)}
                                </ExpandableContainer>
                            </>
                        )}
                    </>
                )}

                {this.draggedItem && this.dragOverlayStyle && (
                    <Teleport to="body">
                        <div class="group__drag-overlay-item" style={this.dragOverlayStyle} ref={this.setDragOverlayRef}>
                            <BaseItem item={this.draggedItem} draggable={false} dragging={true} />
                        </div>
                    </Teleport>
                )}
            </div>
        );
    },
});
