import { defineComponent, onBeforeUnmount, computed } from "vue";
import type { PropType } from "vue";
import { useBackendStore } from "@/stores/backend";
import IconButton from "@/components/IconButton";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";
import type { DisplayFile, FileParseStatus } from "@/types/file";
import { ButtonShape } from "@/types/button";
import { getFileCategoryLabel } from "@/utils/filehelper";

/**
 * FilesListPopover component displays a list of hidden files in a popover
 */
export default defineComponent({
    name: "FilesListPopover",
    components: {
        IconButton,
        SvgIcon,
    },
    props: {
        fileList: {
            type: Array as PropType<DisplayFile[]>,
            default: () => [],
        },
        deletable: {
            type: Boolean,
            default: true,
        },
    },
    emits: ["fileClick", "fileDelete"],

    setup(props, { emit }) {
        const backend = useBackendStore();

        // Track created blob URLs for cleanup
        const blobUrls: string[] = [];

        // Get file icon
        const getFileIcon = (file: DisplayFile) => {
            if (!file || !file.imgBase64) return "";

            try {
                const byteCharacters = atob(file.imgBase64);
                const byteNumbers = new Array(byteCharacters.length);
                for (let i = 0; i < byteCharacters.length; i++) {
                    byteNumbers[i] = byteCharacters.charCodeAt(i);
                }
                const byteArray = new Uint8Array(byteNumbers);
                const blob = new Blob([byteArray], { type: "image/png" });
                const url = URL.createObjectURL(blob);
                blobUrls.push(url);
                return url;
            } catch (e) {
                console.error("Failed to convert base64 to blob:", e);
                return "";
            }
        };

        // Handle file click
        const handleFileClick = (event: MouseEvent, file: DisplayFile) => {
            event.stopPropagation();
            emit("fileClick", file);
        };

        // Handle file delete
        const handleFileDelete = (event: MouseEvent, file: DisplayFile) => {
            event.stopPropagation();
            emit("fileDelete", file);
        };

        const getFileTypeLabel = (file: DisplayFile) => {
            return getFileCategoryLabel(file, (key) => backend.translate(key)) || file.fileType || "";
        };

        // Status helper methods
        const isLoading = (status?: FileParseStatus) => {
            return status === "pending" || status === "parsing";
        };

        const isError = (status?: FileParseStatus) => {
            return status === "error";
        };

        const isCompleted = (status?: FileParseStatus) => {
            return !status || status === "completed";
        };

        const loadingText = computed(() => {
            return backend.translate("Parsing...");
        });

        const errorText = computed(() => {
            return backend.translate("Parsing failed");
        });

        // Cleanup blob URLs
        onBeforeUnmount(() => {
            blobUrls.forEach((url) => URL.revokeObjectURL(url));
        });

        return {
            backend,
            getFileIcon,
            handleFileClick,
            handleFileDelete,
            getFileTypeLabel,
            isLoading,
            isError,
            isCompleted,
            loadingText,
            errorText,
        };
    },

    render() {
        const isAdvanced = this.backend.isEnableAdvancedCssFeatures;

        return (
            <div class={["files-list-popover", isAdvanced && "files-list-popover--advanced"]}>
                {this.$props.fileList.map((file) => (
                    <Tooltip key={file.index} content={file.fileNameText || ""} placement="top">
                    <div
                        class={[
                            "files-list-popover__item",
                            this.$props.deletable && "files-list-popover__item--deletable",
                        ]}
                        onClick={(e) => this.handleFileClick(e, file)}
                    >
                        <div class="files-list-popover__icon">
                            <img src={this.getFileIcon(file)} alt="file icon" />
                        </div>
                        <div class="files-list-popover__info">
                            <div class="files-list-popover__name">{file.fileNameText || ""}</div>
                            <div class="files-list-popover__details">
                                {this.isLoading(file.parseStatus) && (
                                    <>
                                        <SvgIcon
                                            icon="loading"
                                            size={[12, 12]}
                                            class="files-list-popover__status-icon files-list-popover__status-icon--loading"
                                        />
                                        <span class="files-list-popover__status-text">{this.loadingText}</span>
                                    </>
                                )}
                                {this.isError(file.parseStatus) && (
                                    <>
                                        <SvgIcon
                                            icon="toast-warning"
                                            size={[12, 12]}
                                            class="files-list-popover__status-icon"
                                        />
                                        <span class="files-list-popover__status-text">{this.errorText}</span>
                                    </>
                                )}
                                {this.isCompleted(file.parseStatus) && (
                                    <>
                                        <span class="files-list-popover__type">{this.getFileTypeLabel(file)}</span>
                                        <span class="files-list-popover__size">{file.fileSizeText || ""}</span>
                                    </>
                                )}
                            </div>
                            {this.$props.deletable && (
                                <div class="files-list-popover__delete">
                                    <IconButton
                                        icon="icon_titlebar_close"
                                        size={[22, 22]}
                                        iconSize={[16, 16]}
                                        shape={ButtonShape.Circle}
                                        variant="filled"
                                        onClick={(e) => this.handleFileDelete(e, file)}
                                    />
                                </div>
                            )}
                        </div>
                    </div>
                    </Tooltip>
                ))}
            </div>
        );
    },
});
