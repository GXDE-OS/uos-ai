import { defineComponent, computed, watch, ref } from "vue";
import type { PropType } from "vue";
import type { ErrorMsg } from "@/types/conversation";
import { SettingNav, DependencyPackage } from "@/types/conversation";
import ErrorActionButton from "@/views/window/mainwindow/components/ErrorActionButton";
import { ErrorType, HttpErrorType } from "@/types/errortype";
import { useBackendStore, useAssistantChannelStore, useNotifyStore } from "@/stores";

/**
 * 错误信息气泡组件
 */
export default defineComponent({
    name: "Error",

    props: {
        data: {
            type: Object as PropType<ErrorMsg>,
            required: true,
        },
        isLastMessage: {
            type: Boolean,
            default: false,
        },
        hasContent: {
            type: Boolean,
            default: false,
        },
        model: {
            type: String,
            default: "",
        },
        // 是否从历史记录中加载
        isFromHistory: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        // 操作被取消信号
        operationCanceled: () => {
            return true;
        },
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const assistantChannelStore = useAssistantChannelStore();
        const notifyStore = useNotifyStore();

        // 是否已成功领取额度
        const hasClaimedCredits = ref(false);

        // 处理错误消息显示
        const errorMessage = computed(() => {
            const { error, http_error, error_message } = props.data;
            if (error === ErrorType.HttpError && http_error && http_error === HttpErrorType.OperationCanceledError) {
                // 操作被取消，发出信号
                emit("operationCanceled");
            }
            if (
                error === ErrorType.HttpError &&
                http_error &&
                http_error === HttpErrorType.OperationCanceledError &&
                props.hasContent
            ) {
                return "";
            }

            return error_message;
        });

        // 处理错误码
        // 根据错误码填充操作按钮文本及回调函数
        const actions = computed(() => {
            const result: Array<{ text: string; onClick: (event: MouseEvent) => void }> = [];
            switch (props.data.error) {
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
                    if (!props.isFromHistory && props.isLastMessage && !hasClaimedCredits.value) {
                        result.push({
                            text: backendStore.translate("Claim Free Credits"),
                            onClick: goConfig("model_chat_usage_claim_again"),
                        });
                    }
                    break;
                case ErrorType.KnowledgeBasePluginNotInstalled:
                    result.push({
                        text: backendStore.translate("To install"),
                        onClick: goInstall(DependencyPackage.RAG),
                    });
                    break;
                case ErrorType.KnowledgeBaseEmpty:
                    result.push({
                        text: backendStore.translate("Go to configuration"),
                        onClick: goAddKnowledgeBase(),
                    });
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
                        // 配置并显示领取弹窗
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

        // 关闭弹窗
        const handleCloseDialog = () => {
            alertDialogVisible.value = false;
        };

        // 去安装向量化插件
        const goInstall = (app: DependencyPackage) => {
            return (event: MouseEvent) => {
                backendStore.requestServiceConfig("installApp", app);
            };
        };

        // 去添加知识库
        const goAddKnowledgeBase = () => {
            return (event: MouseEvent) => {
                backendStore.requestWindow("showConfig", SettingNav.KNOWLEDGE);
            };
        };

        // 信号监听结果
        const handleClaimUsageComplete = (ok: boolean, msg: string) => {
            console.log("claimUsageComplete in error component", ok, msg);
            // 领取成功后标记已领取，隐藏"领取免费额度"按钮
            if (ok) {
                hasClaimedCredits.value = true;
            } else {
                hasClaimedCredits.value = false;
            }
            // 领取结果弹窗
            void notifyStore.showDialog({
                title: ok
                    ? backendStore.translate("Successfully Claimed")
                    : backendStore.translate("Failed to Claim. Please Try Again."),
                buttons: [
                    {
                        key: "ok",
                        text: ok ? backendStore.translate("Use it now") : backendStore.translate("Retry"),
                        type: "default",
                        suggested: true,
                    },
                ],
            });
        };

        // 监听 store 中的全局信号结果（只在 isLastMessage 时响应）
        watch(
            () => assistantChannelStore.claimUsageResult,
            (result) => {
                if (result && props.isLastMessage && !props.isFromHistory) {
                    handleClaimUsageComplete(result.ok, result.msg);
                }
            },
        );

        return {
            actions,
            errorMessage,
        };
    },

    render() {
        const { errorMessage } = this;
        const { actions } = this;

        return (
            <div class="error">
                <div class="error__content">
                    <div class="error__message">{errorMessage}</div>
                    <div class="error__actions">
                        {actions.map((action, index) => (
                            <ErrorActionButton key={index} text={action.text} onClick={action.onClick} />
                        ))}
                    </div>
                </div>
            </div>
        );
    },
});
