import { defineStore } from "pinia";
import type {
    NotifyDialogOptions,
    NotifyDialogInstance,
    NotifyDialogResult,
    SystemNotificationActionPayload,
    SystemNotifyOptions,
    ToastOptions,
    ToastInstance,
    ToastResult,
} from "@/types/notify";
import { useBackendStore } from "@/stores/backend";

// 内部 ID 计数器，保证每个 dialog 实例唯一
let _idCounter = 0;
let _systemNotificationInitialized = false;
const _systemNotificationActionListeners = new Set<(payload: SystemNotificationActionPayload) => void>();

export const useNotifyStore = defineStore("notify", {
    state: () => ({
        /** 对话框队列，先进先出，通常同时只有 1 项 */
        dialogs: [] as NotifyDialogInstance[],
        /** Toast 列表，新通知会触发旧通知关闭 */
        toasts: [] as ToastInstance[],
    }),

    actions: {
        /**
         * 函数式显示消息对话框。
         *
         * 返回 Promise，用户点击按钮后 resolve，结果的 key 为被点击按钮的 key。
         * 点击右上角关闭时 resolve key 为 "cancel"。
         *
         * @example
         * const result = await notifyStore.showDialog({
         *   type: "warning",
         *   title: "Delete Conversation",
         *   content: "This action cannot be undone.",
         *   buttons: [
         *     { key: "cancel", text: "Cancel", type: "default" },
         *     { key: "confirm", text: "Delete", type: "danger", suggested: true },
         *   ],
         * });
         * if (result.key === "confirm") { ... }
         */
        showDialog(options: NotifyDialogOptions): Promise<NotifyDialogResult> {
            return new Promise<NotifyDialogResult>((resolve) => {
                const id = String(++_idCounter);
                this.dialogs.push({ id, options, resolve });
            });
        },

        /**
         * 内部方法：由渲染器组件调用，resolve 指定 dialog 并从队列移除。
         */
        _resolveDialog(id: string, key: string): void {
            const idx = this.dialogs.findIndex((d) => d.id === id);
            if (idx === -1) return;
            const instance = this.dialogs[idx];
            if (!instance) return;
            this.dialogs.splice(idx, 1);
            instance.resolve({ key });
        },

        /**
         * 便捷方法：显示 Cancel / Confirm 两按钮确认框。
         * 点击 Confirm 返回 true，其余（含关闭）返回 false。
         *
         * @example
         * const ok = await notifyStore.confirm("Delete?", "Sure?");
         * if (ok) { ... }
         */
        async confirm(title: string, content?: string, options?: Pick<NotifyDialogOptions, "icon">): Promise<boolean> {
            const result = await this.showDialog({
                icon: options?.icon,
                title,
                content,
                buttons: [
                    { key: "cancel", text: "Cancel", type: "default" },
                    { key: "confirm", text: "Confirm", type: "primary", suggested: true },
                ],
            });
            return result.key === "confirm";
        },

        /**
         * 初始化 systemChannel 的通知事件监听（只会注册一次）
         */
        initializeSystemNotificationChannel(systemChannel: any): void {
            if (_systemNotificationInitialized || !systemChannel?.notificationActionInvoked) {
                return;
            }

            systemChannel.notificationActionInvoked.connect((actionId: string) => {
                const payload: SystemNotificationActionPayload = { actionId };
                _systemNotificationActionListeners.forEach((listener) => listener(payload));
            });

            _systemNotificationInitialized = true;
        },

        /**
         * 订阅系统通知 action 事件，返回取消订阅函数。
         */
        onSystemNotificationAction(listener: (payload: SystemNotificationActionPayload) => void): () => void {
            _systemNotificationActionListeners.add(listener);
            return () => {
                _systemNotificationActionListeners.delete(listener);
            };
        },

        /**
         * 调用后端发送系统通知。
         */
        async showSystemNotification(options: SystemNotifyOptions): Promise<void> {
            const backend = useBackendStore();
            const actionPayload = (options.actions ?? []).map((action) => ({
                key: action.key,
                text: action.text,
            }));
            await backend.requestSystem(
                "notify",
                options.title,
                options.body,
                options.icon ?? "UosAiAssistant",
                actionPayload,
                options.timeoutMs ?? -1,
            );
        },

        // 预留：showToast() — Toast 提示（待后续实现）

        /**
         * 显示 Toast 通知。
         * 新通知会立即关闭现有通知（触发关闭动画）。
         * 返回对象包含 id（用于手动关闭）和 promise（用户操作后 resolve）。
         * 自动关闭或点击关闭按钮时 resolve key 为 "dismiss"。
         *
         * @example
         * const { id, promise } = notifyStore.showToast({
         *   type: "error",
         *   message: "Failed to delete",
         *   actions: [{ key: "retry", text: "Retry" }],
         * });
         * const result = await promise;
         * if (result.key === "retry") { ... }
         */
        showToast(options: ToastOptions): { id: string; promise: Promise<ToastResult> } {
            const id = String(++_idCounter);

            const promise = new Promise<ToastResult>((resolve) => {
                // 关闭现有 Toast（设置 closing 标志触发动画，并 resolve Promise）
                this.toasts.forEach((t) => {
                    if (t.timerId) {
                        clearTimeout(t.timerId);
                        t.timerId = undefined;
                    }
                    t.closing = true;
                    t.resolve({ key: "dismiss" });
                });

                // 设置自动关闭定时器
                // processing 状态默认不自动关闭（常驻显示）
                // 其他状态默认 3 秒后自动关闭
                // 设置为 0 则不自动关闭（单位毫秒）
                const isProcessing = options.type === "processing";
                const defaultDuration = isProcessing ? 0 : 3000;
                const duration = options.duration ?? defaultDuration;
                const timerId = duration > 0 ? setTimeout(() => this._closeToast(id, "dismiss"), duration) : undefined;

                // 添加新 Toast
                this.toasts.push({ id, options, closing: false, timerId, resolve });
            });

            return { id, promise };
        },

        /**
         * 内部方法：关闭指定 Toast（触发动画并 resolve Promise）
         */
        _closeToast(id: string, key: string = "dismiss"): void {
            const toast = this.toasts.find((t) => t.id === id);
            if (toast) {
                // 清除定时器，防止重复触发
                if (toast.timerId) {
                    clearTimeout(toast.timerId);
                    toast.timerId = undefined;
                }
                toast.closing = true;
                toast.resolve({ key });
            }
        },

        /**
         * 内部方法：动画结束后移除 Toast
         */
        _removeToast(id: string): void {
            const idx = this.toasts.findIndex((t) => t.id === id);
            if (idx !== -1) {
                this.toasts.splice(idx, 1);
            }
        },

        /**
         * 关闭指定 Toast 提示
         */
        closeToast(id: string): void {
            this._closeToast(id);
        },

        /**
         * 关闭所有 Toast 提示
         */
        closeAllToasts(): void {
            this.toasts.forEach((t) => {
                // 清除定时器，防止重复触发
                if (t.timerId) {
                    clearTimeout(t.timerId);
                    t.timerId = undefined;
                }
                t.closing = true;
                t.resolve({ key: "dismiss" });
            });
        },
    },
});
