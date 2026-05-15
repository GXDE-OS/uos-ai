import { defineComponent, PropType, ref, computed, watch } from "vue";
import type { SliderCardData } from "@/types/conversation";
import SvgIcon from "@/components/SvgIcon";
import { useBackendStore } from "@/stores";
import { debounce } from "lodash-es";
import CardBase from "./CardBase";
import "./CardBase.css";
import "./CommandCard.css";

/**
 * 滑块卡片组件
 * 用于显示和控制滑块值（如屏幕亮度、音量等）
 */
export default defineComponent({
    name: "SliderCard",

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
            if (props.data.percent !== undefined) {
                return Number(props.data.percent);
            }
            return 0;
        };

        // 本地滑块值
        const localValue = ref(getInitialValue());
        // 加载状态
        const isLoading = ref(false);
        // 原始值，用于错误恢复
        const originalValue = ref(localValue.value);

        console.log("[SliderCard] Initialized with value:", localValue.value, "props.data:", props.data);

        // 计算滑块的最小值、最大值和步长
        const min = computed(() => props.data.min);
        const max = computed(() => props.data.max);
        const step = computed(() => props.data.step ?? 1);

        // 获取左右图标名称
        const getSliderIcons = (title: string) => {
            if (title === "Volume") {
                return {
                    left: "card-volume-left",
                    right: "card-volume-right"
                };
            } else if (title === "Brightness") {
                return {
                    left: "card-brightness-left",
                    right: "card-brightness-right"
                };
            }
            // 默认返回音量图标
            return {
                left: "card-volume-left",
                right: "card-volume-right"
            };
        };

        // 计算显示的文本值
        const displayValue = computed(() => {
            if (props.data.percent !== undefined) {
                return `${localValue.value}%`;
            }
            return `${localValue.value}`;
        });

        // 计算进度百分比
        const progressPercentage = computed(() => {
            const range = max.value - min.value;
            if (range === 0) return 0;
            return ((localValue.value - min.value) / range) * 100;
        });

        // 防抖处理：延迟100ms执行更新
        const debouncedUpdate = debounce(async (value: number) => {
            try {
                isLoading.value = true;

                // 创建新的数据对象，更新滑块值
                const newData: SliderCardData = {
                    ...props.data,
                };

                if (props.data.percent !== undefined) {
                    newData.percent = value;
                    // 根据标题判断是音量还是亮度
                    if (props.data.title === "Volume") {
                        await backendStore.requestSystem("updateVolume", value);
                    } else if (props.data.title === "Brightness") {
                        await backendStore.requestSystem("updateBrightness", value);
                    } else {
                        await backendStore.requestSystem("updateSlider", props.data.title, value);
                    }
                }

                emit("sliderChange", newData);
                originalValue.value = value;
            } catch (error) {
                console.error("Failed to update slider:", error);
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
                if (props.data.title === "Volume") {
                    backendStore.requestSystem("openControlCenter", "sound");
                } else if (props.data.title === "Brightness") {
                    backendStore.requestSystem("openControlCenter", "display");
                } else {
                    backendStore.requestSystem("openControlCenter", props.data.title);
                }
            } catch (error) {
                console.error("Failed to open control center:", error);
            }
        };

        const handleSliderChange = (value: number) => {
            console.log("[SliderCard] Slider changed to:", value, "type:", typeof value);
            localValue.value = value;
            debouncedUpdate(localValue.value);
        };

        const handleSliderMouseDown = (e: MouseEvent) => {
            e.preventDefault();
            const sliderContainer = (e.currentTarget as HTMLElement).parentElement;
            if (!sliderContainer) return;

            const handleMouseMove = (moveEvent: MouseEvent) => {
                const rect = sliderContainer.getBoundingClientRect();
                const x = moveEvent.clientX - rect.left;
                const width = rect.width;
                const percentage = Math.max(0, Math.min(1, x / width));
                const value = min.value + (max.value - min.value) * percentage;
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
            console.log("[SliderCard] props.data changed, updating localValue from", localValue.value, "to", newValue, "newData:", newData);
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
            isLoading,
            backendStore,
            getSliderIcons,
        };
    },

    render() {
        const { data } = this.$props;
        const sliderIcons = this.getSliderIcons(data.title);

        return (
            <CardBase
                title={data.title}
                cardClassSuffix="slider"
                settingsIcon="controlcenter"
                showActionIcon={true}
                dividerWidth="260px"
                onActionClick={this.handleCardClick}
            >
                <div class="slider-card__left-section">
                    <span class="slider-card__bottom-title">{this.backendStore.translate(data.title)}</span>
                </div>

                <div class="slider-card__slider-wrapper">
                    <SvgIcon icon={sliderIcons.left} size={[16, 16]} class="slider-card__icon" />
                    <div class="slider-card__slider-container">
                        <div class="slider-card__slider-track">
                            <div
                                class="slider-card__slider-progress"
                                style={{ width: `${this.progressPercentage}%` }}
                            ></div>
                        </div>
                        <div
                            class="slider-card__slider-thumb"
                            style={{ left: `${this.progressPercentage}%` }}
                            onMousedown={(e: MouseEvent) => this.handleSliderMouseDown(e)}
                        >
                            <SvgIcon icon="card-square-slider" size={[32, 32]} />
                        </div>
                    </div>
                    <SvgIcon icon={sliderIcons.right} size={[16, 16]} class="slider-card__icon" />
                </div>

            </CardBase>
        );
    },
});
