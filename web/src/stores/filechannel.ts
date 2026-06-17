import { defineStore } from "pinia";
import { useUploadFilesStore } from "@/stores/uploadfiles";

export const useFileChannelStore = defineStore("fileChannel", {
    state: () => ({}),

    getters: {},

    actions: {
        // 初始化 fileChannel 监听器
        initializeFileChannel(fileChannel: any) {
            if (!fileChannel) {
                console.warn("File channel is not available");
                return;
            }

            const uploadFilesStore = useUploadFilesStore();

            // 监听 fileEvent 信号
            if (fileChannel.fileEvent) {
                fileChannel.fileEvent.connect((event: number, id: string, json: string) => {
                    uploadFilesStore.handleFileEvent(event, id, json);
                });
                uploadFilesStore.setFileChannelListenerRegistered(true);
            }

            void uploadFilesStore.checkScreenshotVisibility();
        },
    },
});
