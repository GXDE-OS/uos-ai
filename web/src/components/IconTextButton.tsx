import { defineComponent, computed, type PropType, type CSSProperties } from "vue";
import SvgIcon from "./SvgIcon";
import Tooltip from "./Tooltip";

export default defineComponent({
    name: "IconTextButton",
    inheritAttrs: true,
    props: {
        icon: {
            type: String,
            required: true,
        },
        text: {
            type: String,
            required: true,
        },
        checked: {
            type: Boolean,
            default: false,
        },
        disabled: {
            type: Boolean,
            default: false,
        },
        iconColor: {
            type: String,
            default: undefined,
        },
        iconSize: {
            type: Array as () => [number, number],
            default: undefined,
        },
        width: {
            type: Number,
            default: undefined,
        },
        height: {
            type: Number,
            default: 30,
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
    setup(props, { attrs }) {
        const isChecked = computed(() => props.checked);
        const isDisabled = computed(() => props.disabled);

        const buttonStyle = computed((): CSSProperties => {
            const style: CSSProperties = {
                height: `${props.height}px`,
            };

            if (props.width !== undefined) {
                style.width = `${props.width}px`;
            }

            return style;
        });

        return () => {
            const buttonContent = (
                <div
                    {...attrs}
                    class={[
                        "icon-text-button",
                        isChecked.value && "icon-text-button-checked",
                        isDisabled.value && "icon-text-button-disabled",
                    ]}
                    style={buttonStyle.value}
                    onClick={isDisabled.value ? undefined : props.onClick}
                >
                    <SvgIcon icon={props.icon} color={props.iconColor} size={props.iconSize} />
                    <span class="icon-text-button-text">{props.text}</span>
                </div>
            );

            // 如果有 tooltip，用 Tooltip 包裹
            if (props.tooltip !== undefined) {
                return (
                    <Tooltip content={props.tooltip}>
                        {buttonContent}
                    </Tooltip>
                );
            }

            return buttonContent;
        };
    },
});
