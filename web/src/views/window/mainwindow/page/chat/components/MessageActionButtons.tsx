import { defineComponent, type PropType, type CSSProperties } from "vue";
import IconButton from "@/components/IconButton";
import CopyButton from "@/components/CopyButton";
import SvgIcon from "@/components/SvgIcon";
import { ButtonShape } from "@/types/button";
import "@/assets/styles/window/mainwindow/page/chat/components/MessageActionButtons.css";

interface ActionButton {
    icon: string;
    onClick: (event: MouseEvent) => void;
    tooltip?: string;
    isPlaying?: boolean; // 是否正在播放（仅用于 voice-play 按钮）
    disabled?: boolean; // 是否禁用
}

interface SiblingMessage {
    currIndex: number;
    total: number;
    onPrevious?: () => void;
    onNext?: () => void;
}

interface MessageActionButtonsConfig {
    // 自定义类名
    className?: string;
    // 自定义样式
    style?: CSSProperties;
    // 模型图标
    modelIcon?: string;
    // 模型名称
    modelName?: string;
    // 操作按钮数组
    actionButtons?: ActionButton[];
    // 兄弟消息
    siblingMessage?: SiblingMessage;
}

export default defineComponent({
    name: "MessageActionButtons",

    props: {
        // 配置对象
        config: {
            type: Object as PropType<MessageActionButtonsConfig>,
            required: true,
        },
    },

    emits: {
        switchToPrevious: () => true,
        switchToNext: () => true,
    },

    setup(props, { emit }) {
        return { emit };
    },

    render() {
        const {
            className,
            style,
            modelIcon,
            modelName,
            actionButtons = [],
            siblingMessage = {} as SiblingMessage,
        } = this.$props.config;
        const hasModel = modelIcon || modelName;
        const hasActions = actionButtons && actionButtons.length > 0;
        const hasSwitchButton = siblingMessage && siblingMessage.total > 1;

        // 如果两个区域都没有数据，则不渲染
        if (!hasModel && !hasActions && !hasSwitchButton) {
            return null;
        }

        return (
            <div class={["message-action-buttons", className].filter(Boolean).join(" ")} style={style}>
                {/* 模型区 */}
                {hasModel && (
                    <div class="message-action-buttons__model">
                        {modelIcon && <SvgIcon icon={modelIcon} class="message-action-buttons__model-icon" />}
                        {modelName && <span class="message-action-buttons__model-name">{modelName}</span>}
                    </div>
                )}

                {/* 分割线 - 只在模型区和操作区都存在时显示 */}
                {hasModel && hasActions && <div class="message-action-buttons__divider"></div>}

                {/* 操作区 */}
                {hasActions && (
                    <div class="message-action-buttons__actions">
                        {actionButtons.map((button, index) => {
                            // 如果是复制按钮，使用 CopyButton 组件
                            if (button.icon === "copy") {
                                return (
                                    <CopyButton
                                        key={index}
                                        size={[24, 24]}
                                        iconSize={[16, 16]}
                                        shape={ButtonShape.Rounded}
                                        tooltip={button.tooltip}
                                        onClick={button.onClick}
                                    />
                                );
                            }
                            // 如果是语音播放按钮且正在播放，显示播放动画
                            if (button.icon === "voice-play" && button.isPlaying) {
                                return (
                                    <div
                                        key={index}
                                        class="voice-play-animation"
                                        onClick={button.onClick}
                                        title={button.tooltip}
                                    >
                                        <svg viewBox="0 0 16 16" xmlns="http://www.w3.org/2000/svg">
                                            <rect class="wave-bar bar-1" x="2" y="4" width="2" height="8" rx="1" />
                                            <rect class="wave-bar bar-2" x="5.5" y="2" width="2" height="12" rx="1" />
                                            <rect class="wave-bar bar-3" x="9" y="3" width="2" height="10" rx="1" />
                                            <rect class="wave-bar bar-4" x="12.5" y="5" width="2" height="6" rx="1" />
                                        </svg>
                                    </div>
                                );
                            }
                            // 其他按钮使用 IconButton 组件
                            return (
                                <IconButton
                                    key={index}
                                    size={[24, 24]}
                                    iconSize={[16, 16]}
                                    icon={button.icon}
                                    shape={ButtonShape.Rounded}
                                    tooltip={button.tooltip}
                                    onClick={button.onClick}
                                    disabled={button.disabled}
                                />
                            );
                        })}
                    </div>
                )}

                {/* 分割线 - 只在左右切换按钮都存在时显示 */}
                {hasSwitchButton && <div class="message-action-buttons__divider"></div>}

                {/* 左右切换按钮 */}
                {hasSwitchButton && (
                    <div class="message-action-buttons__switch">
                        <IconButton
                            size={[24, 24]}
                            iconSize={[16, 16]}
                            icon={"icon_arrow"}
                            shape={ButtonShape.Rounded}
                            disabled={siblingMessage.currIndex <= 0}
                            onClick={() => siblingMessage.onPrevious?.()}
                            class="message-action-buttons__switch-button--previous"
                        />
                        <div class="message-action-buttons__switch-counter">
                            {siblingMessage.currIndex + 1}/{siblingMessage.total}
                        </div>
                        <IconButton
                            size={[24, 24]}
                            iconSize={[16, 16]}
                            icon={"icon_arrow"}
                            shape={ButtonShape.Rounded}
                            disabled={siblingMessage.currIndex >= siblingMessage.total - 1}
                            onClick={() => siblingMessage.onNext?.()}
                            class="message-action-buttons__switch-button--next"
                        />
                    </div>
                )}
            </div>
        );
    },
});
