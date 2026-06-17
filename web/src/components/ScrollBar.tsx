import { defineComponent, ref, onMounted, onUnmounted, nextTick, computed, watch } from "vue";

const FRAME_DURATION = 1000 / 60;
const MOMENTUM_FORCE = 0.22;
const MOMENTUM_DAMPING = 0.88;
const MOMENTUM_MIN_DELTA = 18;
const MOMENTUM_MIN_VELOCITY = 0.12;
const MOMENTUM_MAX_VELOCITY = 28;
const EDGE_BOUNCE_FORCE = 0.12;
const EDGE_BOUNCE_SPRING = 0.12;
const EDGE_BOUNCE_DAMPING = 0.7;
const EDGE_BOUNCE_MAX_OFFSET = 64;
const EDGE_BOUNCE_SETTLE_THRESHOLD = 0.2;
// 释放 thumb 后短时间屏蔽 track click，避免浏览器补发 click 导致滚动跳跃。
const TRACK_CLICK_SUPPRESS_DURATION = 120;

export default defineComponent({
    name: "ScrollBar",
    props: {
        // 滚动方向
        direction: {
            type: String as () => "vertical" | "horizontal",
            default: "vertical",
        },
        // 是否自动隐藏
        autoHide: {
            type: Boolean,
            default: true,
        },
        // 自定义滚动条颜色
        thumbColor: {
            type: String,
            default: "",
        },
        // 自定义轨道颜色
        trackColor: {
            type: String,
            default: "",
        },
        // 是否开启滚轮惯性
        momentum: {
            type: Boolean,
            default: false,
        },
        // 额外扣减的最大滚动距离
        maxScrollOffset: {
            type: Number,
            default: 0,
        },
        // 是否开启边界回弹
        edgeBounce: {
            type: Boolean,
            default: false,
        },
    },
    emits: ["scroll"],
    setup(props, { emit }) {
        const scrollContainerRef = ref<HTMLElement | null>(null);
        const scrollContentInnerRef = ref<HTMLElement | null>(null);
        const thumbRef = ref<HTMLElement | null>(null);
        const trackRef = ref<HTMLElement | null>(null);

        const isDragging = ref(false);
        const isHovering = ref(false);
        const isScrolling = ref(false);
        const scrollPosition = ref(0);
        const scrollSize = ref(0);
        const containerSize = ref(0);
        const overscrollOffset = ref(0);

        let dragStartPosition = 0;
        let dragStartScroll = 0;
        let resizeObserver: ResizeObserver | null = null;
        let animationFrame: number | null = null;
        let lastAnimationTime = 0;
        let scrollVelocity = 0;
        let overscrollVelocity = 0;
        let pendingProgrammaticScroll = false;
        let hideScrollBarTimer: number | null = null;
        // 延迟刷新 thumb，等待滚动条 DOM 渲染完成后再读取尺寸。
        let pendingThumbUpdate = false;
        let thumbUpdateFrame: number | null = null;
        // 拖拽释放后用于吞掉浏览器补发到 track 的 click。
        let suppressTrackClick = false;
        let suppressTrackClickTimer: number | null = null;
        // nextTick / requestAnimationFrame 回调里用来避免卸载后继续访问 DOM。
        let isUnmounted = false;

        // 动画滚动相关状态
        let animatedScrollStartTime = 0;
        let animatedScrollStartPos = 0;
        let animatedScrollTargetPos = 0;
        let animatedScrollDuration = 0;

        /**
         * 计算有效的滚动尺寸，扣除额外的最大滚动偏移量
         * 用于处理某些场景下实际滚动内容比可见滚动区域小的情况
         */
        const getEffectiveScrollSize = () => Math.max(containerSize.value, scrollSize.value - props.maxScrollOffset);

        /**
         * 获取最大可滚动距离
         * @returns 最大滚动像素值，如果内容不足一屏则返回 0
         */
        const getMaxScroll = () => Math.max(0, getEffectiveScrollSize() - containerSize.value);

        /**
         * 清除滚动条自动隐藏计时器
         */
        const clearHideScrollBarTimer = () => {
            if (hideScrollBarTimer !== null) {
                window.clearTimeout(hideScrollBarTimer);
                hideScrollBarTimer = null;
            }
        };

        /**
         * 标记滚动条为滚动中可见状态
         */
        const showScrollBarWhileScrolling = () => {
            if (!props.autoHide) {
                return;
            }

            clearHideScrollBarTimer();
            isScrolling.value = true;
        };

        /**
         * 滚动停止后延迟隐藏滚动条
         */
        const scheduleHideScrollBar = () => {
            if (!props.autoHide) {
                return;
            }

            clearHideScrollBarTimer();
            hideScrollBarTimer = window.setTimeout(() => {
                hideScrollBarTimer = null;

                if (!isHovering.value && !isDragging.value) {
                    isScrolling.value = false;
                }
            }, 700);
        };

        /**
         * 获取滚动条轨道的实际可视尺寸
         * 当样式对滚动条做了 top/bottom 或 left/right 内缩时，
         * 轨道长度会小于滚动容器尺寸，thumb 的计算需要以真实轨道为准。
         */
        const getTrackSize = () => {
            if (!trackRef.value) {
                return containerSize.value;
            }

            return props.direction === "vertical" ? trackRef.value.clientHeight : trackRef.value.clientWidth;
        };

        /**
         * 基于当前内容尺寸和轨道尺寸计算 thumb 相关几何信息
         */
        const getThumbMetrics = () => {
            const effectiveScrollSize = getEffectiveScrollSize();
            const trackSize = getTrackSize();
            const thumbSize = Math.min(
                trackSize,
                Math.max((containerSize.value / effectiveScrollSize) * trackSize, 20),
            );
            const trackTravel = Math.max(trackSize - thumbSize, 0);

            return {
                trackSize,
                thumbSize,
                trackTravel,
            };
        };

        /**
         * 将过滚动偏移量限制在边界回弹的最大范围内
         * @param value - 原始偏移量
         * @returns 限制后的偏移量，范围为 ±EDGE_BOUNCE_MAX_OFFSET
         */
        const clampOverscrollOffset = (value: number) =>
            Math.max(-EDGE_BOUNCE_MAX_OFFSET, Math.min(EDGE_BOUNCE_MAX_OFFSET, value));

        /**
         * 计算内部内容容器的 transform 样式
         * 用于实现边界回弹效果的视觉反馈
         * @returns 当有过滚动偏移时返回 transform 样式对象，否则返回 undefined
         */
        const scrollContentInnerStyle = computed(() => {
            if (Math.abs(overscrollOffset.value) < 0.1) {
                return undefined;
            }

            const transform =
                props.direction === "vertical"
                    ? `translate3d(0, ${overscrollOffset.value}px, 0)`
                    : `translate3d(${overscrollOffset.value}px, 0, 0)`;

            return {
                transform,
            } as Record<string, string>;
        });

        /**
         * 计算滚动容器和内容尺寸
         * 根据滚动方向获取 scrollHeight/scrollWidth 和 clientHeight/clientWidth
         * 并确保滚动位置不超过最大可滚动距离
         */
        const calculateSizes = () => {
            if (!scrollContainerRef.value) {
                return;
            }

            const container = scrollContainerRef.value;

            if (props.direction === "vertical") {
                scrollSize.value = container.scrollHeight;
                containerSize.value = container.clientHeight;
                scrollPosition.value = container.scrollTop;
            } else {
                scrollSize.value = container.scrollWidth;
                containerSize.value = container.clientWidth;
                scrollPosition.value = container.scrollLeft;
            }

            const maxScroll = getMaxScroll();
            if (scrollPosition.value > maxScroll) {
                pendingProgrammaticScroll = true;
                if (props.direction === "vertical") {
                    container.scrollTop = maxScroll;
                } else {
                    container.scrollLeft = maxScroll;
                }
                scrollPosition.value = maxScroll;
            }
        };

        /**
         * 更新滚动滑块（thumb）的位置和大小
         * 根据滚动位置百分比计算滑块在轨道中的位置
         * 并设置滑块的最小尺寸为 20px
         */
        const updateThumb = () => {
            if (!thumbRef.value) {
                return;
            }

            if (scrollSize.value <= containerSize.value) {
                if (props.direction === "vertical") {
                    thumbRef.value.style.height = "";
                    thumbRef.value.style.top = "0";
                } else {
                    thumbRef.value.style.width = "";
                    thumbRef.value.style.left = "0";
                }
                return;
            }

            const { thumbSize, trackTravel } = getThumbMetrics();
            const maxScroll = getMaxScroll();
            const thumbPosition = maxScroll === 0 ? 0 : (scrollPosition.value / maxScroll) * trackTravel;

            if (props.direction === "vertical") {
                thumbRef.value.style.height = `${thumbSize}px`;
                thumbRef.value.style.top = `${thumbPosition}px`;
            } else {
                thumbRef.value.style.width = `${thumbSize}px`;
                thumbRef.value.style.left = `${thumbPosition}px`;
            }
        };

        /**
         * 取消延迟的 thumb 更新任务
         */
        const cancelScheduledThumbUpdate = () => {
            pendingThumbUpdate = false;

            if (thumbUpdateFrame !== null) {
                cancelAnimationFrame(thumbUpdateFrame);
                thumbUpdateFrame = null;
            }
        };

        /**
         * 拖拽 thumb 后浏览器可能补发一次 click 到 track，需要丢弃这次点击
         */
        const suppressNextTrackClick = () => {
            suppressTrackClick = true;

            if (suppressTrackClickTimer !== null) {
                window.clearTimeout(suppressTrackClickTimer);
            }

            suppressTrackClickTimer = window.setTimeout(() => {
                suppressTrackClick = false;
                suppressTrackClickTimer = null;
            }, TRACK_CLICK_SUPPRESS_DURATION);
        };

        const clearSuppressTrackClickTimer = () => {
            if (suppressTrackClickTimer !== null) {
                window.clearTimeout(suppressTrackClickTimer);
                suppressTrackClickTimer = null;
            }

            // 组件卸载或清理时恢复默认 track 点击行为。
            suppressTrackClick = false;
        };

        /**
         * 重新计算尺寸，并在 DOM 渲染完成后更新 thumb
         *
         * 首次计算 scrollSize/containerSize 可能会让滚动条节点从不渲染变为渲染，
         * 因此需要等 nextTick 后再读取 thumbRef/trackRef。
         */
        const scheduleThumbUpdate = () => {
            calculateSizes();

            if (pendingThumbUpdate) {
                return;
            }

            pendingThumbUpdate = true;

            void nextTick(() => {
                // 等 Vue 把滚动条节点渲染出来后，第一次更新 thumb。
                if (isUnmounted) {
                    return;
                }

                pendingThumbUpdate = false;
                updateThumb();

                if (thumbUpdateFrame !== null) {
                    cancelAnimationFrame(thumbUpdateFrame);
                }

                thumbUpdateFrame = requestAnimationFrame(() => {
                    thumbUpdateFrame = null;

                    // 下一帧再兜底一次，处理布局高度在首次渲染后继续变化的场景。
                    if (isUnmounted) {
                        return;
                    }

                    calculateSizes();
                    updateThumb();
                });
            });
        };

        /**
         * 停止所有运动动画（惯性和边界回弹）
         * @param resetOverscroll - 是否重置过滚动偏移和速度，默认为 false
         */
        const stopMotion = (resetOverscroll = false) => {
            if (animationFrame !== null) {
                cancelAnimationFrame(animationFrame);
                animationFrame = null;
            }
            lastAnimationTime = 0;
            scrollVelocity = 0;
            if (resetOverscroll) {
                overscrollVelocity = 0;
                overscrollOffset.value = 0;
            }
        };

        /**
         * 应用边界回弹冲量
         * 当滚动超出边界时，对过滚动偏移和速度施加冲量效果
         * @param delta - 原始滚动变化的 delta 值
         */
        const applyEdgeBounceImpulse = (delta: number) => {
            if (!props.edgeBounce || delta === 0) {
                return;
            }

            const impulse = Math.sign(delta) * Math.max(Math.abs(delta), MOMENTUM_MIN_DELTA) * EDGE_BOUNCE_FORCE;
            overscrollOffset.value = clampOverscrollOffset(overscrollOffset.value - impulse);
            overscrollVelocity -= impulse * 0.45;
        };

        /**
         * 启动动画帧循环，处理惯性滚动和边界回弹
         * 每帧计算新的位置和速度，直到运动完全停止
         */
        const scheduleAnimation = () => {
            if (animationFrame !== null) {
                return;
            }

            const step = (timestamp: number) => {
                if (!scrollContainerRef.value) {
                    stopMotion();
                    return;
                }

                const elapsed = lastAnimationTime === 0 ? FRAME_DURATION : timestamp - lastAnimationTime;
                const frameScale = Math.min(elapsed / FRAME_DURATION, 2);
                lastAnimationTime = timestamp;

                const maxScroll = getMaxScroll();
                const currentScroll =
                    props.direction === "vertical"
                        ? scrollContainerRef.value.scrollTop
                        : scrollContainerRef.value.scrollLeft;
                const nextRawScroll = currentScroll + scrollVelocity * frameScale;
                const nextScroll = Math.max(0, Math.min(nextRawScroll, maxScroll));

                if (Math.abs(nextScroll - currentScroll) > 0.1) {
                    // 注意：不设 pendingProgrammaticScroll。
                    // scheduleAnimation 仅被 handleWheel（用户滚轮）和边界回弹调用，
                    // 属于用户操作，不是外部 API 驱动的程序滚动。
                    if (props.direction === "vertical") {
                        scrollContainerRef.value.scrollTop = nextScroll;
                    } else {
                        scrollContainerRef.value.scrollLeft = nextScroll;
                    }
                }

                if (nextScroll !== nextRawScroll) {
                    applyEdgeBounceImpulse(nextRawScroll - nextScroll);
                    scrollVelocity = 0;
                } else {
                    scrollVelocity *= Math.pow(MOMENTUM_DAMPING, frameScale);
                }

                if (props.edgeBounce) {
                    overscrollVelocity += (0 - overscrollOffset.value) * EDGE_BOUNCE_SPRING * frameScale;
                    overscrollVelocity *= Math.pow(EDGE_BOUNCE_DAMPING, frameScale);
                    overscrollOffset.value = clampOverscrollOffset(
                        overscrollOffset.value + overscrollVelocity * frameScale,
                    );

                    if (
                        Math.abs(overscrollOffset.value) < EDGE_BOUNCE_SETTLE_THRESHOLD &&
                        Math.abs(overscrollVelocity) < EDGE_BOUNCE_SETTLE_THRESHOLD
                    ) {
                        overscrollOffset.value = 0;
                        overscrollVelocity = 0;
                    }
                } else {
                    overscrollOffset.value = 0;
                    overscrollVelocity = 0;
                }

                const isScrollSettled = Math.abs(scrollVelocity) < MOMENTUM_MIN_VELOCITY;
                const isBounceSettled =
                    !props.edgeBounce ||
                    (Math.abs(overscrollOffset.value) < EDGE_BOUNCE_SETTLE_THRESHOLD &&
                        Math.abs(overscrollVelocity) < EDGE_BOUNCE_SETTLE_THRESHOLD);

                if (isScrollSettled && isBounceSettled) {
                    animationFrame = null;
                    lastAnimationTime = 0;
                    scrollVelocity = 0;
                    overscrollVelocity = 0;
                    overscrollOffset.value = 0;
                    return;
                }

                animationFrame = requestAnimationFrame(step);
            };

            animationFrame = requestAnimationFrame(step);
        };

        /**
         * 触发滚动事件，更新滑块并通知父组件
         * @param position - 当前滚动位置
         */
        const emitScroll = (position: number) => {
            updateThumb();
            emit("scroll", position);
        };

        /**
         * 处理滚动容器的原生滚动事件
         * 同步滚动位置并确保不超过最大滚动距离
         */
        const handleScroll = () => {
            if (!scrollContainerRef.value) {
                return;
            }

            showScrollBarWhileScrolling();

            const container = scrollContainerRef.value;
            const nextPosition = props.direction === "vertical" ? container.scrollTop : container.scrollLeft;
            const maxScroll = getMaxScroll();

            if (nextPosition > maxScroll) {
                pendingProgrammaticScroll = true;
                if (props.direction === "vertical") {
                    container.scrollTop = maxScroll;
                } else {
                    container.scrollLeft = maxScroll;
                }
                scrollPosition.value = maxScroll;
                emitScroll(maxScroll);
                scheduleHideScrollBar();
                return;
            }

            scrollPosition.value = nextPosition;

            // 先 emit，让消费者（如 ChatView）在回调中能读到 isProgrammaticScroll() 的值，
            // 之后再重置标志。否则消费者拿到的永远是 false。
            emitScroll(nextPosition);

            if (pendingProgrammaticScroll) {
                pendingProgrammaticScroll = false;
            }

            scheduleHideScrollBar();
        };

        /**
         * 标准化滚轮事件的 delta 值
         * 处理不同 deltaMode（像素、行、页面）的差异，统一转换为像素值
         * @param event - 滚轮事件对象
         * @returns 标准化后的 delta 像素值
         */
        const normalizeWheelDelta = (event: WheelEvent) => {
            const rawDelta = props.direction === "vertical" ? event.deltaY : event.deltaX;
            if (rawDelta === 0) {
                return 0;
            }

            if (event.deltaMode === WheelEvent.DOM_DELTA_LINE) {
                return rawDelta * 16;
            }

            if (event.deltaMode === WheelEvent.DOM_DELTA_PAGE) {
                return rawDelta * containerSize.value;
            }

            return rawDelta;
        };

        /**
         * 处理滚轮事件，实现惯性滚动和边界回弹
         * 将滚轮的 delta 转换为冲量并启动惯性动画
         */
        const handleWheel = (event: WheelEvent) => {
            if (!props.momentum) {
                return;
            }

            if (!scrollContainerRef.value || isDragging.value) {
                return;
            }

            calculateSizes();

            const maxScroll = getMaxScroll();
            if (maxScroll <= 0) {
                return;
            }

            const delta = normalizeWheelDelta(event);
            if (delta === 0) {
                return;
            }

            event.preventDefault();
            showScrollBarWhileScrolling();
            scheduleHideScrollBar();

            const isAtStart = scrollPosition.value <= 0.5;
            const isAtEnd = scrollPosition.value >= maxScroll - 0.5;
            const isPushingPastStart = delta < 0 && isAtStart;
            const isPushingPastEnd = delta > 0 && isAtEnd;

            if (props.edgeBounce && (isPushingPastStart || isPushingPastEnd)) {
                applyEdgeBounceImpulse(delta);
                scheduleAnimation();
                return;
            }

            const impulse = Math.sign(delta) * Math.max(Math.abs(delta), MOMENTUM_MIN_DELTA);
            scrollVelocity = Math.max(
                -MOMENTUM_MAX_VELOCITY,
                Math.min(MOMENTUM_MAX_VELOCITY, scrollVelocity + impulse * MOMENTUM_FORCE),
            );

            if (
                (scrollPosition.value <= 0 && scrollVelocity < 0) ||
                (scrollPosition.value >= maxScroll && scrollVelocity > 0)
            ) {
                scrollVelocity *= 0.4;
            }

            scheduleAnimation();
        };

        /**
         * 鼠标停在自定义滚动条浮层上时，原生滚动容器收不到 wheel。
         * 这里把滚动条区域的 wheel 转成内容滚动，保证 hover 滚动条时也能滚动。
         */
        const handleScrollBarWheel = (event: WheelEvent) => {
            if (!scrollContainerRef.value || isDragging.value) {
                return;
            }

            if (props.momentum) {
                handleWheel(event);
                return;
            }

            calculateSizes();

            const maxScroll = getMaxScroll();
            if (maxScroll <= 0) {
                return;
            }

            const delta = normalizeWheelDelta(event);
            if (delta === 0) {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            showScrollBarWhileScrolling();
            scheduleHideScrollBar();
            setScrollPosition(scrollPosition.value + delta);
        };

        /**
         * 设置滚动位置到指定值
         * @param nextPosition - 目标滚动位置，会被限制在有效范围内
         */
        const setScrollPosition = (nextPosition: number) => {
            if (!scrollContainerRef.value) {
                return;
            }

            const maxScroll = getMaxScroll();
            const clamped = Math.max(0, Math.min(nextPosition, maxScroll));
            pendingProgrammaticScroll = true;

            if (props.direction === "vertical") {
                scrollContainerRef.value.scrollTop = clamped;
            } else {
                scrollContainerRef.value.scrollLeft = clamped;
            }
        };

        /**
         * 滚动到底部（或最右侧）
         * 将滚动容器滚动到最大可滚动位置
         */
        const scrollToBottom = () => {
            if (!scrollContainerRef.value) {
                return;
            }

            const maxScroll = getMaxScroll();
            pendingProgrammaticScroll = true;

            if (props.direction === "vertical") {
                scrollContainerRef.value.scrollTop = maxScroll;
            } else {
                scrollContainerRef.value.scrollLeft = maxScroll;
            }
        };

        /**
         * 以动画方式滚动到底部（或最右侧）
         * 模拟手动向下滚动的动效
         * @param duration - 动画时长（毫秒），默认为 300
         */
        const scrollToBottomWithAnimation = (duration: number = 300) => {
            if (!scrollContainerRef.value) {
                return;
            }

            const currentPos =
                props.direction === "vertical"
                    ? scrollContainerRef.value.scrollTop
                    : scrollContainerRef.value.scrollLeft;
            const maxScroll = getMaxScroll();

            // 如果已经在底部，则不执行动画
            if (Math.abs(currentPos - maxScroll) < 1) {
                return;
            }

            // 停止之前的动画
            stopMotion(true);

            // 记录动画起始状态
            animatedScrollStartTime = performance.now();
            animatedScrollStartPos = currentPos;
            animatedScrollTargetPos = maxScroll;
            animatedScrollDuration = duration;

            /**
             * 缓动函数：ease-out
             * 模拟手动滚动的减速效果
             */
            const easeOut = (t: number): number => {
                return 1 - Math.pow(1 - t, 3);
            };

            /**
             * 动画帧函数
             */
            const animate = (timestamp: number) => {
                const elapsed = timestamp - animatedScrollStartTime;
                const progress = Math.min(elapsed / animatedScrollDuration, 1);
                const easedProgress = easeOut(progress);

                const newPos =
                    animatedScrollStartPos + (animatedScrollTargetPos - animatedScrollStartPos) * easedProgress;

                pendingProgrammaticScroll = true;
                if (props.direction === "vertical") {
                    scrollContainerRef.value!.scrollTop = newPos;
                } else {
                    scrollContainerRef.value!.scrollLeft = newPos;
                }

                if (progress < 1) {
                    animationFrame = requestAnimationFrame(animate);
                } else {
                    animationFrame = null;
                }
            };

            animationFrame = requestAnimationFrame(animate);
        };

        /**
         * 判断当前是否位于底部（或最右侧）
         * @param threshold - 判断阈值，默认为 10px
         * @returns 是否位于底部
         */
        const isAtBottom = (threshold = 10) => {
            if (!scrollContainerRef.value) {
                return true;
            }

            // 先更新尺寸信息，确保判断准确
            calculateSizes();
            const maxScroll = getMaxScroll();
            return scrollPosition.value >= maxScroll - threshold;
        };

        /**
         * 判断最近一次 scroll 事件是否由程序调用 scrollToBottom / setScrollPosition 等触发。
         * 用于区分程序滚动（不应取消自动滚动定时器）和用户手动滚动（应取消定时器）。
         */
        const isProgrammaticScroll = () => pendingProgrammaticScroll;

        /**
         * 判断容器是否可滚动（内容尺寸大于容器尺寸）
         * @returns 是否可滚动
         */
        const isScrollable = () => {
            // 先更新尺寸信息，确保判断准确
            calculateSizes();
            return scrollSize.value > containerSize.value;
        };

        /**
         * 滚动到顶部（或最左侧）
         * 将滚动容器滚动到位置 0
         */
        const scrollToTop = () => {
            setScrollPosition(0);
        };

        /**
         * 阻止 thumb 自身 click 冒泡到 track，避免触发轨道点击跳转
         */
        const handleThumbClick = (event: MouseEvent) => {
            event.preventDefault();
            event.stopPropagation();
        };

        /**
         * 处理滚动滑块的鼠标按下事件
         * 开始拖拽滑块，记录初始位置和滚动位置
         */
        const handleThumbMouseDown = (event: MouseEvent) => {
            event.preventDefault();
            event.stopPropagation();

            stopMotion(true);
            clearHideScrollBarTimer();

            isDragging.value = true;
            dragStartPosition = props.direction === "vertical" ? event.clientY : event.clientX;
            dragStartScroll = scrollPosition.value;

            document.addEventListener("mousemove", handleThumbMouseMove);
            document.addEventListener("mouseup", handleThumbMouseUp);
        };

        /**
         * 处理拖拽滑块的鼠标移动事件
         * 计算鼠标移动距离并同步滚动位置
         */
        const handleThumbMouseMove = (event: MouseEvent) => {
            if (!isDragging.value || !scrollContainerRef.value) {
                return;
            }

            const currentPosition = props.direction === "vertical" ? event.clientY : event.clientX;
            const delta = currentPosition - dragStartPosition;
            const { trackTravel } = getThumbMetrics();
            const maxScroll = getMaxScroll();
            const scrollDelta = trackTravel <= 0 ? 0 : (delta / trackTravel) * maxScroll;
            const nextScrollPosition = Math.max(0, Math.min(dragStartScroll + scrollDelta, maxScroll));

            setScrollPosition(nextScrollPosition);
        };

        /**
         * 处理拖拽滑块的鼠标释放事件
         * 结束拖拽状态，移除事件监听器
         */
        const handleThumbMouseUp = () => {
            isDragging.value = false;
            suppressNextTrackClick();
            document.removeEventListener("mousemove", handleThumbMouseMove);
            document.removeEventListener("mouseup", handleThumbMouseUp);

            if (!isHovering.value) {
                isScrolling.value = false;
            }
        };

        /**
         * 处理轨道点击事件
         * 点击轨道时，滚动条跳跃到点击位置
         */
        const handleTrackClick = (event: MouseEvent) => {
            // thumb click 冒泡或拖拽释放后的补发 click 都不应触发轨道跳转。
            const isClickFromThumb = event.target instanceof Node && thumbRef.value?.contains(event.target);

            if (suppressTrackClick || isClickFromThumb) {
                suppressTrackClick = false;
                event.preventDefault();
                event.stopPropagation();
                return;
            }

            if (!trackRef.value || !scrollContainerRef.value || isDragging.value) {
                return;
            }

            stopMotion(true);
            clearHideScrollBarTimer();

            const trackRect = trackRef.value.getBoundingClientRect();
            const clickPosition =
                props.direction === "vertical" ? event.clientY - trackRect.top : event.clientX - trackRect.left;
            const { trackTravel } = getThumbMetrics();
            const jumpPosition = trackTravel <= 0 ? 0 : (clickPosition / Math.max(trackTravel, 1)) * getMaxScroll();

            setScrollPosition(jumpPosition);
        };

        /**
         * 处理鼠标进入滚动条命中区事件
         * 仅 hover 到滚动条本身时显示滚动条
         */
        const handleScrollBarMouseEnter = () => {
            clearHideScrollBarTimer();
            isHovering.value = true;
        };

        /**
         * 处理鼠标离开滚动条命中区事件
         * 鼠标移走后交给 opacity 过渡缓慢隐藏
         */
        const handleScrollBarMouseLeave = () => {
            isHovering.value = false;
            if (!isDragging.value) {
                isScrolling.value = false;
            }
        };

        /**
         * 处理窗口或容器尺寸变化
         * 重新计算尺寸并更新滑块位置
         */
        const handleResize = () => {
            scheduleThumbUpdate();
        };

        watch(
            () => [props.direction, props.maxScrollOffset],
            () => {
                // 侧边栏 sticky 高度等偏移量变化时，需要重新计算 thumb 高度。
                scheduleThumbUpdate();
            },
        );

        onMounted(() => {
            nextTick(() => {
                window.addEventListener("resize", handleResize);

                if (scrollContainerRef.value) {
                    scrollContainerRef.value.addEventListener("wheel", handleWheel, { passive: false });
                }

                resizeObserver = new ResizeObserver(() => {
                    handleResize();
                });

                if (scrollContainerRef.value) {
                    resizeObserver.observe(scrollContainerRef.value);
                }

                if (scrollContentInnerRef.value) {
                    resizeObserver.observe(scrollContentInnerRef.value);
                }

                scheduleThumbUpdate();
            });
        });

        onUnmounted(() => {
            isUnmounted = true;
            stopMotion(true);
            clearHideScrollBarTimer();
            cancelScheduledThumbUpdate();
            clearSuppressTrackClickTimer();
            window.removeEventListener("resize", handleResize);
            document.removeEventListener("mousemove", handleThumbMouseMove);
            document.removeEventListener("mouseup", handleThumbMouseUp);
            scrollContainerRef.value?.removeEventListener("wheel", handleWheel);
            resizeObserver?.disconnect();
            resizeObserver = null;
        });

        return {
            scrollContainerRef,
            scrollContentInnerRef,
            scrollContentInnerStyle,
            thumbRef,
            trackRef,
            isDragging,
            isHovering,
            isScrolling,
            scrollSize,
            containerSize,
            handleScroll,
            handleThumbClick,
            handleThumbMouseDown,
            handleTrackClick,
            handleScrollBarWheel,
            handleScrollBarMouseEnter,
            handleScrollBarMouseLeave,
            scrollToBottom,
            scrollToBottomWithAnimation,
            scrollToTop,
            isAtBottom,
            isScrollable,
            isProgrammaticScroll,
            setScrollPosition,
        };
    },
    expose: [
        "scrollToBottom",
        "scrollToBottomWithAnimation",
        "scrollToTop",
        "isAtBottom",
        "isScrollable",
        "isProgrammaticScroll",
        "setScrollPosition",
        "scrollContainerRef",
    ],
    render() {
        const showScrollBar = !this.autoHide || this.isScrolling || this.isHovering || this.isDragging;

        const scrollBarClasses = [
            "scroll-bar",
            `scroll-bar--${this.direction}`,
            {
                "scroll-bar--visible": showScrollBar,
                "scroll-bar--dragging": this.isDragging,
            },
        ];

        const thumbStyle = this.thumbColor ? { backgroundColor: this.thumbColor } : {};
        const trackStyle = this.trackColor ? { backgroundColor: this.trackColor } : {};

        return (
            <div class="scroll-bar-container">
                <div ref="scrollContainerRef" class="scroll-bar-content" onScroll={this.handleScroll}>
                    <div
                        ref="scrollContentInnerRef"
                        class="scroll-bar-content-inner"
                        style={this.scrollContentInnerStyle}
                    >
                        {this.$slots.default?.()}
                    </div>
                </div>

                {this.scrollSize - this.maxScrollOffset > this.containerSize && (
                    <div
                        class={scrollBarClasses}
                        onMouseenter={this.handleScrollBarMouseEnter}
                        onMouseleave={this.handleScrollBarMouseLeave}
                        onWheel={this.handleScrollBarWheel}
                    >
                        <div ref="trackRef" class="scroll-bar-track" style={trackStyle} onClick={this.handleTrackClick}>
                            <div
                                ref="thumbRef"
                                class="scroll-bar-thumb"
                                style={thumbStyle}
                                onClick={this.handleThumbClick}
                                onMousedown={this.handleThumbMouseDown}
                            />
                        </div>
                    </div>
                )}
            </div>
        );
    },
});
