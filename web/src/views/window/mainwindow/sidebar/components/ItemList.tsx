import { defineComponent, computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from "vue";
import type { PropType } from "vue";
import type { SidebarGroup } from "@/views/window/mainwindow/sidebar/types";
import Group from "@/views/window/mainwindow/sidebar/components/Group";
import "@/assets/styles/window/mainwindow/sidebar/components/ItemList.css";

const GROUP_HEADER_HEIGHT = 30;
const shouldShowGroupHeader = (group: SidebarGroup) => group.showHeader !== false;

export default defineComponent({
    name: "ItemList",
    props: {
        groups: {
            type: Array as PropType<SidebarGroup[]>,
            required: true,
        },
        scrollTop: {
            type: Number,
            required: true,
        },
    },
    emits: ["item-click", "reorder", "rightButtonIconClick", "stickyChange", "rightButtonClick"],
    setup(props, { emit }) {
        const groupsRef = ref<HTMLElement | null>(null);
        const headerBaseTops = ref<Record<string, number>>({});
        let resizeObserver: ResizeObserver | null = null;
        let measureFrame: number | null = null;

        const handleItemClick = (item: SidebarGroup["items"][number]) => {
            emit("item-click", item);
        };

        // 常驻数量只由支持常驻/隐藏区跨区拖拽的分组上报，其他分组透传即可。
        const handleReorder = (payload: { groupId: string; newItems: string[]; visibleCount?: number }) => {
            emit("reorder", payload);
        };

        const groupsWithHeaders = computed(() => props.groups.filter(shouldShowGroupHeader));
        const groupHeaderOrderMap = computed(() => {
            const orderMap: Record<string, number> = {};
            let headerOrder = 0;

            for (const group of props.groups) {
                if (!shouldShowGroupHeader(group)) {
                    continue;
                }

                orderMap[group.id] = headerOrder;
                headerOrder += 1;
            }

            return orderMap;
        });

        const stickyGroups = computed(() => {
            const nextGroups: SidebarGroup[] = [];

            for (const group of groupsWithHeaders.value) {
                const headerTop = headerBaseTops.value[group.id];
                if (typeof headerTop !== "number") {
                    break;
                }

                const headerOrder = groupHeaderOrderMap.value[group.id] ?? 0;
                if (props.scrollTop > headerTop - headerOrder * GROUP_HEADER_HEIGHT) {
                    nextGroups.push(group);
                    continue;
                }

                break;
            }

            return nextGroups;
        });

        const stickyStackHeight = computed(() => stickyGroups.value.length * GROUP_HEADER_HEIGHT);
        const stickyGroupIdMap = computed(() =>
            Object.fromEntries(stickyGroups.value.map((group) => [group.id, true])) as Record<string, boolean>,
        );

        const listLayerStyle = computed(() => ({
            "--item-list-sticky-stack-height": `${stickyStackHeight.value}px`,
        }) as Record<string, string>);

        const areHeaderBaseTopsEqual = (nextValue: Record<string, number>, prevValue: Record<string, number>) => {
            const nextKeys = Object.keys(nextValue);
            const prevKeys = Object.keys(prevValue);

            if (nextKeys.length !== prevKeys.length) {
                return false;
            }

            for (const key of nextKeys) {
                if (nextValue[key] !== prevValue[key]) {
                    return false;
                }
            }

            return true;
        };

        const measureHeaderBaseTops = () => {
            const groupsElement = groupsRef.value;
            if (!groupsElement) {
                return;
            }

            const nextHeaderBaseTops: Record<string, number> = {};

            for (const group of groupsWithHeaders.value) {
                const groupElement = groupsElement.querySelector<HTMLElement>(`#group-${group.id}`);
                if (!groupElement) {
                    continue;
                }
                nextHeaderBaseTops[group.id] = groupElement.offsetTop;
            }

            if (!areHeaderBaseTopsEqual(nextHeaderBaseTops, headerBaseTops.value)) {
                headerBaseTops.value = nextHeaderBaseTops;
            }
        };

        const scheduleMeasurement = () => {
            if (measureFrame !== null) {
                return;
            }

            measureFrame = requestAnimationFrame(() => {
                measureFrame = null;
                measureHeaderBaseTops();
            });
        };

        const syncMeasurements = () => {
            nextTick(() => {
                scheduleMeasurement();
            });
        };

        const setGroupsRef = (element: Element | { $el?: Element } | null) => {
            groupsRef.value = ((element as { $el?: Element } | null)?.$el ?? element) as HTMLElement | null;

            if (!resizeObserver) {
                return;
            }

            resizeObserver.disconnect();
            if (groupsRef.value) {
                resizeObserver.observe(groupsRef.value);
            }
        };

        onMounted(() => {
            resizeObserver = new ResizeObserver(() => {
                scheduleMeasurement();
            });
            if (groupsRef.value) {
                resizeObserver.observe(groupsRef.value);
            }
            syncMeasurements();
        });

        onBeforeUnmount(() => {
            if (measureFrame !== null) {
                cancelAnimationFrame(measureFrame);
                measureFrame = null;
            }
            resizeObserver?.disconnect();
            resizeObserver = null;
        });

        watch(
            () => props.groups,
            () => {
                syncMeasurements();
            },
            { deep: true },
        );

        watch(
            [stickyGroups, stickyStackHeight],
            () => {
                emit("stickyChange", {
                    groups: stickyGroups.value,
                    stackHeight: stickyStackHeight.value,
                });
            },
            { immediate: true },
        );

        return {
            groupHeaderOrderMap,
            stickyGroupIdMap,
            listLayerStyle,
            handleItemClick,
            handleReorder,
            setGroupsRef,
        };
    },
    render() {
        return (
            <div class="item-list">
                <div class="item-list__content" style={this.listLayerStyle}>
                    <div class="item-list__groups" ref={this.setGroupsRef}>
                        {this.groups.map((group) => (
                            <Group
                                key={group.id}
                                group={group}
                                headerOrder={this.groupHeaderOrderMap[group.id] ?? 0}
                                headerDomId={`inline-group-header-${group.id}`}
                                headerHidden={Boolean(this.stickyGroupIdMap[group.id])}
                                onItem-click={this.handleItemClick}
                                onReorder={this.handleReorder}
                                onRightButtonIconClick={(params: Record<string, any>) =>
                                    this.$emit("rightButtonIconClick", params)
                                }
                                onRightButtonClick={(params: { item: SidebarGroup["items"][number]; event: MouseEvent }) =>
                                    this.$emit("rightButtonClick", params)
                                }
                                v-slots={{
                                    item: this.$slots.item ? (slotProps: any) => this.$slots.item?.(slotProps) : undefined,
                                }}
                            />
                        ))}
                    </div>
                </div>
            </div>
        );
    },
});
