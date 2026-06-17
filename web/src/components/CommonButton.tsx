import { defineComponent, mergeProps, type PropType } from "vue";

export type CommonButtonVariant = "default" | "danger" | "primary" | "filled";

export default defineComponent({
    name: "CommonButton",
    inheritAttrs: false,

    props: {
        text: {
            type: String,
            required: true,
        },
        variant: {
            type: String as PropType<CommonButtonVariant>,
            default: "default",
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
        const triggerClick = (event: MouseEvent | KeyboardEvent) => {
            if (props.disabled) {
                return;
            }

            if (event instanceof MouseEvent) {
                props.onClick?.(event);
                return;
            }

            props.onClick?.(new MouseEvent("click"));
        };

        const handleClick = (event: MouseEvent) => {
            if (props.disabled) {
                event.preventDefault();
                event.stopPropagation();
                return;
            }

            triggerClick(event);
        };

        const handleKeydown = (event: KeyboardEvent) => {
            if (props.disabled) {
                return;
            }

            if (event.key === "Enter") {
                event.preventDefault();
                triggerClick(event);
                return;
            }

            if (event.key !== " ") {
                return;
            }

            event.preventDefault();
        };

        const handleKeyup = (event: KeyboardEvent) => {
            if (props.disabled) {
                return;
            }

            if (event.key !== " ") {
                return;
            }

            triggerClick(event);
        };

        return {
            handleClick,
            handleKeydown,
            handleKeyup,
        };
    },

    render() {
        return (
            <div
                {...mergeProps(this.$attrs, {
                    class: [
                        "common-button",
                        `common-button--${this.$props.variant}`,
                        this.$props.disabled && "common-button--disabled",
                    ],
                    role: "button",
                    onClick: this.handleClick,
                    onKeydown: this.handleKeydown,
                    onKeyup: this.handleKeyup,
                })}
            >
                <span class="common-button__text">{this.$props.text}</span>
            </div>
        );
    },
});
