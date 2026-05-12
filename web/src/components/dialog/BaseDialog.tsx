import { defineComponent, ref, computed, type PropType, onMounted, onBeforeUnmount, watch, nextTick } from "vue";
import type { DialogButton } from "@/types/dialog";
import SvgIcon from "@/components/SvgIcon";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";
import { useBackendStore } from "@/stores/backend";
import { useThemeIcon } from "@/utils/loadThemeIcon";
import "@/assets/styles/components/BaseDialog.css";

/**
 * 基础对话框组件
 *
 * 结构：顶部（图标 + 关闭按钮）+ 默认插槽（body）+ 底部按钮列表。
 * 调用方通过 onCancel / onButtonClick 两个事件感知用户操作。
 * 每个按钮可挂 beforeClick 钩子做提交前校验，返回 false 则阻止事件下传。
 */
export default defineComponent({
    name: "BaseDialog",

    props: {
        /** 对话框是否可见 */
        visible: {
            type: Boolean,
            required: true,
        },
        /**
         * 顶部左侧图标名称（SVG name 或系统应用图标名）。
         * 传空字符串则不显示图标区域。
         * 默认使用 uos-ai 应用图标。
         */
        icon: {
            type: String,
            default: "UosAiAssistant",
        },
        /** 对话框标题，为空时隐藏标题行 */
        title: {
            type: String,
            default: "",
        },
        /** 底部按钮列表，空数组时隐藏底部区域 */
        buttons: {
            type: Array as PropType<DialogButton[]>,
            default: () => [],
        },
        /**
         * 追加到 .dialog 容器的额外 CSS class，用于调用方做样式定制。
         * 例如 AlertDialog 传入 "alert-dialog-content" 以保留其旧版视觉。
         */
        dialogClass: {
            type: String,
            default: "",
        },
        /**
         * 后端图标加载尺寸 [w, h]，默认 [20, 20]。
         * AlertDialog 等需要大图标的场景可传 [32, 32]。
         */
        iconSize: {
            type: Array as unknown as PropType<[number, number]>,
            default: () => [20, 20],
        },
        /**
         * 标题位置。
         * row: 标题显示在单独一行并居中。
         * header: 标题显示在 header 左侧，与图标同行。
         */
        titlePlacement: {
            type: String as PropType<"row" | "header">,
            default: "row",
        },
        /**
         * 点击遮罩层时是否关闭对话框。
         * 默认关闭，保持现有对话框行为不变。
         */
        closeOnOverlayClick: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        /** 点击右上角关闭按钮时触发 */
        cancel: () => true,
        /** 底部按钮通过 beforeClick 校验后触发，携带按钮 key */
        buttonClick: (key: string) => typeof key === "string",
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const dialogRef = ref<HTMLElement | null>(null);
        const previousActiveElement = ref<HTMLElement | null>(null);

        // 图标名称响应式引用
        const iconName = computed(() => props.icon);
        const [iconW, iconH] = props.iconSize;
        // 加载主题图标，失败时 fallback 到 iconName 用于 SvgIcon 渲染
        const appIconUrl = useThemeIcon(iconName, {
            width: iconW,
            height: iconH,
            fallbackToName: true,
        });

        const focusDialog = () => {
            if (!props.visible) return;
            if (!previousActiveElement.value && document.activeElement instanceof HTMLElement) {
                previousActiveElement.value = document.activeElement;
            }
            nextTick(() => {
                dialogRef.value?.focus();
            });
        };

        onMounted(() => {
            focusDialog();
        });

        onBeforeUnmount(() => {
            previousActiveElement.value?.focus?.();
        });

        watch(
            () => props.visible,
            (visible) => {
                if (visible) {
                    focusDialog();
                }
            },
        );

        const handleButtonClick = async (button: DialogButton) => {
            if (button.disabled) {
                return;
            }

            if (button.beforeClick) {
                const allowed = await button.beforeClick();
                if (allowed === false) return;
            }
            emit("buttonClick", button.key);
        };

        const handleCancel = () => {
            emit("cancel");
        };

        const handleOverlayClick = () => {
            if (!props.closeOnOverlayClick) {
                return;
            }

            emit("cancel");
        };

        const handleDialogKeyDown = (event: KeyboardEvent) => {
            if (event.key === "Escape") {
                event.preventDefault();
                emit("cancel");
            }
        };

        return {
            appIconUrl,
            dialogRef,
            handleButtonClick,
            handleCancel,
            handleOverlayClick,
            handleDialogKeyDown,
            isEnableAdvancedCssFeatures: backend.$state.isEnableAdvancedCssFeatures,
        };
    },

    render() {
        if (!this.visible) return null;

        return (
            <div class="dialog-overlay" onClick={this.handleOverlayClick}>
                <div
                    ref="dialogRef"
                    class={["dialog", this.isEnableAdvancedCssFeatures && "dialog--blur", this.dialogClass || ""]}
                    tabindex={-1}
                    onClick={(e: MouseEvent) => e.stopPropagation()}
                    onKeydown={this.handleDialogKeyDown}
                >
                    {/* 标题栏：左侧图标（可选）+ 右侧关闭按钮 */}
                    <div class="dialog__header">
                        <div class="dialog__header-main">
                            {this.appIconUrl &&
                                (this.appIconUrl.startsWith("data:") ? (
                                    <img
                                        class="dialog__app-icon"
                                        src={this.appIconUrl}
                                        onMousedown={(e) => e.stopPropagation()}
                                    />
                                ) : (
                                    <SvgIcon class="dialog__app-icon" icon={this.appIconUrl} size={[20, 20]} />
                                ))}
                            {this.title && this.titlePlacement === "header" && (
                                <h3 class={["dialog__title", "dialog__title--header"]}>{this.title}</h3>
                            )}
                        </div>
                        <IconButton
                            icon="icon_titlebar_close"
                            iconSize={[16, 16]}
                            size={[40, 40]}
                            shape={ButtonShape.Rounded}
                            onClick={this.handleCancel}
                        />
                    </div>

                    {/* 居中标题（title 为空时跳过渲染） */}
                    {this.title && this.titlePlacement === "row" && (
                        <div class="dialog__title-row">
                            <h3 class="dialog__title">{this.title}</h3>
                        </div>
                    )}

                    {/* 默认插槽：body 区域由调用方填充 */}
                    <div class="dialog__body">{this.$slots.default?.()}</div>

                    {/* 底部按钮区（无按钮时跳过渲染） */}
                    {this.buttons.length > 0 && (
                        <div class="dialog__footer">
                            {this.buttons.map((button) => (
                                <button
                                    key={button.key}
                                    class={[
                                        "dialog__btn",
                                        `dialog__btn--${button.type ?? "default"}`,
                                        button.disabled && "dialog__btn--disabled",
                                        button.suggested && "dialog__btn--suggested",
                                    ]}
                                    disabled={button.disabled}
                                    onClick={() => this.handleButtonClick(button)}
                                >
                                    {button.text}
                                </button>
                            ))}
                        </div>
                    )}
                </div>
            </div>
        );
    },
});
