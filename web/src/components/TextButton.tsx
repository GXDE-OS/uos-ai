import { defineComponent, type PropType } from "vue";

export default defineComponent({
    name: "TextButton",
    inheritAttrs: false,

    props: {
        text: {
            type: String,
            required: true,
        },
        disabled: {
            type: Boolean,
            default: false,
        },
        onClick: {
            type: Function as PropType<(event: MouseEvent) => void>,
            default: undefined,
        },
    },

    setup(props) {
        const handleClick = (event: MouseEvent) => {
            if (props.disabled) {
                event.preventDefault();
                event.stopPropagation();
                return;
            }
            props.onClick?.(event);
        };

        return { handleClick };
    },

    render() {
        return (
            <div
                {...this.$attrs}
                class={[
                    "text-button",
                    this.disabled && "text-button--disabled",
                ]}
                onClick={this.handleClick}
            >
                <span class="text-button__text">{this.text}</span>
            </div>
        );
    },
});
