import { computed, defineComponent, type CSSProperties, type PropType } from "vue";
import Tooltip from "@/components/Tooltip";
import SvgIcon from "@/components/SvgIcon";

export default defineComponent({
    name: "TitleButton",
    inheritAttrs: true,

    props: {
        icon: {
            type: String,
            required: true,
        },
        iconSize: {
            type: Array as () => [number, number],
            default: () => [16, 16],
        },
        size: {
            type: [Array, Number] as PropType<[number, number] | number>,
            default: 40,
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
    },

    setup(props) {
        const buttonStyle = computed((): CSSProperties => {
            if (Array.isArray(props.size)) {
                return {
                    width: `${props.size[0]}px`,
                    height: `${props.size[1]}px`,
                };
            }

            return {
                width: `${props.size}px`,
                height: `${props.size}px`,
            };
        });

        const handleKeyDown = (event: KeyboardEvent) => {
            if ((event.key !== "Enter" && event.key !== " ") || props.disabled) {
                return;
            }

            event.preventDefault();
            props.onClick?.(event as unknown as MouseEvent);
        };

        return {
            buttonStyle,
            handleKeyDown,
        };
    },

    render() {
        return (
            <Tooltip content={this.$props.tooltip || ""} showAfter={1000} disabled={this.$props.tooltip === undefined}>
                <div
                    {...this.$attrs}
                    class={["title-button", this.$props.disabled && "title-button--disabled"]}
                    style={this.buttonStyle}
                    onClick={this.$props.disabled ? undefined : this.$props.onClick}
                    onKeydown={this.handleKeyDown}
                    role="button"
                    aria-disabled={this.$props.disabled ? "true" : undefined}
                >
                    <SvgIcon icon={this.$props.icon} size={this.$props.iconSize} />
                </div>
            </Tooltip>
        );
    },
});
