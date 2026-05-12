import {
    defineComponent,
    ref,
    computed,
    onMounted,
    onUnmounted,
    type PropType,
    type CSSProperties,
    type VNodeRef,
} from "vue";
import SvgIcon from "@/components/SvgIcon";
import { ComboBoxDropdownAlign, type ComboboxOption } from "@/types/combobox";
import ComboBoxDropdown from "./ComboBoxDropdown";

export default defineComponent({
    name: "ComboBox",
    props: {
        options: {
            type: Array as PropType<ComboboxOption[]>,
            required: true,
        },
        value: {
            type: [String, Number] as PropType<string | number>,
            required: true,
        },
        compact: {
            type: Boolean,
            default: false,
        },
        placeholder: {
            type: String,
            default: "",
        },
        disabled: {
            type: Boolean,
            default: false,
        },
        arrowIcon: {
            type: String,
            default: "icon_arrow",
        },
        customClass: {
            type: String,
            default: "",
        },
        dropdownAlign: {
            type: String as PropType<ComboBoxDropdownAlign>,
            default: ComboBoxDropdownAlign.Left,
        },
        defaultValue: {
            type: String,
            default: "",
        },
        paddingX: {
            type: [String, Number] as PropType<string | number>,
            default: undefined,
        },
    },
    emits: {
        updateValue: (value: string | number) => true,
        changeDropdown: (isOpen: boolean) => true,
        clickOption: (option: ComboboxOption) => true,
    },
    setup(props, { emit }) {
        const isDropdownOpen = ref(false);
        const containerElement = ref<HTMLElement | null>(null);
        const triggerElement = ref<HTMLElement | null>(null);

        const selectedOption = computed(() => {
            return props.options.find((opt) => opt.value === props.value);
        });

        const displayText = computed(() => {
            if (props.compact) return "";
            if (props.options.length === 0) return props.defaultValue || props.placeholder || "";
            return selectedOption.value?.label || String(props.value) || props.placeholder || "";
        });

        const containerStyle = computed((): CSSProperties => {
            return {
                position: "relative",
            };
        });

        const comboboxStyle = computed((): CSSProperties => {
            if (props.paddingX === undefined) return {};
            const paddingValue = typeof props.paddingX === "number" ? `${props.paddingX}px` : props.paddingX;
            return {
                paddingLeft: paddingValue,
                paddingRight: paddingValue,
            };
        });

        const toggleDropdown = () => {
            if (props.disabled) return;
            isDropdownOpen.value = !isDropdownOpen.value;
            emit("changeDropdown", isDropdownOpen.value);
        };

        const handleSelect = (option: ComboboxOption) => {
            emit("clickOption", option);
            emit("updateValue", option.value);
            isDropdownOpen.value = false;
        };

        const handleClose = () => {
            isDropdownOpen.value = false;
        };

        const handleMouseDownOutside = (event: MouseEvent) => {
            if (!isDropdownOpen.value) return;
            if (containerElement.value && !containerElement.value.contains(event.target as Node)) {
                isDropdownOpen.value = false;
            }
        };

        const handleWindowBlur = () => {
            if (isDropdownOpen.value) {
                isDropdownOpen.value = false;
            }
        };

        const setContainerRef: VNodeRef = (el) => {
            containerElement.value = el as HTMLElement | null;
        };

        const setTriggerRef: VNodeRef = (el) => {
            triggerElement.value = el as HTMLElement | null;
        };

        onMounted(() => {
            // 使用 mousedown 事件在捕获阶段监听，确保在其他组件处理点击之前关闭下拉菜单
            document.addEventListener("mousedown", handleMouseDownOutside, true);
            // 监听窗口失焦事件，处理点击窗口外部的情况（Qt WebEngine 中点击窗口外不会触发 document 事件）
            window.addEventListener("blur", handleWindowBlur);
        });

        onUnmounted(() => {
            document.removeEventListener("mousedown", handleMouseDownOutside, true);
            window.removeEventListener("blur", handleWindowBlur);
        });

        return {
            isDropdownOpen,
            containerElement,
            triggerElement,
            selectedOption,
            displayText,
            containerStyle,
            comboboxStyle,
            toggleDropdown,
            handleSelect,
            handleClose,
            setContainerRef,
            setTriggerRef,
        };
    },
    render() {
        return (
            <div ref={this.setContainerRef} class={this.$props.customClass} style={this.containerStyle}>
                <div
                    ref={this.setTriggerRef}
                    class={[
                        "combobox",
                        this.$props.compact && "combobox--compact",
                        this.$props.disabled && "combobox-disabled",
                    ]}
                    style={this.comboboxStyle}
                    onClick={this.toggleDropdown}
                >
                    {this.selectedOption?.icon && <SvgIcon icon={this.selectedOption.icon} class="combobox-icon" />}
                    <span class="combobox-text">{this.displayText}</span>
                    <SvgIcon
                        icon={this.$props.arrowIcon}
                        class={["combobox-arrow", this.isDropdownOpen && "combobox-arrow--open"]}
                    />
                </div>

                {this.$slots.dropdown ? (
                    this.$slots.dropdown({
                        isOpen: this.isDropdownOpen,
                        options: this.$props.options,
                        selectedValue: this.$props.value,
                        triggerRef: this.triggerElement,
                        dropdownAlign: this.$props.dropdownAlign,
                        onSelect: this.handleSelect,
                        onClose: this.handleClose,
                    })
                ) : (
                    <ComboBoxDropdown
                        isOpen={this.isDropdownOpen}
                        options={this.$props.options}
                        selectedValue={this.$props.value}
                        triggerRef={this.triggerElement}
                        dropdownAlign={this.$props.dropdownAlign}
                        onSelect={this.handleSelect}
                        onClose={this.handleClose}
                    />
                )}
            </div>
        );
    },
});
