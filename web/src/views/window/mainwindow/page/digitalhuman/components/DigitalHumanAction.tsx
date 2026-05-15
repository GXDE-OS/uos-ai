import { defineComponent, type PropType, computed, watch, ref } from "vue";
import IconButton from "@/components/IconButton";
import SvgIcon from "@/components/SvgIcon";
import { DigitalHumanState } from "@/types/DigitalHuman";
import { ErrorType, HttpErrorType } from "@/types/errortype";
import { useBackendStore, useDigitalHumanStore, useNotifyStore, useAssistantChannelStore } from "@/stores";
import { SettingNav, type ErrorMsg } from "@/types/conversation";
import ErrorActionButton from "@/views/window/mainwindow/components/ErrorActionButton";
import Tooltip from "@/components/Tooltip";

export default defineComponent({
    name: "DigitalHumanAction",

    props: {
        // 当前状态
        state: {
            type: String as PropType<DigitalHumanState>,
            default: DigitalHumanState.Silence,
        },
        // 错误信息结构
        errorMessage: {
            type: Object as PropType<ErrorMsg | null>,
            default: null,
        },
        // 是否显示倒计时
        showCountdown: {
            type: Boolean,
            default: false,
        },
        // 倒计时值
        countdownValue: {
            type: Number,
            default: 0,
        },
        // 模型名称
        model: {
            type: String,
            default: "",
        },
        // 快捷键列表
        shortcutList: {
            type: Array as PropType<string[]>,
            default: () => [],
        },
    },

    emits: {
        // 发送按钮点击事件
        start: null,
        // 停止按钮点击事件
        stop: null,
        // 操作被取消信号
        operationCanceled: () => {
            return true;
        },
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const digitalHumanStore = useDigitalHumanStore();
        const notifyStore = useNotifyStore();
        const assistantChannelStore = useAssistantChannelStore();

        // 获取错误数据（如果有）
        const firstErrorData = computed(() => {
            return props.errorMessage;
        });

        // 处理错误消息显示
        const errorMessage = computed(() => {
            const errorData = firstErrorData.value;
            if (!errorData) return "";

            const { error, http_error, error_message } = errorData;
            if (error === ErrorType.HttpError && http_error && http_error === HttpErrorType.OperationCanceledError) {
                return "";
            }
            return error_message || "";
        });

        // 处理错误码
        // 根据错误码填充操作按钮文本及回调函数
        const actions = computed(() => {
            const result: Array<{ text: string; onClick: (event: MouseEvent) => void }> = [];
            const errorData = firstErrorData.value;

            if (!errorData) return result;

            switch (errorData.error) {
                case ErrorType.HttpError: // HTTP错误码
                    // TODO:网络错误是否有需要显示去配置等按钮的逻辑
                    break;
                case ErrorType.InvalidModel: // model无效
                    result.push({
                        text: backendStore.translate("Go to configuration"),
                        onClick: goConfig("model_invalid"),
                    });
                    break;
                case ErrorType.ModelChatUsageLimitReached: // 模型调用次数超过限制，且无领取次数
                    result.push({
                        text: backendStore.translate("Go to configuration"),
                        onClick: goConfig("model_chat_usage_limit_reached"),
                    });
                    break;
                case ErrorType.ModelChatUsageClaimAgain: // 模型调用次数超过限制，且有领取次数
                    result.push({
                        text: backendStore.translate("Go to configuration"),
                        onClick: goConfig("model_chat_usage_limit_reached"),
                    });
                    result.push({
                        text: backendStore.translate("Claim Credits"),
                        onClick: goConfig("model_chat_usage_claim_again"),
                    });
                    break;
                case ErrorType.AudioNetworkError: // 音频网络错误
                    break;
                default:
                    break;
            }

            return result;
        });

        // 去配置
        const goConfig = (config: string) => {
            return (event: MouseEvent) => {
                switch (config) {
                    case "model_invalid": // model无效
                    case "model_chat_usage_limit_reached":
                        // 打开模型调用次数超过限制配置页面
                        backendStore.requestWindow("showConfig", SettingNav.MODEL);
                        break;
                    case "model_chat_usage_claim_again":
                        // 函数式显示领取弹窗，await 结果后按 key 分发逻辑
                        void notifyStore
                            .showDialog({
                                title: backendStore.translate("Free Credits Delivered"),
                                content: backendStore.translate(
                                    "You've used up the free generation credits for your trial account. We've given you an extra 200 free credits valid this month. Explore more features and unlock UOS AI's limitless capabilities!",
                                ),
                                buttons: [
                                    { key: "notNow", text: backendStore.translate("Not Now"), type: "default" },
                                    {
                                        key: "claim",
                                        text: backendStore.translate("Claim Credits"),
                                        type: "primary",
                                        suggested: true,
                                    },
                                ],
                            })
                            .then((result) => {
                                if (result.key === "claim") {
                                    backendStore.requestAssistant("claimUsageRequest", props.model);
                                }
                            });
                        break;
                    default:
                        break;
                }
            };
        };

        // 信号监听结果
        const handleClaimUsageComplete = (ok: boolean, msg: string) => {
            console.log("claimUsageComplete in digital human", ok, msg);

            // 如果领取成功，将数字人状态设置为 Silence
            if (ok) {
                digitalHumanStore.digitalHumanState = DigitalHumanState.Silence;
            }

            // 函数式显示领取结果弹窗
            void notifyStore.showDialog({
                title: ok
                    ? backendStore.translate("Successfully Claimed")
                    : backendStore.translate("Failed to Claim. Please Try Again."),
                buttons: [
                    {
                        key: "ok",
                        text: ok ? backendStore.translate("Use it now") : backendStore.translate("Retry"),
                        type: "default",
                    },
                ],
            });
        };

        // 监听 store 中的全局信号结果
        watch(
            () => assistantChannelStore.claimUsageResult,
            (result) => {
                if (result) {
                    handleClaimUsageComplete(result.ok, result.msg);
                }
            },
        );

        // 判断是否显示发送按钮
        const isShowSendButton = () => {
            return props.state === DigitalHumanState.Silence || props.state === DigitalHumanState.Error;
        };

        // 判断是否禁用按钮
        const isButtonDisabled = () => {
            return props.state === DigitalHumanState.Error;
        };

        // 处理按钮点击事件
        const handleButtonClick = () => {
            if (isShowSendButton()) {
                emit("start");
            } else {
                emit("stop");
            }
        };

        // 根据状态获取状态文本
        const getStateText = () => {
            switch (props.state) {
                case DigitalHumanState.Silence:
                    return backendStore.translate("Sleeping");
                case DigitalHumanState.Listen:
                    return backendStore.translate("Listening");
                case DigitalHumanState.Thinking:
                    return backendStore.translate("Thinking");
                case DigitalHumanState.Answer:
                    return backendStore.translate("Answering");
                case DigitalHumanState.Error:
                    return "";
                default:
                    return backendStore.translate("Sleeping");
            }
        };

        // 根据状态获取提示文案
        const getHintText = () => {
            switch (props.state) {
                case DigitalHumanState.Silence:
                    let text = backendStore.translate("Click on the animation%1 to activate");
                    if (props.shortcutList.length > 0) {
                        text = text.replace("%1", "/" + props.shortcutList.join("+"));
                    } else {
                        text = text.replace("%1", "");
                    }
                    return text;
                case DigitalHumanState.Listen:
                    return props.showCountdown
                        ? backendStore
                              .translate("Stop recording after %1 seconds")
                              .replace("%1", props.countdownValue.toString())
                        : backendStore.translate("Click the animation or press Enter to send");
                case DigitalHumanState.Thinking:
                case DigitalHumanState.Answer:
                    return backendStore.translate("Click animation to interrupt");
                case DigitalHumanState.Error:
                    // 使用 errorMessage 计算属性
                    return errorMessage.value || "";
                default:
                    return "";
            }
        };

        return {
            actions,
            errorMessage,
            isShowSendButton,
            isButtonDisabled,
            handleButtonClick,
            getStateText,
            getHintText,
        };
    },

    render() {
        return (
            <div class="digital-human-action">
                {/* 当前状态显示 */}
                {this.$props.state !== DigitalHumanState.Error && (
                    <div class="digital-human-action__state">
                        {this.$props.state === DigitalHumanState.Listen && (
                            <SvgIcon icon="voice-active" size={[16, 16]} class="digital-human-action__voice-icon" />
                        )}
                        <span class="digital-human-action__state-text">{this.getStateText()}</span>
                    </div>
                )}

                {/* 提示文案 */}
                {this.getHintText() && <div class="digital-human-action__hint">{this.getHintText()}</div>}

                {/* 错误状态下的操作按钮 */}
                {this.$props.state === DigitalHumanState.Error && this.actions.length > 0 && (
                    <div class="digital-human-action__actions">
                        {this.actions.map((action, index) => (
                            <ErrorActionButton key={index} text={action.text} onClick={action.onClick} />
                        ))}
                    </div>
                )}

                {/* 发送按钮 */}
                {this.isShowSendButton() && (
                    <Tooltip content={useBackendStore().translate("Activate")} showAfter={1000}>
                        <div class="digital-human-action__send-wrapper">
                            <IconButton
                                class="digital-human-action__send-button"
                                icon="send"
                                iconSize={[16, 16]}
                                onClick={this.handleButtonClick}
                                disabled={this.isButtonDisabled()}
                            />
                        </div>
                    </Tooltip>
                )}
                {/* 停止按钮 */}
                {!this.isShowSendButton() && (
                    <Tooltip content={useBackendStore().translate("Stop")} showAfter={1000}>
                        <div
                            class="digital-human-action__stop-button digital-human-action__send-wrapper"
                            onClick={this.handleButtonClick}
                        >
                            <div class="custom-stop-button"></div>
                        </div>
                    </Tooltip>
                )}
            </div>
        );
    },
});
