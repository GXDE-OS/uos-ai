import { defineComponent, computed, type CSSProperties } from "vue";

export default defineComponent({
    name: "SvgIcon",
    inheritAttrs: true,
    props: {
        icon: {
            type: String,
            required: true,
        },
        color: {
            type: String,
            default: undefined,
        },
        size: {
            type: Array as () => [number, number],
            default: undefined,
        },
    },
    setup(props, { attrs }) {
        const iconName = computed(() => `#icon-${props.icon}`);

        const svgClass = computed(() => {
            return props.icon ? `svg-icon icon-${props.icon}` : "svg-icon";
        });

        const svgStyle = computed((): CSSProperties => {
            const style: CSSProperties = {
                color: props.color || undefined,
            };

            if (props.size) {
                style.width = `${props.size[0]}px`;
                style.height = `${props.size[1]}px`;
            }

            return style;
        });

        return () => (
            <svg {...attrs} class={svgClass.value} style={svgStyle.value} aria-hidden="true">
                <use xlinkHref={iconName.value} />
            </svg>
        );
    },
});
