import { defineComponent, ref, onMounted } from "vue";
import type { PropType } from "vue";
import SvgIcon from "@/components/SvgIcon";
import type { MenuItem } from "@/types/menu";
import { loadThemeIcon } from "@/utils/loadThemeIcon";
import "@/assets/styles/components/menu/MenuItem.css";

export default defineComponent({
    name: "MenuItem",
    props: {
        item: {
            type: Object as PropType<MenuItem>,
            required: true,
        },
        selected: {
            type: Boolean,
            default: false,
        },
        level: {
            type: Number,
            default: 0,
        },
        /** 菜单项是否可勾选（显示勾选标记） */
        checkable: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        click: (item: MenuItem) => true,
    },
    setup(props, { emit }) {
        const themeIconUrl = ref<string>("");

        const handleClick = () => {
            if (props.item.disabled) return;
            emit("click", props.item);
            if (props.item.onClick) {
                props.item.onClick();
            }
        };

        const handleKeyDown = (event: KeyboardEvent) => {
            if (props.item.disabled) return;
            if (event.key === "Enter" || event.key === " ") {
                event.preventDefault();
                handleClick();
            }
        };

        onMounted(async () => {
            if (!props.item.themeIcon) return;
            const url = await loadThemeIcon(props.item.themeIcon, 16, 16);
            if (url) themeIconUrl.value = url;
        });

        return {
            handleClick,
            handleKeyDown,
            themeIconUrl,
        };
    },
    render() {
        const isChecked = this.$props.checkable && this.$props.item.checked === true;

        return (
            <div
                class={[
                    "menu-item",
                    this.$props.selected && "menu-item--selected",
                    this.$props.item.disabled && "menu-item--disabled",
                    this.$props.level > 0 && `menu-item--level-${this.$props.level}`,
                    this.$props.checkable && "menu-item--checkable",
                    isChecked && "menu-item--checked",
                ]}
                onClick={this.handleClick}
                onKeydown={this.handleKeyDown}
                role="menuitem"
                aria-disabled={this.$props.item.disabled}
                aria-selected={this.$props.selected}
                aria-checked={this.$props.checkable ? isChecked : undefined}
                tabindex={this.$props.item.disabled ? -1 : 0}
            >
                {/* Checkable 勾选标记区域 */}
                {this.$props.checkable && (
                    <div class="menu-item__check">
                        {isChecked && <SvgIcon icon="icon_selected" size={[16, 16]} />}
                    </div>
                )}

                {this.$slots.icon ? (
                    <div class="menu-item__icon">{this.$slots.icon()}</div>
                ) : this.themeIconUrl ? (
                    <div class="menu-item__icon">
                        <img src={this.themeIconUrl} style={{ width: "100%", height: "100%", objectFit: "contain" }} />
                    </div>
                ) : this.$props.item.icon ? (
                    <div class="menu-item__icon">
                        <SvgIcon icon={this.$props.item.icon} size={[16, 16]} />
                    </div>
                ) : null}

                <div class="menu-item__label">
                    {this.$slots.label ? this.$slots.label() : this.$props.item.label}
                </div>
            </div>
        );
    },
});
