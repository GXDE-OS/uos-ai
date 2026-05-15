import { defineComponent, ref, nextTick, watch, onMounted } from "vue";
import "@/assets/styles/window/mainwindow/sidebar/components/ExpandableContainer.css";

export default defineComponent({
    name: "ExpandableContainer",
    props: {
        expanded: {
            type: Boolean,
            default: false,
        },
        // 拖拽重排会频繁改动隐藏区结构，此时关闭高度动画可避免抖动。
        disableTransition: {
            type: Boolean,
            default: false,
        },
    },
    setup(props) {
        const containerRef = ref<HTMLElement>();
        const contentRef = ref<HTMLElement>();

        const syncExpandedState = async (isExpanded: boolean, animate: boolean) => {
            if (!containerRef.value || !contentRef.value) {
                return;
            }

            const container = containerRef.value;

            if (!animate || props.disableTransition) {
                // 首次挂载或外部禁用动画时直接同步最终高度。
                container.style.height = isExpanded ? "auto" : "0px";
                return;
            }

            if (isExpanded) {
                // 展开前先读出自然高度，再从 0px 过渡到目标高度。
                container.style.height = "auto";
                const height = container.scrollHeight;
                container.style.height = "0px";

                await nextTick();

                container.style.height = `${height}px`;
                return;
            }

            // 收起时从当前内容高度过渡到 0px，避免 auto 无法直接参与动画。
            container.style.height = `${container.scrollHeight}px`;
            await nextTick();
            container.style.height = "0px";
        };

        onMounted(async () => {
            await nextTick();
            await syncExpandedState(props.expanded, false);
        });

        watch(
            () => props.expanded,
            async (isExpanded) => {
                await syncExpandedState(isExpanded, true);
            },
        );

        watch(
            () => props.disableTransition,
            async (disableTransition) => {
                if (disableTransition) {
                    // 进入禁用动画态时立刻校正高度，防止上一段过渡残留。
                    await syncExpandedState(props.expanded, false);
                }
            },
        );

        const handleTransitionEnd = () => {
            if (props.expanded && containerRef.value) {
                containerRef.value.style.height = "auto";
            }
        };

        return {
            containerRef,
            contentRef,
            handleTransitionEnd,
        };
    },
    render() {
        const contentClass = {
            "expandable-content": true,
            "expandable-content--expanded": this.expanded,
            "expandable-content--transition-disabled": this.disableTransition,
        };

        return (
            <div ref="containerRef" class={contentClass} onTransitionend={this.handleTransitionEnd}>
                <div ref="contentRef">{this.$slots.default?.()}</div>
            </div>
        );
    },
});
