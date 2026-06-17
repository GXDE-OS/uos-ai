import { ref, computed } from "vue";
import { defineStore } from "pinia";
import { useAssistantChannelStore } from "./assistantChannel";

export const useBackendStore = defineStore("backend", {
    state: () => ({
        sessionChannel: null,
        windowChannel: null,
        assistantChannel: null,
        systemChannel: null,
        serviceConfigChannel: null,
        conversationChannel: null,
        fileChannel: null,
        audioChannel: null,
        taskChannel: null,
        skillsMgr: null,
        reportChannel: null,
        translations: {},
        isChineseLanguage: false, // 是否为中文环境
        isEnableAdvancedCssFeatures: false, // 是否启用高级 CSS 功能
        themeIconVersion: 0,
    }),

    getters: {},

    actions: {
        setSessionChannel(channel: any) {
            this.sessionChannel = channel;
        },

        setWindowChannel(channel: any) {
            this.windowChannel = channel;
        },

        setAssistantChannel(channel: any) {
            this.assistantChannel = channel;
            // 初始化 assistantChannel 相关的信号监听
            const assistantChannelStore = useAssistantChannelStore();
            assistantChannelStore.initChannelListeners(channel);
        },

        setSystemChannel(channel: any) {
            this.systemChannel = channel;
        },

        bumpThemeIconVersion() {
            this.themeIconVersion += 1;
        },

        setServiceConfigChannel(channel: any) {
            this.serviceConfigChannel = channel;
        },

        setConversationChannel(channel: any) {
            this.conversationChannel = channel;
        },

        setFileChannel(channel: any) {
            this.fileChannel = channel;
        },

        setAudioChannel(channel: any) {
            this.audioChannel = channel;
        },

        setTaskChannel(channel: any) {
            this.taskChannel = channel;
        },

        setSkillsMgr(skillsMgr: any) {
            this.skillsMgr = skillsMgr;
        },

        setReportChannel(channel: any) {
            this.reportChannel = channel;
        },

        requestWindow(method: any, ...args) {
            return this.request(this.windowChannel, method, ...args);
        },

        requestAssistant(method: any, ...args) {
            return this.request(this.assistantChannel, method, ...args);
        },

        requestSession(method: any, ...args) {
            return this.request(this.sessionChannel, method, ...args);
        },

        requestSystem(method: any, ...args) {
            return this.request(this.systemChannel, method, ...args);
        },

        requestServiceConfig(method: any, ...args) {
            return this.request(this.serviceConfigChannel, method, ...args);
        },

        requestConversation(method: any, ...args) {
            return this.request(this.conversationChannel, method, ...args);
        },

        requestFile(method: any, ...args) {
            return this.request(this.fileChannel, method, ...args);
        },

        requestAudio(method: any, ...args) {
            return this.request(this.audioChannel, method, ...args);
        },

        requestTask(method: any, ...args) {
            return this.request(this.taskChannel, method, ...args);
        },

        requestSkillsMgr(method: any, ...args) {
            return this.request(this.skillsMgr, method, ...args);
        },

        requestReport(method: any, ...args) {
            return this.request(this.reportChannel, method, ...args);
        },

        request(channel: any, method: any, ...args) {
            const req = new Promise((resolve, reject) => {
                if (!channel.hasOwnProperty(method)) {
                    reject(new Error("channel does not have method " + method));
                    return;
                }

                if (typeof channel[method] !== "function") {
                    reject(new Error(method + " is not a function"));
                    return;
                }

                channel[method](...args, (res) => {
                    resolve(res);
                });
            });

            return req;
        },

        translate(key: string) {
            return this.translations[key] || key;
        },

        async loadAdvancedCssFeaturesStatus() {
            this.isEnableAdvancedCssFeatures = await this.requestSystem("isEnableAdvancedCssFeatures");
        },

        async loadLanguageStatus() {
            this.isChineseLanguage = await this.requestSystem("checkChineseLanguage");
        },

        async loadTranslations() {
            this.translations = await this.requestSystem("loadTranslations");
        },
    },
});
