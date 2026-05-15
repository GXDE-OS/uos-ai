import { defineStore } from "pinia";

export const useToggleStateStore = defineStore("toggleState", {
    state: () => ({
        // 深度思考开关状态
        deepThinkEnabled: true,
        // 联网搜索开关状态
        webSearchEnabled: true,
    }),

    getters: {
        // 获取深度思考状态
        isDeepThinkEnabled: (state) => state.deepThinkEnabled,
        // 获取联网搜索状态
        isWebSearchEnabled: (state) => state.webSearchEnabled,
    },

    actions: {
        // 切换深度思考开关
        toggleDeepThink() {
            this.deepThinkEnabled = !this.deepThinkEnabled;
        },

        // 设置深度思考开关状态
        setDeepThink(enabled: boolean) {
            this.deepThinkEnabled = enabled;
        },

        // 切换联网搜索开关
        toggleWebSearch() {
            this.webSearchEnabled = !this.webSearchEnabled;
        },

        // 设置联网搜索开关状态
        setWebSearch(enabled: boolean) {
            this.webSearchEnabled = enabled;
        },

        // 重置所有开关状态
        resetAll() {
            this.deepThinkEnabled = false;
            this.webSearchEnabled = false;
        },
    },
});
