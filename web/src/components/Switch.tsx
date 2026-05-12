import { computed, defineComponent, getCurrentInstance, ref } from "vue";

export default defineComponent({
    name: "Switch",

    props: {
        value: {
            type: Boolean,
            default: undefined,
        },
        defaultValue: {
            type: Boolean,
            default: false,
        },
        disabled: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        change: (_value: boolean, _event?: Event) => true,
    },

    setup(props, { emit }) {
        const instance = getCurrentInstance();
        const innerValue = ref(props.defaultValue);

        const isControlled = computed(() => {
            const rawProps = instance?.vnode.props || {};
            return Object.prototype.hasOwnProperty.call(rawProps, "value");
        });

        const currentValue = computed(() => {
            return isControlled.value ? props.value : innerValue.value;
        });

        const switchClass = computed(() => {
            return [
                "switch-control",
                currentValue.value && "switch-control--checked",
                props.disabled && "switch-control--disabled",
            ];
        });

        const toggleValue = (event?: Event) => {
            if (props.disabled) {
                return;
            }

            const nextValue = !currentValue.value;

            if (!isControlled.value) {
                innerValue.value = nextValue;
            }

            emit("change", nextValue, event);
        };

        const handleClick = (event: MouseEvent) => {
            toggleValue(event);
        };

        return {
            currentValue,
            switchClass,
            handleClick,
        };
    },

    render() {
        return (
            <div class={this.switchClass} role="switch" onClick={this.handleClick}>
                <span class="switch-control__thumb" />
            </div>
        );
    },
});
