import { defineComponent } from "vue";

export default defineComponent({
    name: "Splitter",
    props: {
        disabled: {
            type: Boolean,
            default: false,
        },
        handleWidth: {
            type: Number,
            default: 2,
        },
    },
    emits: ["resize-start"],
    setup(props, { emit }) {
        const handleMouseDown = (e: MouseEvent) => {
            if (props.disabled) return;
            emit("resize-start", e);
        };

        return {
            handleMouseDown,
        };
    },
    render() {
        return (
            <div
                class={[
                    "splitter",
                    { "splitter-disabled": this.$props.disabled },
                ]}
                style={{ width: `${this.$props.handleWidth}px` }}
                onMousedown={this.handleMouseDown}
            >
                <div class="splitter-handle" />
            </div>
        );
    },
});
