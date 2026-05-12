import { defineComponent, computed } from "vue";
import { useNotifyStore } from "@/stores";
import CommonDialog from "@/components/dialog/CommonDialog";

/**
 * 应用内通知对话框渲染器
 *
 * 挂载在 MainWindow 根节点，消费 notifyStore.dialogs 队列（先进先出）。
 * 同一时刻只显示队头的对话框，关闭后自动显示下一个。
 */
export default defineComponent({
    name: "NotifyDialogRenderer",

    setup() {
        const notifyStore = useNotifyStore();
        // 取队头（FIFO），没有则为 null
        const current = computed(() => notifyStore.dialogs[0] ?? null);
        return { notifyStore, current };
    },

    render() {
        const { current, notifyStore } = this;
        if (!current) return null;

        return (
            <CommonDialog
                visible={true}
                icon={current.options.icon}
                title={current.options.title}
                content={current.options.content ?? ""}
                buttons={current.options.buttons}
                onCancel={() => notifyStore._resolveDialog(current.id, "cancel")}
                onButtonClick={(key: string) => notifyStore._resolveDialog(current.id, key)}
            />
        );
    },
});
