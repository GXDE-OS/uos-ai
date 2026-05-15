import { defineComponent, type PropType } from "vue";
import { ElTooltip } from "element-plus";
import { useBackendStore } from "@/stores";

/**
 * Tooltip 组件
 * 封装 ElTooltip，提供默认配置（样式、位置、延时等）
 * 自动处理 isAdvanced 状态
 */
export default defineComponent({
    name: "Tooltip",

    props: {
        /** Tooltip 内容 */
        content: {
            type: String,
            required: true,
        },
        /** 显示位置 */
        placement: {
            type: String as PropType<
                | "top" | "top-start" | "top-end"
                | "bottom" | "bottom-start" | "bottom-end"
                | "left" | "left-start" | "left-end"
                | "right" | "right-start" | "right-end"
            >,
            default: "bottom",
        },
        /** 是否禁用 tooltip */
        disabled: {
            type: Boolean,
            default: false,
        },
        /** 是否显示箭头 */
        showArrow: {
            type: Boolean,
            default: false,
        },
        /** 延迟显示（毫秒） */
        showAfter: {
            type: Number,
            default: 500,
        },
        /** 延迟隐藏（毫秒） */
        hideAfter: {
            type: Number,
            default: 0,
        },
        /** 自定义 popper class（会与默认的 uos-tooltip 合并） */
        popperClass: {
            type: [String, Array] as PropType<string | string[]>,
            default: "",
        },
        /** 是否原始内容 */
        isRawContent: {
            type: Boolean,
            default: false,
        },
        /** 触发方式 */
        trigger: {
            type: String as PropType<"hover" | "click" | "focus" | "contextmenu">,
            default: "hover",
        },
        /** 受控显示状态（null 表示不受控） */
        visible: {
            type: [Boolean, null] as PropType<boolean | null>,
            default: null,
        },
    },

    setup(props, { slots }) {
        const backend = useBackendStore();

        // 合并默认样式类和自定义样式类
        const computedPopperClass = () => {
            const classes = ["uos-tooltip"];
            // 自动添加 advanced 样式类
            if (backend.isEnableAdvancedCssFeatures) {
                classes.push("uos-tooltip--advanced");
            }
            // 添加用户自定义样式类
            if (typeof props.popperClass === "string") {
                if (props.popperClass) classes.push(props.popperClass);
            } else if (Array.isArray(props.popperClass)) {
                classes.push(...props.popperClass);
            }
            return classes.join(" ");
        };

        return () => (
            <ElTooltip
                content={props.content}
                placement={props.placement}
                disabled={props.disabled}
                trigger={props.trigger}
                visible={props.visible === null ? undefined : props.visible}
                visible-arrow={props.showArrow}
                show-after={props.showAfter}
                hide-after={props.hideAfter}
                popper-class={computedPopperClass()}
                raw-content={props.isRawContent}
                v-slots={{
                    // 保持现有纯文本/HTML content 用法不变，同时允许复杂 tooltip 走具名 slot。
                    default: () => slots.default?.(),
                    ...(slots.content ? { content: () => slots.content?.() } : {}),
                }}
            />
        );
    },
});
