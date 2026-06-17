import { defineComponent, ref, watch, type VNode } from "vue";
import type { PropType } from "vue";
import SvgIcon from "@/components/SvgIcon";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";

import type { SidebarItem } from "@/views/window/mainwindow/sidebar/types";
import "@/assets/styles/window/mainwindow/sidebar/components/BaseItem.css";

export default defineComponent({
    name: "BaseItem",
    components: {
        SvgIcon,
    },
    props: {
        item: {
            type: Object as PropType<SidebarItem>,
            required: true,
        },
        draggable: {
            type: Boolean,
            default: false,
        },
        dragging: {
            type: Boolean,
            default: false,
        },
        ghost: {
            type: Boolean,
            default: false,
        },
        // 拖拽排序时由外层临时关闭 hover/active 反馈，避免列表项和拖拽浮层同时高亮。
        suppressHover: {
            type: Boolean,
            default: false,
        },
    },
    emits: ["click", "rightButtonClick"],
    setup(props, { emit }) {
        const isHovered = ref(false);

        watch(
            () => props.suppressHover,
            (suppressHover) => {
                // 外部进入反馈抑制态时，立即清空内部 hover 状态。
                if (suppressHover) {
                    isHovered.value = false;
                }
            },
        );

        const handleClick = () => {
            emit("click", props.item);
        };

        const handleMouseEnter = () => {
            if (props.suppressHover) {
                // 抑制态下不展示右侧按钮，保持拖拽过程中的视觉稳定。
                isHovered.value = false;
                return;
            }
            isHovered.value = true;
        };

        const handleMouseLeave = () => {
            isHovered.value = false;
        };

        const handleRightButtonClick = (event: MouseEvent) => {
            event.stopPropagation();
            emit("rightButtonClick", { item: props.item, event });
        };

        return {
            handleClick,
            handleMouseEnter,
            handleMouseLeave,
            handleRightButtonClick,
            isHovered,
        };
    },
    render() {
        const { item, $attrs } = this;
        const rightContent = (() => {
            if (!item.right) return null;
            if (typeof item.right === "string") {
                return <span class="base-item__status-icon">{item.right}</span>;
            }
            return item.right();
        })();

        const rightMoreButton =
            item.rightButtonIcon && this.isHovered && !this.suppressHover ? (
                <span class="base-item__status-icon">
                    <IconButton
                        icon={item.rightButtonIcon as string}
                        iconSize={[16, 16]}
                        size={[16, 16]}
                        shape={ButtonShape.Rounded}
                        colorOnly={true}
                        onClick={this.handleRightButtonClick}
                    />
                </span>
            ) : undefined;

        const baseClass = {
            "base-item": true,
            "base-item--selected": item.selected ?? false,
            "base-item--draggable": this.draggable,
            "base-item--dragging": this.dragging,
            "base-item--ghost": this.ghost,
            "base-item--feedback-suppressed": this.suppressHover,
            "base-item--has-right-content": !!rightMoreButton || !!rightContent,
        };

        const renderIcon = (): VNode | null => {
            if (!item.icon) return null;

            if (item.icon.startsWith("file://")) {
                return (
                    <div class="base-item__icon-wrapper">
                        <img src={item.icon} />
                    </div>
                );
            }

            return (
                <div class="base-item__icon-wrapper">
                    <SvgIcon icon={item.icon} size={[16, 16]} />
                </div>
            );
        };

        return (
            <div
                class={baseClass}
                draggable={this.draggable}
                onClick={this.handleClick}
                onMouseenter={this.handleMouseEnter}
                onMouseleave={this.handleMouseLeave}
                {...$attrs}
            >
                <div class="base-item__content">
                    <div class="base-item__left">
                        {renderIcon()}
                        <span class="base-item__name">{item.name}</span>
                    </div>
                    {rightContent && <div class="base-item__right">{rightContent}</div>}
                    {rightMoreButton && <div class="base-item__right">{rightMoreButton}</div>}
                </div>
            </div>
        );
    },
});
