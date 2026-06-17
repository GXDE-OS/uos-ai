import { defineComponent, ref, nextTick, watch, type PropType, type CSSProperties, type VNodeRef } from "vue";
import SvgIcon from "@/components/SvgIcon";
import { ComboBoxDropdownAlign, type ComboboxOption } from "@/types/combobox";
import { useBackendStore } from "@/stores/backend";

export default defineComponent({
    name: "ComboBoxDropdown",
    props: {
        isOpen: {
            type: Boolean,
            required: true,
        },
        options: {
            type: Array as PropType<ComboboxOption[]>,
            required: true,
        },
        selectedValue: {
            type: [String, Number] as PropType<string | number>,
            required: true,
        },
        triggerRef: {
            type: Object as PropType<HTMLElement | null>,
            default: null,
        },
        dropdownAlign: {
            type: String as PropType<ComboBoxDropdownAlign>,
            default: ComboBoxDropdownAlign.Left,
        },
    },
    emits: {
        select: (option: ComboboxOption) => true,
        close: () => true,
    },
    setup(props, { emit }) {
        const dropdownElement = ref<HTMLElement | null>(null);
        const dropdownStyle = ref<CSSProperties>({ position: "fixed", visibility: "hidden" });
        const backend = useBackendStore();

        const updatePosition = () => {
            if (!props.triggerRef || !props.isOpen) return;

            const triggerRect = props.triggerRef.getBoundingClientRect();
            const workspace = document.querySelector(".workspace") as HTMLElement;

            // 获取边界
            const windowHeight = window.innerHeight;
            const windowWidth = window.innerWidth;
            const workspaceRect = workspace ? workspace.getBoundingClientRect() : { left: 0, right: windowWidth };
            const maxDropdownWidth = Math.max(workspaceRect.right - workspaceRect.left - 16, 120);
            const measuredDropdownWidth = dropdownElement.value?.offsetWidth || 0;
            const dropdownWidth = Math.min(Math.max(triggerRect.width, measuredDropdownWidth, 120), maxDropdownWidth);

            // 默认显示在触发器下方
            let top = triggerRect.bottom + 4;
            let left = triggerRect.left;

            // 检查是否超出底部，如果超出则显示在上方
            if (top + 200 > windowHeight) {
                top = triggerRect.top - 204; // 200 是 max-height，4 是间距
            }

            switch (props.dropdownAlign) {
                case ComboBoxDropdownAlign.Right:
                    left = triggerRect.right - dropdownWidth;
                    break;
                case ComboBoxDropdownAlign.Center:
                    left = triggerRect.left + (triggerRect.width - dropdownWidth) / 2;
                    break;
                default:
                    left = triggerRect.left;
            }

            // 检查是否超出 workspace 右边界
            if (left + dropdownWidth > workspaceRect.right) {
                left = workspaceRect.right - dropdownWidth - 8; // 8 是右边距
            }

            // 检查是否超出 workspace 左边界
            if (left < workspaceRect.left) {
                left = workspaceRect.left + 8; // 8 是左边距
            }

            dropdownStyle.value = {
                position: "fixed",
                visibility: "visible",
                top: `${top}px`,
                left: `${left}px`,
                minWidth: `${dropdownWidth}px`,
                maxWidth: `${workspaceRect.right - workspaceRect.left - 16}px`,
            };
        };

        watch(
            () => props.isOpen,
            (isOpen) => {
                if (isOpen) {
                    dropdownStyle.value = { position: "fixed", visibility: "hidden" };
                    void nextTick(() => setTimeout(updatePosition, 0));
                }
            },
        );

        const handleItemClick = (option: ComboboxOption) => {
            emit("select", option);
        };

        const setDropdownRef: VNodeRef = (el) => {
            dropdownElement.value = el as HTMLElement | null;
        };

        return {
            dropdownElement,
            dropdownStyle,
            isEnableAdvancedCssFeatures: backend.$state.isEnableAdvancedCssFeatures,
            handleItemClick,
            setDropdownRef,
        };
    },
    render() {
        if (!this.$props.isOpen) return null;

        return (
            <div
                ref={this.setDropdownRef}
                class={["combobox-dropdown", this.isEnableAdvancedCssFeatures && "combobox-dropdown--advanced-css"]}
                style={this.dropdownStyle}
            >
                {this.$props.options.map((option) => {
                    const isSelected = option.value === this.$props.selectedValue;

                    return (
                        <div
                            key={String(option.value)}
                            class={["combobox-dropdown-item", isSelected && "combobox-dropdown-item--selected"]}
                            onClick={() => this.handleItemClick(option)}
                        >
                            {isSelected && <SvgIcon icon="icon_selected" class="combobox-dropdown-item-check-icon" />}
                            <span class="combobox-dropdown-item-text">{option.label || String(option.value)}</span>
                        </div>
                    );
                })}
            </div>
        );
    },
});
