import { defineComponent, ref, nextTick, onBeforeUnmount } from "vue";
import type { PropType, CSSProperties } from "vue";
import SvgIcon from "@/components/SvgIcon";
import type { MenuItem } from "@/types/menu";
import MenuItemComp from "./MenuItem";
import MenuSeparatorComp from "./MenuSeparator";
import "@/assets/styles/components/menu/SubMenu.css";

// 关闭延迟（ms）：留出足够时间让光标从触发行移入子菜单
const CLOSE_DELAY = 200;

export default defineComponent({
    name: "SubMenu",
    props: {
        item: {
            type: Object as PropType<MenuItem>,
            required: true,
        },
        level: {
            type: Number,
            default: 0,
        },
        checkable: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        selectItem: (item: MenuItem) => true,
    },
    setup(props, { emit }) {
        const isSubMenuOpen = ref(false);
        const childrenRef = ref<HTMLElement | null>(null);
        // true = 向左展开（右侧空间不足），false = 向右展开（默认）
        const openLeft = ref(false);
        // 检测完方向后才显示，避免位置跳闪
        const childrenVisible = ref(false);

        let closeTimer: ReturnType<typeof setTimeout> | null = null;
        let exitTimer: ReturnType<typeof setTimeout> | null = null;

        const cancelExit = () => {
            if (exitTimer !== null) {
                clearTimeout(exitTimer);
                exitTimer = null;
            }
        };

        const cancelClose = () => {
            if (closeTimer !== null) {
                clearTimeout(closeTimer);
                closeTimer = null;
            }
        };

        const scheduleClose = () => {
            cancelClose();
            closeTimer = setTimeout(() => {
                childrenVisible.value = false;
                closeTimer = null;
                // 等退出动画结束（160ms）后再销毁 DOM
                exitTimer = setTimeout(() => {
                    isSubMenuOpen.value = false;
                    exitTimer = null;
                }, 160);
            }, CLOSE_DELAY);
        };

        const handleMouseEnter = () => {
            if (props.item.disabled) return;
            cancelClose();
            cancelExit();
            // 子菜单已打开（含正在退出动画中），直接恢复显示
            if (isSubMenuOpen.value) {
                childrenVisible.value = true;
                return;
            }
            openLeft.value = false;
            childrenVisible.value = false;
            isSubMenuOpen.value = true;
            // 渲染后量边界，再决定方向并显示
            nextTick(() => {
                if (childrenRef.value) {
                    const rect = childrenRef.value.getBoundingClientRect();
                    if (rect.right > window.innerWidth - 8) {
                        openLeft.value = true;
                    }
                }
                childrenVisible.value = true;
            });
        };

        const handleMouseLeave = () => {
            // 延迟关闭，给光标"穿越间隙"或斜向移入子菜单留出时间
            scheduleClose();
        };

        // 光标进入子菜单面板时取消关闭计时器（也取消退出动画，恢复显示）
        const handleChildrenMouseEnter = () => {
            cancelClose();
            cancelExit();
            childrenVisible.value = true;
        };

        // 光标离开子菜单面板时重新计时
        const handleChildrenMouseLeave = () => {
            scheduleClose();
        };

        const handleSelectItem = (item: MenuItem) => {
            emit("selectItem", item);
        };

        onBeforeUnmount(() => {
            cancelClose();
            cancelExit();
        });

        return {
            isSubMenuOpen,
            childrenRef,
            openLeft,
            childrenVisible,
            handleMouseEnter,
            handleMouseLeave,
            handleChildrenMouseEnter,
            handleChildrenMouseLeave,
            handleSelectItem,
        };
    },
    render() {
        // 根据方向构造定位样式；动画状态由 class 控制，不再用 inline visibility
        const childrenStyle: CSSProperties = this.openLeft
            ? { position: "absolute", right: "100%", top: "0" }
            : { position: "absolute", left: "100%", top: "0" };

        return (
            <div
                class={[
                    "submenu",
                    this.$props.item.disabled && "submenu--disabled",
                    this.$props.checkable && "submenu--checkable",
                ]}
                onMouseenter={this.handleMouseEnter}
                onMouseleave={this.handleMouseLeave}
            >
                <div class="submenu__content">
                    {/* Checkable 占位区域，保持与 MenuItem 对齐 */}
                    {this.$props.checkable && <div class="submenu__check" />}
                    {this.$props.item.icon && (
                        <div class="submenu__icon">
                            <SvgIcon icon={this.$props.item.icon} />
                        </div>
                    )}
                    <span class="submenu__label">{this.$props.item.label}</span>
                    <div class="submenu__arrow">
                        <SvgIcon icon="icon_arrow" />
                    </div>
                </div>

                {/* 内联渲染子菜单，避免 position:fixed 冲突；方向自适应视口 */}
                {this.isSubMenuOpen && this.$props.item.children && (
                    <div
                        ref={(el) => { this.childrenRef = el as HTMLElement | null; }}
                        class={["submenu__children", this.openLeft && "submenu__children--open-left", this.childrenVisible && "submenu__children--visible"]}
                        style={childrenStyle}
                        onMouseenter={this.handleChildrenMouseEnter}
                        onMouseleave={this.handleChildrenMouseLeave}
                    >
                        <div class="menu" role="menu">
                            <div class="menu__content">
                                {this.$props.item.children.map((child, index) =>
                                    child.type === "separator"
                                        ? <MenuSeparatorComp key={`sep-${index}`} />
                                        : <MenuItemComp
                                            key={child.id || `item-${index}`}
                                            item={child}
                                            onClick={this.handleSelectItem}
                                            checkable={this.$props.checkable}
                                          />
                                )}
                            </div>
                        </div>
                    </div>
                )}
            </div>
        );
    },
});
