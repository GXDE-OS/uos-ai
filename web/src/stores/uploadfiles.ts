import { ref } from "vue";
import { defineStore } from "pinia";
import { useBackendStore } from "./backend";
import { useNotifyStore } from "./notify";
import type { NativeDropPayload, SelectFileOptions, UploadFile } from "@/types/uploadfile";
import { FileCategory, FileEvent } from "@/types/uploadfile";

type UploadConstraintResult = {
    acceptedPaths: string[];
    toastMessageKey?: string;
};

type IncomingFilesProcessOptions = Pick<SelectFileOptions, "category"> & {
    defaultPrompt?: string;
    backendMethod?: "handleDroppedFiles" | "handleCopiedFiles" | "handleScreenshotFile";
};

type ValidatedIncomingPathsResult = {
    paths: string[];
};

// 附件限制文案统一放在这里，所有入口共用同一套约束判断和提示出口。
export const MAX_UPLOAD_FILES = 50;
export const MAX_UPLOAD_FILES_MESSAGE_KEY = "You can add up to 50 files";
export const MAX_OUTLINE_FILES_MESSAGE_KEY = "Only supports uploading 1 outline file";

const parseFileCategory = (value: unknown): FileCategory | undefined => {
    const category = typeof value === "number" ? value : Number(value);

    if (category === FileCategory.Material || category === FileCategory.Outline) {
        return category;
    }

    return undefined;
};

const parseIncomingFilesBackendMethod = (value: unknown): IncomingFilesProcessOptions["backendMethod"] | undefined => {
    if (value === "handleDroppedFiles" || value === "handleCopiedFiles" || value === "handleScreenshotFile") {
        return value;
    }

    return undefined;
};

/**
 * Upload files store
 * Manages file upload operations and state
 */
export interface ExpiredFile {
    imgBase64?: string;
    filePath: string;
}

