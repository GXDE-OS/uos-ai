import { computed, defineComponent } from "vue";
import SvgIcon from "@/components/SvgIcon";

export default defineComponent({
    name: "CheckButton",

    props: {
        checked: {
            type: Boolean,
            default: false,
        },
        indeterminate: {
            type: Boolean,
            default: false,
        },
        disabled: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        change: (_checked: boolean, _event?: Event) => true,
    },

    setup(props, { emit }) {
        const currentChecked = computed(() => {
            return props.checked;
        });

        const currentIndeterminate = computed(() => {
            return props.indeterminate;
        });

        const checkButtonClass = computed(() => {
            return [
                "check-button",
                currentIndeterminate.value && "check-button--indeterminate",
                currentChecked.value && !currentIndeterminate.value && "check-button--checked",
                props.disabled && "check-button--disabled",
            ];
        });

        const toggleChecked = (event?: Event) => {
            if (props.disabled) {
                return;
            }

            const nextChecked = currentIndeterminate.value ? true : !currentChecked.value;
            emit("change", nextChecked, event);
        };

        const handleClick = (event: MouseEvent) => {
            event.stopPropagation();
            toggleChecked(event);
        };

        return {
            currentChecked,
            currentIndeterminate,
            checkButtonClass,
            handleClick,
        };
    },

    render() {
        return (
            <div class={this.checkButtonClass} role="checkbox" onClick={this.handleClick}>
                <span class="check-button__box">
                    {this.currentIndeterminate ? (
                        <span class="check-button__indeterminate" />
                    ) : (
                        this.currentChecked && <SvgIcon icon="checked" size={[20, 20]} class="check-button__icon" />
                    )}
                </span>
            </div>
        );
    },
});
