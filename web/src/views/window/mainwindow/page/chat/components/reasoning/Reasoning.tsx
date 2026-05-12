import { defineComponent, ref, watch, nextTick, onMounted, onBeforeUnmount, computed } from "vue";
import type { PropType } from "vue";
import type { ReasoningData } from "@/types/conversation";
import SvgIcon from "@/components/SvgIcon";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";
import { useBackendStore, useMainWindowStore } from "@/stores";
import deepThinkingWebp from "@/assets/video/deep-thinking.webp";
import deepThinkingDarkWebp from "@/assets/video/deep-thinking-dark.webp";

type ExpandState = 'thinking' | 'full' | 'collapsed';

export default defineComponent({
    name: "Reasoning",

    props: {
        data: {
            type: Object as PropType<ReasoningData>,
            required: true,
        },
    },

    setup(props) {
        const expandState = ref<ExpandState>('collapsed');
        const contentRef = ref<HTMLElement | null>(null);
        const blockRef = ref<HTMLElement | null>(null);
        const isStuck = ref(false);
        const isReleased = ref(false);
        const userManuallyToggled = ref(false);
        const backendStore = useBackendStore();
        const mainWindowStore = useMainWindowStore();

        let scrollEl: HTMLElement | Window | null = null;
        let scrollHandler: (() => void) | null = null;
        let resizeHandler: (() => void) | null = null;
        let rafScheduled = false;

        const isOverflowing = () => {
            const el = contentRef.value;
            return el ? el.scrollHeight > el.clientHeight : false;
        };

        // 沿 DOM 向上找最近的滚动祖先；都不是则回落到 window
        const findScrollParent = (el: HTMLElement): HTMLElement | null => {
            let cur: HTMLElement | null = el.parentElement;
            while (cur && cur !== document.body && cur !== document.documentElement) {
                const oy = getComputedStyle(cur).overflowY;
                if (oy === 'auto' || oy === 'scroll' || oy === 'overlay') return cur;
                cur = cur.parentElement;
            }
            return null;
        };

        // 直接用 CSS sticky 计算后的真实矩形判定，不依赖对 chat 滚动容器顶坐标的推算：
        // - 吸附中：header 被 sticky 从自然位置（block.top）推下来了
        // - 已释放：header 被推到 block 底部，即将随 block 整体上移
        const updateScrollState = () => {
            const blockEl = blockRef.value;
            if (!blockEl) return;
            const headerEl = blockEl.querySelector('.reasoning-block__header') as HTMLElement | null;
            if (!headerEl) return;

            const blockRect = blockEl.getBoundingClientRect();
            const headerRect = headerEl.getBoundingClientRect();

            isStuck.value = headerRect.top > blockRect.top + 0.5;
            isReleased.value = headerRect.bottom >= blockRect.bottom - 0.5;
        };

        const onScroll = () => {
            if (rafScheduled) return;
            rafScheduled = true;
            requestAnimationFrame(() => {
                rafScheduled = false;
                updateScrollState();
            });
        };

        const teardownStickyObserver = () => {
            if (scrollHandler && scrollEl) {
                scrollEl.removeEventListener('scroll', scrollHandler);
            }
            if (resizeHandler) {
                window.removeEventListener('resize', resizeHandler);
            }
            scrollHandler = null;
            resizeHandler = null;
            scrollEl = null;
            isStuck.value = false;
            isReleased.value = false;
        };

        const setupStickyObserver = () => {
            teardownStickyObserver();
            const blockEl = blockRef.value;
            if (!blockEl) return;

            scrollEl = findScrollParent(blockEl) || window;
            scrollHandler = onScroll;
            resizeHandler = onScroll;
            scrollEl.addEventListener('scroll', scrollHandler, { passive: true });
            window.addEventListener('resize', resizeHandler, { passive: true });
            updateScrollState();
        };

        const toggle = () => {
            userManuallyToggled.value = true;
            if (expandState.value === 'full') {
                expandState.value = 'collapsed';
            } else {
                expandState.value = 'full';
            }
        };

        const scrollToBottom = () => {
            nextTick(() => {
                const el = contentRef.value;
                if (el) el.scrollTop = el.scrollHeight;
            });
        };

        onMounted(() => {
            // 初始状态：思考中则展开
            if (props.data.status === 0) {
                expandState.value = 'thinking';
            }
            scrollToBottom();
        });
        onBeforeUnmount(teardownStickyObserver);

        watch(() => props.data.reasoning_content, () => {
            if (expandState.value === 'thinking') scrollToBottom();
        });

        watch(expandState, (state) => {
            if (state === 'full') nextTick(setupStickyObserver);
            else teardownStickyObserver();
        });

        // 思考开始时自动展开，完成时若用户未手动操作则自动收起
        watch(() => props.data.status, (status, prevStatus) => {
            if (status === 0 && prevStatus !== 0 && expandState.value === 'collapsed') {
                expandState.value = 'thinking';
                userManuallyToggled.value = false;
            } else if (status === 1 && expandState.value === 'thinking' && !userManuallyToggled.value) {
                expandState.value = 'collapsed';
            }
        });

        const formatElapsed = (sec: number) => {
            if (sec < 60) return `${sec}`;
            return `${Math.floor(sec / 60) * 60 + sec % 60}`;
        };

        const title = computed(() => {
            const { status, elapsed } = props.data;
            const isDone = status === 1;
            if (isDone) {
                const elapsedStr = elapsed != null ? formatElapsed(elapsed) : "";
                return backendStore.translate("Deep think completed (took %1s)").replace("%1", elapsedStr);
            }
            return backendStore.translate("Thinking...");
        });

        const thinkingWebp = computed(() =>
            mainWindowStore.isDarkMode ? deepThinkingDarkWebp : deepThinkingWebp
        );

        return { expandState, toggle, contentRef, blockRef, isStuck, isReleased, title, isOverflowing, thinkingWebp };
    },

    render() {
        const { reasoning_content: content, status } = this.$props.data;
        const canExpand = !!content;
        const isFull = this.expandState === 'full';
        const isCollapsed = this.expandState === 'collapsed';
        const showBody = canExpand && !isCollapsed;
        const isCompact = !canExpand || isCollapsed;
        const isDone = status === 1;

        return (
            <div ref="blockRef" class={["reasoning-block", isCompact && "reasoning-block--compact", isFull && "reasoning-block--expanded", isFull && this.isStuck && "reasoning-block--stuck", isFull && this.isReleased && "reasoning-block--released", isDone && "reasoning-block--done"]}>

                <div class="reasoning-block__header" onClick={canExpand ? this.toggle : undefined}>
                    <span class="reasoning-block__icon-wrapper">
                        <img src={this.thinkingWebp} class="reasoning-block__icon-anim"/>
                        <SvgIcon icon="deep-think" size={[16, 16]} class="reasoning-block__icon" />
                    </span>
                    <span class="reasoning-block__title">{this.title}</span>
                    {canExpand && (isCollapsed || (this.expandState === 'thinking' && this.isOverflowing())) && (
                        <IconButton
                            class="reasoning-block__chevron"
                            icon="mdeditor-fullscreen"
                            iconSize={[16, 16]}
                            size={24}
                            shape={ButtonShape.Rounded}
                        />
                    )}
                    {canExpand && (isFull || (this.expandState === 'thinking' && !this.isOverflowing())) && (
                        <IconButton
                            class="reasoning-block__chevron"
                            icon="mdeditor-exit-fullscreen"
                            iconSize={[16, 16]}
                            size={30}
                            shape={ButtonShape.Rounded}
                        />
                    )}
                </div>

                {showBody && (
                    <div class={["reasoning-block__body", isFull ? "reasoning-block__body--full" : "reasoning-block__body--thinking"]} ref="contentRef"
                        onWheel={!isFull ? (e: WheelEvent) => {
                            const el = e.currentTarget as HTMLElement;
                            const atTop = el.scrollTop <= 0;
                            const atBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 1;
                            if ((e.deltaY < 0 && atTop) || (e.deltaY > 0 && atBottom)) return;
                            e.stopPropagation();
                        } : undefined}>
                        <div class="reasoning-block__content">{content?.trimStart()}</div>
                    </div>
                )}
            </div>
        );
    },
});
