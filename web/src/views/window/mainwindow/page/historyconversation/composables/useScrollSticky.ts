import { ref, onMounted, onUnmounted } from "vue";
import { debounce } from "@/utils/execution-control";

/**
 * 分组滚动吸附组合式函数
 * @param containerRef 列表容器DOM元素
 * @param groupElements 分组标题DOM元素集合
 * @returns 当前吸附的分组标签和是否吸附的状态
 */
export function useScrollSticky(containerRef: { value: HTMLElement | null }, groupElements: { value: HTMLElement[] }) {
    const stickyLabel = ref("");
    const isSticky = ref(false);

    // 防抖处理滚动事件
    const handleScroll = debounce(() => {
        if (!containerRef.value || groupElements.value.length === 0) {
            return;
        }

        const container = containerRef.value;
        const containerRect = container.getBoundingClientRect();
        const scrollTop = container.scrollTop;

        // 找到当前可视区域内的分组标题
        let currentLabel = "";
        let foundSticky = false;

        for (let i = 0; i < groupElements.value.length; i++) {
            const groupElement = groupElements.value[i];
            if (!groupElement) continue;

            const groupRect = groupElement.getBoundingClientRect();

            // 判断分组标题是否进入可视区域
            if (groupRect.top <= containerRect.top + 50) {
                currentLabel = groupElement.textContent || "";
                foundSticky = true;
            } else {
                break;
            }
        }

        stickyLabel.value = currentLabel;
        isSticky.value = foundSticky;
    }, 100);

    const setupScrollListener = () => {
        if (containerRef.value) {
            containerRef.value.addEventListener("scroll", handleScroll);
        }
    };

    const removeScrollListener = () => {
        if (containerRef.value) {
            containerRef.value.removeEventListener("scroll", handleScroll);
        }
    };

    onMounted(() => {
        setupScrollListener();
        // 初始化时执行一次
        handleScroll();
    });

    onUnmounted(() => {
        removeScrollListener();
    });

    return {
        stickyLabel,
        isSticky,
    };
}
