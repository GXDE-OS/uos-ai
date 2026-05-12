import { defineComponent, PropType, ref, computed, watch } from "vue";
import type { SliderCardData } from "@/types/conversation";
import SvgIcon from "@/components/SvgIcon";
import CardBase from "./CardBase";
import { useBackendStore } from "@/stores";
import { debounce } from "lodash-es";

/**
 * 字体大小滑块卡片组件
 * 专门用于显示和控制字体大小
 */
export default defineComponent({
    name: "FontSizeCard",

    components: {
        SvgIcon,
        CardBase,
    },

    props: {
        data: {
            type: Object as PropType<SliderCardData>,
            required: true,
        },
    },

    emits: {
        cardClick: (data: SliderCardData) => !!data,
        sliderChange: (data: SliderCardData) => !!data,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();

        // 获取初始值的函数
        const getInitialValue = () => {
            if (props.data.size !== undefined) {
                return Number(props.data.size);
            }
            return 0;
        };

        // 本地滑块值
        const localValue = ref(getInitialValue());
        // 加载状态
        const isLoading = ref(false);
        // 原始值，用于错误恢复
        const originalValue = ref(localValue.value);

        console.log("[FontSizeCard] Initialized with value:", localValue.value, "props.data:", props.data);

        // 计算滑块的最小值、最大值和步长
        props.data.min = 11;
        const min = computed(() => props.data.min);
        const max = computed(() => props.data.max);
        const step = computed(() => props.data.step ?? 1);

        // 字体刻度值
        const fontTickValues = [11, 12, 13, 14, 15, 16, 18, 20];

        // 刻度线和标签的左右边距（5px）
        const tickPadding = 5;

        // 计算显示的文本值
        const displayValue = computed(() => {
            if (props.data.size !== undefined) {
                return `${localValue.value}`;
            }
            return `${localValue.value}`;
        });

        // 计算进度百分比
        const progressPercentage = computed(() => {
            // 找到当前值在刻度值数组中的索引
            // localValue.value = 11;
            
            const currentIndex = fontTickValues.findIndex(v => v === localValue.value);
            if (currentIndex === -1) return 0;

            // 打印当前值和索引
            console.log("[FontSizeCard] Calculating progress percentage, localValue:", localValue.value, "currentIndex:", currentIndex, "fontTickValues:", fontTickValues);
            // 基于刻度值数组计算百分比
            return (currentIndex / (fontTickValues.length - 1)) * 100;
        });

        // 防抖处理：延迟100ms执行更新
        const debouncedUpdate = debounce(async (value: number) => {
            try {
                isLoading.value = true;

                // 创建新的数据对象，更新滑块值
                const newData: SliderCardData = {
                    ...props.data,
                };

                if (props.data.size !== undefined) {
                    newData.size = value;
                    // 系统字号使用updateFontSize接口
                    await backendStore.requestSystem("updateFontSize", value);
                }

                emit("sliderChange", newData);
                originalValue.value = value;
            } catch (error) {
                console.error("Failed to update font size:", error);
                // 恢复原值
                localValue.value = originalValue.value;
            } finally {
                isLoading.value = false;
            }
        }, 300);

        const handleCardClick = () => {
            emit("cardClick", props.data);

            // 跳转到控制中心
            try {
                backendStore.requestSystem("openControlCenter", "font");
            } catch (error) {
                console.error("Failed to open control center:", error);
            }
        };

        const handleSliderChange = (value: number) => {
            console.log("[FontSizeCard] Slider changed to:", value, "type:", typeof value);
            // 确保值是刻度值之一
            const closestValue = fontTickValues.reduce((prev, curr) => {
                return (Math.abs(curr - value) < Math.abs(prev - value) ? curr : prev);
            });
            localValue.value = closestValue;
            debouncedUpdate(localValue.value);
        };

        // 根据点击位置计算对应的刻度值
        const getValueFromPosition = (clientX: number, container: HTMLElement): number => {
            const rect = container.getBoundingClientRect();
            const x = clientX - rect.left;
            const width = rect.width;

            // 有效区域是从 tickPadding 到 width - tickPadding
            const effectiveWidth = width - tickPadding * 2;
            const effectiveX = x - tickPadding;

            // 计算在有效区域内的百分比
            const percentage = Math.max(0, Math.min(1, effectiveX / effectiveWidth));

            // 基于刻度索引计算，与 progressPercentage 计算逻辑保持一致
            const index = Math.round(percentage * (fontTickValues.length - 1));
            const clampedIndex = Math.max(0, Math.min(fontTickValues.length - 1, index));
            return fontTickValues[clampedIndex] ?? fontTickValues[0]!;
        };

        // 点击滑轨容器时的处理
        const handleSliderContainerClick = (e: MouseEvent) => {
            // 如果点击的是滑块本身，不处理（由 mousedown 处理）
            const target = e.target as HTMLElement;
            if (target.closest('.slider-card__slider--font-thumb')) return;

            const container = e.currentTarget as HTMLElement;
            const value = getValueFromPosition(e.clientX, container);
            handleSliderChange(value);
        };

        const handleSliderMouseDown = (e: MouseEvent) => {
            e.preventDefault();
            const sliderContainer = (e.currentTarget as HTMLElement).parentElement;
            if (!sliderContainer) return;

            const handleMouseMove = (moveEvent: MouseEvent) => {
                const value = getValueFromPosition(moveEvent.clientX, sliderContainer);
                handleSliderChange(value);
            };

            const handleMouseUp = () => {
                document.removeEventListener('mousemove', handleMouseMove);
                document.removeEventListener('mouseup', handleMouseUp);
            };

            document.addEventListener('mousemove', handleMouseMove);
            document.addEventListener('mouseup', handleMouseUp);
        };

        // 监听props变化，更新本地值
        watch(() => props.data, (newData) => {
            const newValue = getInitialValue();
            console.log("[FontSizeCard] props.data changed, updating localValue from", localValue.value, "to", newValue, "newData:", newData);
            localValue.value = newValue;
            originalValue.value = newValue;
        }, { deep: true });

        return {
            localValue,
            min,
            max,
            step,
            displayValue,
            progressPercentage,
            handleCardClick,
            handleSliderChange,
            handleSliderMouseDown,
            handleSliderContainerClick,
            isLoading,
            backendStore,
            fontTickValues,
            tickPadding,
        };
    },

    render() {
        const { data } = this.$props;

        return (
            <CardBase
                title={data.title}
                cardClassSuffix="slider"
                settingsIcon="controlcenter"
                showActionIcon={true}
                dividerWidth="300px"
                onActionClick={this.handleCardClick}
                class="slider-card--font-size"
            >
                {/* 第一行：左边文字和导轨线垂直居中对齐 */}
                <div class="slider-card__font-row">
                    <div class="slider-card__left-section">
                        <span class="slider-card__bottom-title">{this.backendStore.translate(data.title)}</span>
                    </div>

                    <div
                        class="slider-card__slider--font-container"
                        onClick={(e: MouseEvent) => this.handleSliderContainerClick(e)}
                    >
                        <div class="slider-card__slider--font-track">
                            <div
                                style={{
                                    left: `${this.tickPadding}px`,
                                    width: `calc((100% - ${this.tickPadding * 2}px) * ${this.progressPercentage} / 100)`
                                }}
                            ></div>
                        </div>
                        <div
                            class="slider-card__slider--font-thumb"
                            style={{ left: `calc(${this.tickPadding}px + (100% - ${this.tickPadding * 2}px) * ${this.progressPercentage} / 100)` }}
                            onMousedown={(e: MouseEvent) => this.handleSliderMouseDown(e)}
                        >
                            <SvgIcon icon="card-triangle-slider" size={[32, 32]} />
                        </div>
                    </div>
                </div>

                {/* 第二行：刻度线和数字 */}
                <div class="slider-card__font-ticks-row">
                    <div class="slider-card__ticks">
                        {this.fontTickValues.map((value, index) => {
                            // 在有效区域内（左右各留 5px）计算位置
                            const percent = (index / (this.fontTickValues.length - 1)) * 100;
                            const left = `calc(${this.tickPadding}px + (100% - ${this.tickPadding * 2}px) * ${percent} / 100)`;
                            return (
                                <div
                                    key={value}
                                    class="slider-card__tick"
                                    style={{ left }}
                                />
                            );
                        })}
                    </div>
                    <div class="slider-card__labels">
                        {this.fontTickValues.map((value, index) => {
                            // 在有效区域内（左右各留 5px）计算位置
                            const percent = (index / (this.fontTickValues.length - 1)) * 100;
                            const left = `calc(${this.tickPadding}px + (100% - ${this.tickPadding * 2}px) * ${percent} / 100)`;
                            return (
                                <div
                                    key={value}
                                    class={`slider-card__label ${this.localValue === value ? 'active' : ''}`}
                                    style={{ left }}
                                    onClick={() => this.handleSliderChange(value)}
                                >
                                    {value}
                                </div>
                            );
                        })}
                    </div>
                </div>

            </CardBase>
        );
    },
});
