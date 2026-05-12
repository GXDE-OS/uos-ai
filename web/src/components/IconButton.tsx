import { defineComponent, computed, type PropType, type CSSProperties } from "vue";
import { useBackendStore } from "@/stores";
import SvgIcon from "./SvgIcon";
import { ButtonShape } from "../types/button";
import Tooltip from "./Tooltip";

export default defineComponent({
    name: "IconButton",
    inheritAttrs: true,
    props: {
        icon: {
            type: String,
            required: true,
        },
        shape: {
            type: Number as PropType<ButtonShape>,
            default: ButtonShape.Square,
        },
        iconColor: {
            type: String,
            default: undefined,
        },
        iconSize: {
            type: Array as () => [number, number],
            default: undefined,
        },
        size: {
            type: Array as () => [number, number] | Number,
            default: undefined,
        },
        disabled: {
            type: Boolean,
            default: false,
        },
        tooltip: {
            type: String,
            default: undefined,
        },
        onClick: {
            type: Function as PropType<(event: MouseEvent) => void>,
            default: undefined,
        },
        variant: {
            type: String as PropType<"default" | "filled">,
            default: "default",
        },
        border: {
            type: Boolean,
            default: false,
        },
    },

    setup(props) {
        const backend = useBackendStore();
        const shapeClass = computed(() => {
            return props.shape === ButtonShape.Circle
                ? "icon-button-circle"
                : props.shape === ButtonShape.Rounded
                  ? "icon-button-rounded"
                  : "icon-button-square";
        });

        const variantClass = computed(() => {
            return props.variant === "filled" ? "icon-button--filled" : "";
        });

        const borderClass = computed(() => {
            return props.border ? "icon-button--bordered" : "";
        });

        const isDisabled = computed(() => props.disabled);

        const buttonStyle = computed((): CSSProperties => {
            const style: CSSProperties = {};

            if (props.size !== undefined) {
                if (Array.isArray(props.size)) {
                    style.width = `${props.size[0]}px`;
                    style.height = `${props.size[1]}px`;
                } else if (typeof props.size === "number") {
                    style.width = `${props.size}px`;
                    style.height = `${props.size}px`;
                }
            }

            return style;
        });

        const handleKeyDown = (event: KeyboardEvent) => {
            if ((event.key === "Enter" || event.key === " ") && !isDisabled.value) {
                event.preventDefault();
                if (props.onClick) {
                    props.onClick(event as unknown as MouseEvent);
                }
            }
        };

        return {
            backend,
            shapeClass,
            variantClass,
            borderClass,
            isDisabled,
            buttonStyle,
            handleKeyDown,
        };
    },

    render() {
        const isAdvanced = this.backend.isEnableAdvancedCssFeatures;
        return (
            <Tooltip content={this.$props.tooltip || ""} showAfter={1000} disabled={this.$props.tooltip === undefined}>
                <div
                    {...this.$attrs}
                    class={[
                        "icon-button",
                        this.shapeClass,
                        this.variantClass,
                        this.borderClass,
                        isAdvanced && this.variantClass && "icon-button--advanced",
                        this.isDisabled && "icon-button-disabled",
                    ]}
                    style={this.buttonStyle}
                    onClick={this.isDisabled ? undefined : this.$props.onClick}
                    onKeydown={this.handleKeyDown}
                    role="button"
                >
                    <SvgIcon icon={this.$props.icon} color={this.$props.iconColor} size={this.$props.iconSize} />
                </div>
            </Tooltip>
        );
    },
});
