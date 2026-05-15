import { defineComponent } from "vue";
import { useNotifyStore } from "@/stores";
import { useBackendStore } from "@/stores";
import ToastItem from "./ToastItem";

/**
 * Toast 渲染器组件
 *
 * 挂载在窗口根节点，消费 notifyStore.toasts 队列。
 * 支持多个 Toast 实例同时存在（新通知会触发旧通知关闭动画）。
 */
export default defineComponent({
    name: "ToastRenderer",

    setup() {
        const notifyStore = useNotifyStore();
        const backend = useBackendStore();

        return {
            notifyStore,
            isEnableAdvancedCssFeatures: backend.$state.isEnableAdvancedCssFeatures,
        };
    },

    render() {
        const { toasts } = this.notifyStore;
        if (toasts.length === 0) return null;

        return (
            <div class="toast-container">
                {toasts.map((toast) => (
                    <ToastItem
                        key={toast.id}
                        toast={toast}
                        isEnableAdvancedCssFeatures={this.isEnableAdvancedCssFeatures}
                        onClose={() => this.notifyStore._closeToast(toast.id, "dismiss")}
                        onAction={(key: string) => this.notifyStore._closeToast(toast.id, key)}
                        onAnimationEnd={() => this.notifyStore._removeToast(toast.id)}
                    />
                ))}
            </div>
        );
    },
});
