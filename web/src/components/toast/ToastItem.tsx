import { defineComponent, computed, onUnmounted, type PropType } from "vue";
import type { ToastInstance, ToastType } from "@/types/notify";
import SvgIcon from "@/components/SvgIcon";
import { useThemeIcon } from "@/utils/loadThemeIcon";
import "@/assets/styles/components/Toast.css";

/** Toast 消息类型到图标的映射 */
const TOAST_TYPE_ICON_MAP: Record<ToastType, string> = {
    info: "toast-info",
    warning: "toast-warning",
    error: "toast-warning",
    success: "toast-done",
    processing: "loading",
};

/**
 * 单个 Toast 条目组件
 *
 * 显示图标、消息、操作按钮和关闭按钮。
 * 通过 closing prop 控制进入/退出动画。
 * 支持主题图标（通过后端加载系统图标）。
 */
export default defineComponent({
    name: "ToastItem",

    props: {
        /** Toast 实例数据 */
        toast: {
            type: Object as PropType<ToastInstance>,
            required: true,
        },
        /** 是否启用高级 CSS 特效（如 backdrop-filter） */
        isEnableAdvancedCssFeatures: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        /** 点击关闭按钮时触发 */
        close: () => true,
        /** 点击操作按钮时触发，携带 action key */
        action: (key: string) => typeof key === "string",
        /** 退出动画结束时触发 */
        animationEnd: () => true,
    },

    setup(props, { emit }) {
        // 解析图标名称：手动指定 > type 映射
        const iconName = computed(() => {
            const { type, icon: customIcon } = props.toast.options;
            return customIcon || TOAST_TYPE_ICON_MAP[type ?? "info"];
        });

        // 加载主题图标，失败时 fallback 到 iconName 用于 SvgIcon 渲染
        const appIconUrl = useThemeIcon(iconName, {
            width: 16,
            height: 16,
            fallbackToName: true,
        });

        // 处理动画结束事件
        const handleAnimationEnd = (e: AnimationEvent) => {
            // 只有关闭动画结束时才触发移除
            if (props.toast.closing && e.animationName === "toast-exit") {
                emit("animationEnd");
            }
        };

        // 组件卸载时确保触发移除（防止动画被打断导致 toast 残留）
        onUnmounted(() => {
            emit("animationEnd");
        });

        return {
            appIconUrl,
            handleAnimationEnd,
            handleClose: () => emit("close"),
            handleAction: (key: string) => emit("action", key),
        };
    },

    render() {
        const { toast, appIconUrl, isEnableAdvancedCssFeatures } = this;
        const { options, closing } = toast;
        const isProcessing = options.type === "processing";

        return (
            <div
                class={[
                    "toast-item",
                    { "toast-item--closing": closing },
                    isEnableAdvancedCssFeatures && "toast-item--advanced-css",
                ]}
                onAnimationend={this.handleAnimationEnd}
            >
                {/* 类型图标 */}
                {appIconUrl &&
                    (appIconUrl.startsWith("data:") ? (
                        <img class={["toast-item__icon", isProcessing && "toast-item__icon--loading"]} src={appIconUrl} />
                    ) : (
                        <SvgIcon
                            icon={appIconUrl}
                            class={["toast-item__icon", isProcessing && "toast-item__icon--loading"]}
                        />
                    ))}

                {/* 消息文案 */}
                <span class="toast-item__message">{options.message}</span>

                {/* 操作按钮列表 */}
                {options.actions?.map((action) => (
                    <div
                        key={action.key}
                        class={["toast-item__action", `toast-item__action--${action.type ?? "default"}`]}
                        onClick={() => this.handleAction(action.key)}
                    >
                        {action.text}
                    </div>
                ))}

                {/* 关闭按钮（showClose 为 true 时显示） */}
                {options.showClose && (
                    <div class="toast-item__close" onClick={this.handleClose}>
                        <SvgIcon icon="icon_titlebar_close" size={[16, 16]} />
                    </div>
                )}
            </div>
        );
    },
});
