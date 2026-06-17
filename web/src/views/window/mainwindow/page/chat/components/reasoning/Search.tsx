import { defineComponent, ref, watch, nextTick, onMounted, onBeforeUnmount, computed } from "vue";
import type { PropType } from "vue";
import type { WebSearchData, WebSearchContent } from "@/types/conversation";
import { WebSearchStatus } from "@/types/conversation";
import SvgIcon from "@/components/SvgIcon";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";
import { useBackendStore } from "@/stores";

type ExpandState = 'full' | 'collapsed';

export default defineComponent({
    name: "Search",

    props: {
        data: {
            type: Object as PropType<WebSearchData>,
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

        let scrollEl: HTMLElement | Window | null = null;
        let scrollHandler: (() => void) | null = null;
        let resizeHandler: (() => void) | null = null;
        let rafScheduled = false;

        const isOverflowing = () => {
            const el = contentRef.value;
            return el ? el.scrollHeight > el.clientHeight : false;
        };

        const findScrollParent = (el: HTMLElement): HTMLElement | null => {
            let cur: HTMLElement | null = el.parentElement;
            while (cur && cur !== document.body && cur !== document.documentElement) {
                const oy = getComputedStyle(cur).overflowY;
                if (oy === 'auto' || oy === 'scroll' || oy === 'overlay') return cur;
                cur = cur.parentElement;
            }
            return null;
        };

        const updateScrollState = () => {
            const blockEl = blockRef.value;
            if (!blockEl) return;
            const headerEl = blockEl.querySelector('.search-block__header') as HTMLElement | null;
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

        onMounted(() => {
            // 搜索完成时默认收起
            if (props.data.status === WebSearchStatus.COMPLETED ||
                props.data.status === WebSearchStatus.FAILED) {
                expandState.value = 'collapsed';
            }
        });
        onBeforeUnmount(teardownStickyObserver);

        watch(expandState, (state) => {
            if (state === 'full') nextTick(setupStickyObserver);
            else teardownStickyObserver();
        });

        // 搜索完成时自动收起（如果用户未手动操作）
        watch(() => props.data.status, (status, prevStatus) => {
            if ((status === WebSearchStatus.COMPLETED || status === WebSearchStatus.FAILED) &&
                !userManuallyToggled.value &&
                prevStatus !== WebSearchStatus.COMPLETED &&
                prevStatus !== WebSearchStatus.FAILED) {
                expandState.value = 'collapsed';
            }
        });

        const isSearching = computed(() => {
            return props.data.status === WebSearchStatus.SEARCHING;
        });

        const isReading = computed(() => {
            return props.data.status === WebSearchStatus.READING;
        });

        const isCompleted = computed(() => {
            return props.data.status === WebSearchStatus.COMPLETED;
        });

        const isFailed = computed(() => {
            return props.data.status === WebSearchStatus.FAILED;
        });

        const isDone = computed(() => {
            return isCompleted.value || isFailed.value;
        });

        const title = computed(() => {
            return props.data.title;
        });

        const openUrl = (url: string) => {
            void backendStore.requestSystem("openUrl", url);
        };

        return {
            expandState,
            toggle,
            contentRef,
            blockRef,
            isStuck,
            isReleased,
            title,
            isOverflowing,
            isSearching,
            isReading,
            isCompleted,
            isFailed,
            isDone,
            openUrl,
            WebSearchStatus,
        };
    },

    render() {
        const { content, status } = this.$props.data;
        const canExpand = !!content && content.length > 0;
        const isFull = this.expandState === 'full';
        const isCollapsed = this.expandState === 'collapsed';
        // 搜索完成/失败且展开时才显示内容
        const showBody = this.isDone && canExpand && !isCollapsed;
        // 搜索中时 compact，阅读中/搜索完成后根据展开状态决定
        const isCompact = this.isSearching || (this.isDone && isCollapsed);

        return (
            <div ref="blockRef" class={["search-block", isCompact && "search-block--compact", isFull && "search-block--expanded", isFull && this.isStuck && "search-block--stuck", isFull && this.isReleased && "search-block--released", this.isReading && "search-block--reading", this.isCompleted && "search-block--done", this.isFailed && "search-block--failed"]}>

                <div class="search-block__header" onClick={this.isDone && canExpand ? this.toggle : undefined}>
                    <span class="search-block__icon-wrapper">
                        <SvgIcon icon="search" size={[16, 16]} class="search-block__icon" />
                    </span>
                    <span class="search-block__title">{this.title}</span>
                    {this.isDone && canExpand && isCollapsed && (
                        <IconButton
                            class="search-block__chevron"
                            icon="mdeditor-fullscreen"
                            iconSize={[16, 16]}
                            size={30}
                            shape={ButtonShape.Rounded}
                        />
                    )}
                    {this.isDone && canExpand && isFull && (
                        <IconButton
                            class="search-block__chevron"
                            icon="mdeditor-exit-fullscreen"
                            iconSize={[16, 16]}
                            size={30}
                            shape={ButtonShape.Rounded}
                        />
                    )}
                </div>

                {showBody && (
                    <div class="search-block__body search-block__body--full" ref="contentRef">
                        <div class="search-block__content">
                            {content?.map((item: WebSearchContent, index: number) => (
                                <a
                                    key={index}
                                    class="search-result-item"
                                    href={item.url}
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    onClick={(e: Event) => { e.preventDefault(); this.openUrl(item.url); }}
                                >
                                    <span class="search-result-item__text">{index + 1}. {item.title}</span>
                                    <SvgIcon icon="web-link" size={[16, 16]} class="search-result-item__arrow" />
                                </a>
                            ))}
                        </div>
                    </div>
                )}
            </div>
        );
    },
});
