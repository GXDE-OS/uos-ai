import { computed, defineComponent, nextTick, onMounted, onUnmounted, ref, watch, type VNode, type VNodeRef } from "vue";
import type { PropType } from "vue";

import IconButton from "@/components/IconButton";
import Switch from "@/components/Switch";
import Tooltip from "@/components/Tooltip";
import { useBackendStore } from "@/stores";
import { ButtonShape } from "@/types/button";
import type {
    ToolManagementCustomAction,
    ToolManagementCustomActionResolver,
    ToolManagementItem,
} from "@/views/window/mainwindow/page/settings/common/types";

const URL_REGEX = /(https?:\/\/[^\s<>"']+|www\.[^\s<>"']+)/gi;

const normalizeUrl = (url: string) => {
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }

    return `https://${url}`;
};

export default defineComponent({
    name: "ToolManagementListItem",

    components: {
        IconButton,
        Switch,
        Tooltip,
    },

    props: {
        item: {
            type: Object as PropType<ToolManagementItem>,
            required: true,
        },
        showEditButton: {
            type: Boolean,
            default: true,
        },
        customAction: {
            type: Object as PropType<ToolManagementCustomAction>,
            default: undefined,
        },
    },

    emits: {
        toggleItem: (_itemId: string, _enabled: boolean) => true,
        editItem: (_itemId: string) => true,
        deleteItem: (_itemId: string) => true,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const descriptionRef = ref<HTMLElement | null>(null);
        const isDescriptionOverflowing = ref(false);
        let resizeObserver: ResizeObserver | null = null;

        const resolveCustomActionValue = <T,>(
            value: ToolManagementCustomActionResolver<T> | undefined,
            fallback?: T,
        ): T | undefined => {
            if (typeof value === "function") {
                return (value as (item: ToolManagementItem) => T)(props.item);
            }

            return value ?? fallback;
        };

        const customActionConfig = computed(() => {
            const customAction = props.customAction;

            if (!customAction) {
                return null;
            }

            const icon = resolveCustomActionValue(customAction.icon);

            if (!icon) {
                return null;
            }

            return {
                icon,
                tooltip: resolveCustomActionValue(customAction.tooltip),
                iconSize: resolveCustomActionValue(customAction.iconSize, [16, 16]) ?? [16, 16],
                visible: resolveCustomActionValue(customAction.visible, true) ?? true,
                disabled: resolveCustomActionValue(customAction.disabled, false) ?? false,
            };
        });

        const showActions = computed(() => {
            return Boolean(customActionConfig.value?.visible) || (props.showEditButton && props.item.editable) || props.item.removable;
        });

        // 只有描述实际被省略时才显示完整 tooltip
        const tooltipDisabled = computed(() => {
            return !props.item.description || !isDescriptionOverflowing.value;
        });

        const updateDescriptionOverflowState = () => {
            const descriptionElement = descriptionRef.value;

            if (!descriptionElement) {
                isDescriptionOverflowing.value = false;
                return;
            }

            isDescriptionOverflowing.value = descriptionElement.scrollWidth - descriptionElement.clientWidth > 1;
        };

        const handleDescriptionMouseEnter = () => {
            updateDescriptionOverflowState();
        };

        const setDescriptionRef: VNodeRef = (element) => {
            descriptionRef.value = element as HTMLElement | null;
        };

        // tooltip 内容挂在浮层内部，显式走 systemChannel 才能稳定拉起系统默认浏览器
        const handleTooltipLinkClick = (event: MouseEvent, url: string) => {
            event.preventDefault();
            event.stopPropagation();

            void backendStore.requestSystem("openUrl", normalizeUrl(url));
        };

        // 将描述拆成"普通文本 + 可点击链接"，同时保留原始换行
        const renderTooltipDescription = (): Array<string | VNode> => {
            const nodes: Array<string | VNode> = [];
            const lines = props.item.description.split("\n");
            let nodeKey = 0;

            lines.forEach((line, lineIndex) => {
                URL_REGEX.lastIndex = 0;
                let lastIndex = 0;
                let match = URL_REGEX.exec(line);

                while (match) {
                    const matchedUrl = match[0];

                    if (match.index > lastIndex) {
                        nodes.push(line.slice(lastIndex, match.index));
                    }

                    const normalizedUrl = normalizeUrl(matchedUrl);

                    nodes.push(
                        <a
                            key={`tool-item-tooltip-link-${nodeKey}`}
                            href={normalizedUrl}
                            class="tool-management-list-item__tooltip-link"
                            onClick={(event) => handleTooltipLinkClick(event as MouseEvent, matchedUrl)}
                        >
                            {matchedUrl}
                        </a>,
                    );

                    nodeKey += 1;
                    lastIndex = match.index + matchedUrl.length;
                    match = URL_REGEX.exec(line);
                }

                if (lastIndex < line.length) {
                    nodes.push(line.slice(lastIndex));
                }

                if (lineIndex < lines.length - 1) {
                    nodes.push(<br key={`tool-item-tooltip-break-${nodeKey}`} />);
                    nodeKey += 1;
                }
            });

            return nodes;
        };

        const renderTooltipContent = () => {
            return (
                <div class="tool-management-list-item__tooltip">
                    <div class="tool-management-list-item__tooltip-title">{props.item.name}</div>
                    <div class="tool-management-list-item__tooltip-description">{renderTooltipDescription()}</div>
                </div>
            );
        };

        const handleToggleChange = (value: boolean) => {
            emit("toggleItem", props.item.id, value);
        };

        const handleEdit = () => {
            emit("editItem", props.item.id);
        };

        const handleDelete = () => {
            emit("deleteItem", props.item.id);
        };

        const handleCustomAction = (event: MouseEvent) => {
            props.customAction?.onClick(props.item, event);
        };

        onMounted(() => {
            nextTick(() => {
                updateDescriptionOverflowState();

                if (typeof ResizeObserver !== "undefined" && descriptionRef.value) {
                    // 列表宽度变化时同步更新省略状态，避免 tooltip 启用状态滞后
                    resizeObserver = new ResizeObserver(() => {
                        updateDescriptionOverflowState();
                    });
                    resizeObserver.observe(descriptionRef.value);
                }
            });
        });

        onUnmounted(() => {
            resizeObserver?.disconnect();
            resizeObserver = null;
        });

        watch(
            () => [props.item.id, props.item.description],
            () => {
                nextTick(() => {
                    updateDescriptionOverflowState();
                });
            },
        );

        return {
            showActions,
            customActionConfig,
            tooltipDisabled,
            setDescriptionRef,
            handleDescriptionMouseEnter,
            renderTooltipContent,
            handleToggleChange,
            handleEdit,
            handleDelete,
            handleCustomAction,
        };
    },

    render() {
        const actionClass = this.showActions
            ? "tool-management-list-item__actions tool-management-list-item__actions--custom"
            : "tool-management-list-item__actions";

        return (
            <div class="tool-management-list-item">
                <div class="tool-management-list-item__content">
                    <div class="tool-management-list-item__header">
                        <div class="tool-management-list-item__title-row">
                            <span class="tool-management-list-item__title">{this.$props.item.name}</span>
                            {this.$props.item.isBuiltIn && (
                                <span class="tool-management-list-item__badge tool-management-list-item__badge--builtin">
                                    built-in
                                </span>
                            )}
                        </div>

                        <div class={actionClass}>
                            {this.$props.showEditButton && this.$props.item.editable && (
                                <IconButton
                                    icon="edit"
                                    iconSize={[16, 16]}
                                    onClick={this.handleEdit}
                                    shape={ButtonShape.Rounded}
                                    size={[24, 24]}
                                />
                            )}

                            {this.customActionConfig?.visible && (
                                <IconButton
                                    icon={this.customActionConfig.icon}
                                    iconSize={this.customActionConfig.iconSize}
                                    onClick={this.handleCustomAction}
                                    shape={ButtonShape.Rounded}
                                    size={[24, 24]}
                                    tooltip={this.customActionConfig.tooltip}
                                    disabled={this.customActionConfig.disabled}
                                />
                            )}

                            {this.$props.item.removable && (
                                <IconButton
                                    icon="trash"
                                    iconSize={[14, 14]}
                                    onClick={this.handleDelete}
                                    shape={ButtonShape.Rounded}
                                    size={[24, 24]}
                                />
                            )}
                        </div>
                    </div>

                    <Tooltip
                        content={this.$props.item.description}
                        placement="bottom"
                        showAfter={1000}
                        hideAfter={300}
                        disabled={this.tooltipDisabled}
                        popperClass="tool-management-list-item__tooltip-popper"
                        v-slots={{
                            content: this.renderTooltipContent,
                        }}
                    >
                        <div
                            ref={this.setDescriptionRef}
                            class="tool-management-list-item__description"
                            onMouseenter={this.handleDescriptionMouseEnter}
                        >
                            {this.$props.item.description}
                        </div>
                    </Tooltip>
                </div>

                <Switch
                    value={this.$props.item.enabled}
                    onChange={this.handleToggleChange}
                />
            </div>
        );
    },
});
