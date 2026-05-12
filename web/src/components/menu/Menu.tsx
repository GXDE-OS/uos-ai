import { defineComponent, ref, computed, watch, onMounted, onUnmounted, nextTick, type VNodeRef } from "vue";
import type { PropType, CSSProperties } from "vue";
import { useBackendStore } from "@/stores/backend";
import MenuItem from "./MenuItem";
import MenuSeparator from "./MenuSeparator";
import SubMenu from "./SubMenu";
import type { MenuItem as MenuItemType } from "@/types/menu";
import "@/assets/styles/components/menu/Menu.css";

export default defineComponent({
    name: "Menu",
    props: {
        items: {
            type: Array as PropType<MenuItemType[]>,
            required: true,
        },
        visible: {
            type: Boolean,
            required: true,
        },
        triggerRef: {
            type: Object as PropType<HTMLElement | null>,
            default: undefined,
        },
        placement: {
            type: String as PropType<"top" | "bottom" | "left" | "right">,
            default: "bottom",
        },
        offset: {
            type: Number,
            default: 8,
        },
        /** 菜单项是否可勾选（显示勾选标记） */
        checkable: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        updateVisible: (visible: boolean) => true,
        selectItem: (item: MenuItemType) => true,
    },
    setup(props, { emit }) {
        const menuRef = ref<HTMLElement | null>(null);
        const activeItemId = ref<string | undefined>(undefined);
        // 控制动画的可见状态（与 props.visible 解耦：先定位，再触发动画）
        const menuVisible = ref(false);
        // 动画方向：true = 菜单在触发元素下方（从上向下入场），false = 在上方（从下向上入场）
        const menuFromTop = ref(true);
        const backend = useBackendStore();

        const menuStyle = computed((): CSSProperties => ({
            position: "fixed",
            zIndex: 9999,
        }));

        const updateMenuPosition = () => {
            if (!menuRef.value || !props.triggerRef) return;

            const menuRect = menuRef.value.getBoundingClientRect();
            const triggerRect = props.triggerRef.getBoundingClientRect();
            const viewportWidth = window.innerWidth;
            const viewportHeight = window.innerHeight;

            let top = 0;
            let left = 0;

            switch (props.placement) {
                case "bottom":
                    top = triggerRect.bottom + props.offset;
                    left = triggerRect.left;
                    break;
                case "top":
                    top = triggerRect.top - menuRect.height - props.offset;
                    left = triggerRect.left;
                    break;
                case "left":
                    top = triggerRect.top;
                    left = triggerRect.left - menuRect.width - props.offset;
                    break;
                case "right":
                    top = triggerRect.top;
                    left = triggerRect.right + props.offset;
                    break;
            }

            // 边界检测：防止超出视口
            if (left + menuRect.width > viewportWidth) {
                left = viewportWidth - menuRect.width - 8;
            }
            if (left < 0) {
                left = 8;
            }
            if (top + menuRect.height > viewportHeight) {
                top = viewportHeight - menuRect.height - 8;
            }
            if (top < 0) {
                top = 8;
            }
            menuRef.value.style.top = `${top}px`;
            menuRef.value.style.left = `${left}px`;

            // 定位完成后，根据菜单实际位置与触发元素的相对关系决定动画方向
            // 菜单顶部 >= 触发元素底部 → 菜单在下方，从上向下入场；否则从下向上入场
            menuFromTop.value = top >= triggerRect.bottom;
        };

        const handleClickOutside = (event: MouseEvent) => {
            if (!props.visible || !menuRef.value) return;

            const target = event.target as Node;
            const triggerEl = props.triggerRef;

            // 检查点击是否在菜单内或触发元素上
            const isClickInsideMenu = menuRef.value.contains(target);
            const isClickOnTrigger = triggerEl && triggerEl.contains(target);

            if (!isClickInsideMenu && !isClickOnTrigger) {
                emit("updateVisible", false);
            }
        };

        const handleSelectItem = (item: MenuItemType) => {
            // 调用菜单项定义的 onClick 处理器
            if (item.onClick) {
                item.onClick();
            }
            emit("selectItem", item);
            emit("updateVisible", false);
        };

        const handleItemClick = (item: MenuItemType) => {
            activeItemId.value = item.id;
            handleSelectItem(item);
        };

        const setMenuRef: VNodeRef = (el) => {
            menuRef.value = el as HTMLElement | null;
        };

        // 监听 visible 变化，更新菜单位置
        watch(
            () => props.visible,
            (newValue) => {
                if (newValue) {
                    nextTick(() => {
                        updateMenuPosition();
                        // 定位完成后再触发动画（避免从 0,0 开始动画）
                        menuVisible.value = true;
                    });
                } else {
                    menuVisible.value = false;
                    activeItemId.value = undefined;
                }
            },
        );

        onMounted(() => {
            document.addEventListener("click", handleClickOutside);
        });

        onUnmounted(() => {
            document.removeEventListener("click", handleClickOutside);
        });

        return {
            menuRef,
            activeItemId,
            menuVisible,
            menuFromTop,
            menuStyle,
            handleItemClick,
            isEnableAdvancedCssFeatures: backend.$state.isEnableAdvancedCssFeatures,
            setMenuRef,
        };
    },
    render() {
        const renderMenuItem = (item: MenuItemType, index: number) => {
            switch (item.type) {
                case "separator":
                    return <MenuSeparator key={`separator-${index}`} />;
                case "submenu":
                    return (
                        <SubMenu key={item.id || `submenu-${index}`} item={item} onSelectItem={this.handleItemClick} checkable={this.$props.checkable} />
                    );
                case "item":
                default:
                    return (
                        <MenuItem
                            key={item.id || `item-${index}`}
                            item={item}
                            selected={this.activeItemId === item.id}
                            onClick={this.handleItemClick}
                            checkable={this.$props.checkable}
                        />
                    );
            }
        };

        return (
            <>
                {this.$slots.default && this.$slots.default()}

                <div
                    ref={this.setMenuRef}
                    class={["menu", "menu--root", this.isEnableAdvancedCssFeatures && "menu--advanced-css", this.menuVisible && "menu--visible", this.menuFromTop ? "menu--from-top" : "menu--from-bottom"]}
                    style={this.menuStyle}
                    role="menu"
                    aria-hidden={!this.$props.visible}
                    onDblclick={(event) => {
                        event.preventDefault();
                        event.stopPropagation();
                    }}
                >
                    <div class="menu__content">
                        {this.$props.items.map((item, index) => renderMenuItem(item, index))}
                    </div>
                </div>
            </>
        );
    },
});
