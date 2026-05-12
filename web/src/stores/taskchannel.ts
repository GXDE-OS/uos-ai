import { defineStore } from "pinia";
import { useConversationManagerStore } from "@/stores/conversationmanager";
import { useBackendStore } from "@/stores/backend";

// TaskMode 枚举
export enum TaskMode {
    UploadImage = 1,
    OverrideQuestion = 2,
    AddKnowledgeBase = 3,
    AddAskQuestion = 4,
    ChangeToConversation = 5,
    ChangeToDigitalMode = 6,
}

export const useTaskChannelStore = defineStore("taskChannel", {
    state: () => ({}),

    getters: {},

    actions: {
        // 初始化 taskChannel 监听器
        initializeTaskChannel(taskChannel: any) {
            if (!taskChannel) {
                console.warn("Task channel is not available");
                return;
            }

            // 监听 taskAdded 信号
            if (taskChannel.taskAdded) {
                taskChannel.taskAdded.connect((mode: number) => {
                    console.log("taskAdded signal received, mode:", mode);
                    this.handleTaskAdded(taskChannel, mode);
                });
            }
        },

        // 处理任务添加事件
        handleTaskAdded(taskChannel: any, mode: number) {
            switch (mode) {
                case TaskMode.UploadImage:
                    taskChannel.processPendingAppendImages();
                    break;
                case TaskMode.OverrideQuestion:
                    taskChannel.processPendingOverrideQuestions();
                    break;
                case TaskMode.AddKnowledgeBase:
                    taskChannel.processPendingAddKnowledgeBases();
                    break;
                case TaskMode.AddAskQuestion:
                    taskChannel.processPendingAppendPrompts();
                    break;
                case TaskMode.ChangeToConversation:
                    taskChannel.processPendingChangeToConversations();
                    break;
                case TaskMode.ChangeToDigitalMode:
                    taskChannel.processPendingChangeToDigitalMode();
                    break;
                default:
                    console.warn("Unknown task mode:", mode);
            }
        },

        // 通知后端窗口初始化完成
        notifyWindowCreated(taskChannel: any) {
            if (taskChannel && taskChannel.onWindowCreated) {
                taskChannel.onWindowCreated();
            }
        },
    },
});