export const useUploadFilesStore = defineStore("uploadFiles", {
    state: () => ({
        uploadedFiles: ref<UploadFile[]>([]),
        isScreenshotEnabled: ref(true),
        fileChannelListenerRegistered: ref(false),
        pendingNativeDrop: null as NativeDropPayload | null,
        pendingNativeDropVersion: 0,
        expiredFiles: [] as ExpiredFile[],
        expiredFilesVersion: 0,
        pendingEditMessageId: null as string | null,
        expiredFilesConfirmedVersion: 0,
        validFilesForEdit: null as UploadFile[] | null,
    }),

    getters: {
        // Get uploaded files list
        getUploadedFiles: (state) => state.uploadedFiles,
        // Get file count
        getFileCount: (state) => state.uploadedFiles.length,
        // Check if max files reached
        isMaxFilesReached: (state) => state.uploadedFiles.length >= MAX_UPLOAD_FILES,
        // Check if screenshot is enabled
        getIsScreenshotEnabled: (state) => state.isScreenshotEnabled,
    },

    actions: {
        // Set uploaded files
        setUploadedFiles(files: UploadFile[]) {
            this.uploadedFiles = files;
        },

        // Set screenshot enabled status
        setIsScreenshotEnabled(enabled: boolean) {
            this.isScreenshotEnabled = enabled;
        },

        // Set file channel listener registered status
        setFileChannelListenerRegistered(registered: boolean) {
            this.fileChannelListenerRegistered = registered;
        },

        setPendingNativeDrop(payload: NativeDropPayload) {
            this.pendingNativeDrop = payload;
            this.pendingNativeDropVersion += 1;
        },

        clearPendingNativeDrop() {
            this.pendingNativeDrop = null;
        },

        setExpiredFiles(files: ExpiredFile[], messageId?: string, validFiles?: UploadFile[]) {
            this.expiredFiles = files;
            this.pendingEditMessageId = messageId || null;
            this.validFilesForEdit = validFiles || null;
            this.expiredFilesVersion += 1;
        },

        clearExpiredFiles() {
            this.expiredFiles = [];
            this.pendingEditMessageId = null;
            this.validFilesForEdit = null;
        },

        confirmExpiredFiles() {
            this.expiredFiles = [];
            // 注意：不清除 pendingEditMessageId，让 Message 组件处理后再清除
            this.expiredFilesConfirmedVersion += 1;
        },

        getPendingEditMessageId(): string | null {
            return this.pendingEditMessageId;
        },

        getValidFilesForEdit(): UploadFile[] | null {
            return this.validFilesForEdit;
        },

        clearPendingEditMessage() {
            this.pendingEditMessageId = null;
            this.validFilesForEdit = null;
        },

        async validateIncomingPaths(paths: string[], showUnsupportedToast = true): Promise<string[]> {
            if (paths.length === 0) {
                return [];
            }

            try {
                const backend = useBackendStore();
                const response = await backend.requestFile(
                    "validateIncomingPaths",
                    JSON.stringify({
                        paths,
                        show_unsupported_toast: showUnsupportedToast,
                    }),
                );

                const parsedResponse =
                    typeof response === "string" ? (JSON.parse(response) as ValidatedIncomingPathsResult) : response;

                if (!parsedResponse || !Array.isArray(parsedResponse.paths)) {
                    console.warn("[UploadFiles] Invalid validateIncomingPaths response:", response);
                    return paths;
                }

                return parsedResponse.paths.filter((path): path is string => typeof path === "string" && path.length > 0);
            } catch (error) {
                console.error("[UploadFiles] Failed to validate incoming paths:", error);
                return paths;
            }
        },

        // 大纲文件只允许存在 1 个，所有入口都复用这一份状态判断。
        hasOutlineFile(): boolean {
            return this.uploadedFiles.some((file) => file.category === FileCategory.Outline);
        },

        getRemainingFileSlots(): number {
            return Math.max(0, MAX_UPLOAD_FILES - this.uploadedFiles.length);
        },

        showConstraintToast(messageKey: string): void {
            const notifyStore = useNotifyStore();
            const backend = useBackendStore();

            notifyStore.showToast({
                type: "warning",
                message: backend.translate(messageKey),
            });
        },

        // 入口级快速拦截：只处理“不需要知道具体路径数量”也能判断的条件。
        getEntryConstraintMessageKey(options: Pick<SelectFileOptions, "category"> = {}): string | undefined {
            if (options.category === FileCategory.Outline && this.hasOutlineFile()) {
                return MAX_OUTLINE_FILES_MESSAGE_KEY;
            }

            if (this.getRemainingFileSlots() <= 0) {
                return MAX_UPLOAD_FILES_MESSAGE_KEY;
            }

            return undefined;
        },

        // 真正的上传约束集中在这里：所有拿到路径的入口都只能经过这一层。
        resolveIncomingPaths(
            paths: string[],
            options: Pick<SelectFileOptions, "category"> = {},
        ): UploadConstraintResult {
            const blockerMessageKey = this.getEntryConstraintMessageKey(options);
            if (blockerMessageKey) {
                return {
                    acceptedPaths: [],
                    toastMessageKey: blockerMessageKey,
                };
            }

            let acceptedPaths = paths;
            let toastMessageKey: string | undefined;

            // 大纲文件单次也只允许保留 1 个，避免后续处理时再分散兜底。
            if (options.category === FileCategory.Outline && acceptedPaths.length > 1) {
                acceptedPaths = acceptedPaths.slice(0, 1);
                toastMessageKey = MAX_OUTLINE_FILES_MESSAGE_KEY;
            }

            const limitedPaths = acceptedPaths.slice(0, this.getRemainingFileSlots());
            return {
                acceptedPaths: limitedPaths,
                toastMessageKey: limitedPaths.length < acceptedPaths.length ? MAX_UPLOAD_FILES_MESSAGE_KEY : toastMessageKey,
            };
        },

        async processIncomingFiles(paths: string[], options: IncomingFilesProcessOptions = {}): Promise<void> {
            const { defaultPrompt = "", category, backendMethod = "handleDroppedFiles" } = options;
            const { acceptedPaths, toastMessageKey } = this.resolveIncomingPaths(paths, { category });

            if (toastMessageKey) {
                this.showConstraintToast(toastMessageKey);
            }

            if (acceptedPaths.length === 0) {
                return;
            }

            const backend = useBackendStore();

            if (backendMethod === "handleScreenshotFile") {
                await backend.requestFile("handleScreenshotFile", JSON.stringify({ path: acceptedPaths[0] }));
                return;
            }

            if (backendMethod === "handleCopiedFiles") {
                await backend.requestFile(
                    "handleCopiedFiles",
                    JSON.stringify({
                        paths: acceptedPaths,
                        category,
                    }),
                );
                return;
            }

            await backend.requestFile(
                "handleDroppedFiles",
                JSON.stringify({
                    paths: acceptedPaths,
                    default_prompt: defaultPrompt,
                    category,
                }),
            );
        },

        // Select a file through file dialog
        async selectFile(options: SelectFileOptions = {}): Promise<void> {
            try {
                // 文件选择框打开前先做一次快速预检，避免明知无法上传还继续走系统对话框。
                const blockerMessageKey = this.getEntryConstraintMessageKey(options);
                if (blockerMessageKey) {
                    this.showConstraintToast(blockerMessageKey);
                    return;
                }

                const backend = useBackendStore();
                const params = JSON.stringify({
                    plugin_only: options.pluginOnly ?? false,
                    multiple: options.multiple ?? false,
                    category: options.category,
                });
                await backend.requestFile("selectFile", params);
            } catch (error) {
                console.error("Failed to select file:", error);
                throw error;
            }
        },

        // Request file parsing
        async requestParse(id: string, filePath: string): Promise<void> {
            try {
                const backend = useBackendStore();
                await backend.requestFile("parseFile", id, filePath);
                this.updateFileStatus(filePath, "parsing");
            } catch (error) {
                console.error("Failed to parse file:", error);
                this.updateFileStatus(filePath, "error");
                throw error;
            }
        },

        // Handle dropped files
        async handleDroppedFiles(
            paths: string[],
            defaultPrompt = "",
            options: Pick<SelectFileOptions, "category"> = {},
        ): Promise<void> {
            try {
                await this.processIncomingFiles(paths, {
                    defaultPrompt,
                    category: options.category,
                    backendMethod: "handleDroppedFiles",
                });
            } catch (error) {
                console.error("Failed to handle dropped files:", error);
                throw error;
            }
        },

        // Handle copied files (from clipboard)
        async handleCopiedFiles(paths: string[]): Promise<void> {
            try {
                await this.processIncomingFiles(paths, { backendMethod: "handleCopiedFiles" });
            } catch (error) {
                console.error("Failed to handle copied files:", error);
                throw error;
            }
        },

        // Handle screenshot file
        async handleScreenshotFile(imagePath: string): Promise<void> {
            try {
                await this.processIncomingFiles([imagePath], { backendMethod: "handleScreenshotFile" });
            } catch (error) {
                console.error("Failed to handle screenshot file:", error);
                throw error;
            }
        },

        // Remove a file
        async removeFile(filePath: string): Promise<void> {
            try {
                const backend = useBackendStore();
                await backend.requestFile("removeFile", filePath);
                this.deleteFile(filePath);
            } catch (error) {
                console.error("Failed to remove file:", error);
                throw error;
            }
        },

        // Start screenshot
        async startScreenshot(): Promise<void> {
            try {
                const blockerMessageKey = this.getEntryConstraintMessageKey();
                if (blockerMessageKey) {
                    this.showConstraintToast(blockerMessageKey);
                    return;
                }

                const backend = useBackendStore();
                await backend.requestFile("startScreenshot");
            } catch (error) {
                console.error("Failed to start screenshot:", error);
                throw error;
            }
        },

        // Check if screenshot is enabled
        async checkScreenshotEnabled(): Promise<boolean> {
            try {
                const backend = useBackendStore();
                const result = await backend.requestFile("isEnableScreenshot");
                // 0 means enabled
                const enabled = result === 0;
                this.setIsScreenshotEnabled(enabled);
                return enabled;
            } catch (error) {
                console.error("Failed to check screenshot enabled:", error);
                return false;
            }
        },

        // Add a file to the list
        addFile(file: UploadFile): boolean {
            // 这里只保留数据层兜底，不再负责用户提示，避免 toast 出口再次分散。
            if (file.category === FileCategory.Outline && this.hasOutlineFile()) {
                console.warn("[UploadFiles] Outline file already exists, ignoring:", file.filePath);
                return false;
            }

            const existingFileIndex = this.uploadedFiles.findIndex(
                (currentFile) => currentFile.filePath === file.filePath,
            );
            if (existingFileIndex !== -1) {
                this.uploadedFiles.splice(existingFileIndex, 1);
            }

            if (this.uploadedFiles.length >= MAX_UPLOAD_FILES) {
                console.warn("[UploadFiles] Max files reached, ignoring:", file.filePath);
                return false;
            }

            this.uploadedFiles.push(file);
            return true;
        },

        // Update file status
        updateFileStatus(filePath: string, status: UploadFile["parseStatus"]): void {
            const file = this.uploadedFiles.find((f) => f.filePath === filePath);
            if (file) {
                file.parseStatus = status;
            }
        },

        // Update file parse result
        updateFileParseResult(filePath: string, status: UploadFile["parseStatus"], error: number): void {
            const file = this.uploadedFiles.find((f) => f.filePath === filePath);
            if (file) {
                file.parseStatus = status;
                file.error = error;
            }
        },

        // Delete a file
        deleteFile(filePath: string): void {
            const index = this.uploadedFiles.findIndex((f) => f.filePath === filePath);
            if (index !== -1) {
                this.uploadedFiles.splice(index, 1);
            }
        },

        // Clear all files
        clearFiles(): void {
            this.uploadedFiles = [];
        },

        // Get a detached snapshot of current uploaded files for message caching
        getUploadedFilesSnapshot(): UploadFile[] {
            return this.uploadedFiles.map((file) => ({ ...file }));
        },

        // Get file by file path
        getFileByPath(filePath: string): UploadFile | undefined {
            return this.uploadedFiles.find((f) => f.filePath === filePath);
        },

        // Get file name from file path
        getFileName(filePath: string): string {
            const parts = filePath.split(/[\\/]/);
            return parts[parts.length - 1] || filePath;
        },

        // Generate unique ID
        generateId(): string {
            return Date.now().toString(36) + Math.random().toString(36).substring(2);
        },

        // Handle file event from FileChannel
        handleFileEvent(event: number, id: string, json: string): void {
            console.log("[UploadFiles] Received file event:", event, id, json);

            try {
                const data = JSON.parse(json);

                switch (event) {
                    case FileEvent.FeFileReady: {
                        const filePath = data.file_path;
                        const error = data.error;
                        const fileSize = data.file_size;
                        const icon = data.icon;
                        const defaultPrompt = data.default_prompt;
                        const category = parseFileCategory(data.category);

                        if (error === 0) {
                            const file: UploadFile = {
                                id: id || this.generateId(),
                                filePath,
                                fileName: this.getFileName(filePath),
                                fileSize: fileSize !== undefined ? Number(fileSize) : undefined,
                                icon,
                                defaultPrompt,
                                parseStatus: "pending",
                                error: 0,
                                category,
                            };
                            const added = this.addFile(file);

                            if (added) {
                                // Trigger parse
                                console.log("[UploadFiles] Triggering parse for file:", file);
                                void this.requestParse(file.id, filePath);
                            }
                        } else {
                            console.error("[UploadFiles] File ready error:", error);
                        }
                        break;
                    }

                    case FileEvent.FeParseResult: {
                        console.log("[UploadFiles] Received parse result:", data);
                        const filePath = data.file_path;
                        const error = data.error;
                        this.updateFileParseResult(filePath, error === 0 ? "completed" : "error", error);
                        break;
                    }

                    case FileEvent.FeNativeDrop: {
                        const paths = Array.isArray(data.paths)
                            ? data.paths.filter((path): path is string => typeof path === "string" && path.length > 0)
                            : [];
                        const x = typeof data.x === "number" ? data.x : Number(data.x);
                        const y = typeof data.y === "number" ? data.y : Number(data.y);

                        if (paths.length === 0 || Number.isNaN(x) || Number.isNaN(y)) {
                            console.warn("[UploadFiles] Invalid native drop payload:", data);
                            break;
                        }

                        this.setPendingNativeDrop({ paths, x, y });
                        break;
                    }

                    case FileEvent.FeIncomingFiles: {
                        const paths = Array.isArray(data.paths)
                            ? data.paths.filter((path): path is string => typeof path === "string" && path.length > 0)
                            : [];
                        const defaultPrompt = typeof data.default_prompt === "string" ? data.default_prompt : "";
                        const category = parseFileCategory(data.category);
                        const backendMethod = parseIncomingFilesBackendMethod(data.backend_method);

                        if (paths.length === 0) {
                            console.warn("[UploadFiles] Invalid incoming files payload:", data);
                            break;
                        }

                        void this.processIncomingFiles(paths, {
                            defaultPrompt,
                            category,
                            backendMethod,
                        });
                        break;
                    }

                    default:
                        console.warn("[UploadFiles] Unknown file event:", event);
                }
            } catch (err) {
                console.error("[UploadFiles] Failed to parse file event JSON:", json, err);
            }
        },
    },
});

export default useUploadFilesStore;
