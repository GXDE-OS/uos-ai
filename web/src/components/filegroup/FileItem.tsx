import { defineComponent, computed, watch, onBeforeUnmount, ref, type PropType } from "vue";
import IconButton from "@/components/IconButton";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";
import type { DisplayFile } from "@/types/file";
import { ButtonShape } from "@/types/button";
import { getFileCategoryLabel } from "@/utils/filehelper";
import { useBackendStore } from "@/stores/backend";

/**
 * FileItem component displays a single file with icon and info
 */
export default defineComponent({
    name: "FileItem",
    components: {
        IconButton,
        SvgIcon,
    },
    props: {
        fileInfo: {
            type: Object as PropType<DisplayFile>,
            required: true,
        },
        deletable: {
            type: Boolean,
            default: true,
        },
    },
    emits: ["openFile", "deleteFile"],

    setup(props, { emit }) {
        const backend = useBackendStore();

        const imageUrl = ref("");
        const blobUrls: string[] = [];

        const fileNameText = computed(() => {
            return props.fileInfo.fileNameText || "";
        });

        const fileTypeName = computed(() => {
            return (
                getFileCategoryLabel(props.fileInfo, (key) => backend.translate(key)) || props.fileInfo.fileType || ""
            );
        });

        const fileSizeText = computed(() => {
            return props.fileInfo.fileSizeText || "";
        });

        const parseStatus = computed(() => {
            return props.fileInfo.parseStatus;
        });

        const isLoading = computed(() => {
            return parseStatus.value === "pending" || parseStatus.value === "parsing";
        });

        const isError = computed(() => {
            return parseStatus.value === "error";
        });

        const isCompleted = computed(() => {
            return !parseStatus.value || parseStatus.value === "completed";
        });

        const loadingText = computed(() => {
            return backend.translate("Parsing...");
        });

        const errorText = computed(() => {
            return backend.translate("Parsing failed");
        });

        const base64ToBlob = (base64: string): Blob => {
            const byteCharacters = atob(base64);
            const byteNumbers = new Array(byteCharacters.length);
            for (let i = 0; i < byteCharacters.length; i++) {
                byteNumbers[i] = byteCharacters.charCodeAt(i);
            }
            const byteArray = new Uint8Array(byteNumbers);
            return new Blob([byteArray], { type: "image/png" });
        };

        const createObjectURL = (blob: Blob): string => {
            const url = URL.createObjectURL(blob);
            blobUrls.push(url);
            return url;
        };

        const setFileIcon = (imgBase64: string) => {
            if (!imgBase64) return;
            const blob = base64ToBlob(imgBase64);
            imageUrl.value = createObjectURL(blob);
        };

        const handleFileClick = (event: MouseEvent) => {
            event.stopPropagation();
            emit("openFile", props.fileInfo);
        };

        const handleDeleteClick = async (event: MouseEvent) => {
            event.stopPropagation();
            emit("deleteFile", props.fileInfo);
        };

        watch(
            () => props.fileInfo.imgBase64,
            (newVal) => {
                if (newVal) {
                    setFileIcon(newVal);
                }
            },
            { immediate: true },
        );

        onBeforeUnmount(() => {
            blobUrls.forEach((url) => URL.revokeObjectURL(url));
        });

        return {
            fileNameText,
            fileTypeName,
            fileSizeText,
            imageUrl,
            parseStatus,
            isLoading,
            isError,
            isCompleted,
            loadingText,
            errorText,
            handleFileClick,
            handleDeleteClick,
        };
    },

    render() {
        const fileItemContent = (
            <div class="file-item" onClick={this.handleFileClick}>
                <div class="file-item__icon">
                    <img src={this.imageUrl || ""} alt="file icon" />
                </div>
                <div class="file-item__info">
                    <div class="file-item__name">{this.fileNameText}</div>
                    <div class="file-item__details">
                        {this.isLoading && (
                            <>
                                <SvgIcon
                                    icon="loading"
                                    size={[12, 12]}
                                    class="file-item__status-icon file-item__status-icon--loading"
                                />
                                <span class="file-item__status-text">{this.loadingText}</span>
                            </>
                        )}
                        {this.isError && (
                            <>
                                <SvgIcon icon="toast-warning" size={[12, 12]} class="file-item__status-icon" />
                                <span class="file-item__status-text">{this.errorText}</span>
                            </>
                        )}
                        {this.isCompleted && (
                            <>
                                <span class="file-item__type">{this.fileTypeName}</span>
                                {this.fileSizeText && <span class="file-item__size">{this.fileSizeText}</span>}
                            </>
                        )}
                    </div>
                </div>
                {this.$props.deletable && (
                    <div class="file-item__delete" onClick={this.handleDeleteClick}>
                        <IconButton
                            icon="icon_titlebar_close"
                            size={[22, 22]}
                            iconSize={[16, 16]}
                            shape={ButtonShape.Circle}
                            variant="filled"
                        />
                    </div>
                )}
            </div>
        );

        return (
            <Tooltip content={this.fileNameText} placement="top">
                {fileItemContent}
            </Tooltip>
        );
    },
});
