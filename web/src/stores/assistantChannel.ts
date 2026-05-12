import { defineStore } from "pinia";
import { ref } from "vue";

/**
 * Assistant Channel Store
 * 管理 assistantChannel 相关的全局信号和状态
 */
export const useAssistantChannelStore = defineStore("assistantChannel", () => {
    // 领取次数结果
    const claimUsageResult = ref<{ ok: boolean; msg: string } | null>(null);

    /**
     * 初始化 assistantChannel 信号监听
     * 只应该在 channel 设置时调用一次
     */
    const initChannelListeners = (channel: any) => {
        if (!channel) return;

        // 监听领取次数完成信号
        if (channel.claimUsageComplete) {
            channel.claimUsageComplete.connect((ok: boolean, msg: string) => {
                console.log("claimUsageComplete in store", ok, msg);
                claimUsageResult.value = { ok, msg };
            });
        }
    };

    /**
     * 清除领取结果（可选，用于重置状态）
     */
    const clearClaimResult = () => {
        claimUsageResult.value = null;
    };

    return {
        claimUsageResult,
        initChannelListeners,
        clearClaimResult,
    };
});
