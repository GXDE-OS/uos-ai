import { defineComponent, computed, ref, watch, onBeforeUnmount, type PropType } from "vue";
import Dialog from "@/components/dialog/BaseDialog";
import SvgIcon from "@/components/SvgIcon";
import ScrollBar from "@/components/ScrollBar";
import { useBackendStore } from "@/stores/backend";
import type { DialogButton } from "@/types/dialog";
import "@/assets/styles/components/ExpiredFilesDialog.css";
import Tooltip from "./Tooltip";

interface ExpiredFile {
    imgBase64?: string;
    filePath: string;
}

export default defineComponent({
    name: "ExpiredFilesDialog",

    props: {
        visible: {
            type: Boolean,
            required: true,
        },
        files: {
            type: Array as PropType<ExpiredFile[]>,
            default: () => [],
        },
    },

    emits: {
        close: () => true,
        confirm: () => true,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const fileIcons = ref<(string | undefined)[]>([]);

        const title = computed(() =>
            props.files.length > 1
                ? backend
                      .translate("The following %1 files are invalid and unavailable. Continue?")
                      .replace("%1", props.files.length.toString())
                : backend.translate("The following file is invalid and unavailable. Continue?"),
        );

        const fileList = computed(() =>
            props.files.map((file, index) => ({
                index: index + 1,
                fileName: file.filePath.split("/").pop() || file.filePath,
                filePath: file.filePath,
                iconUrl: fileIcons.value[index] || "",
            })),
        );

        const dialogButtons = computed<DialogButton[]>(() => [
            {
                key: "cancel",
                text: backend.translate("Cancel"),
                type: "default",
            },
            {
                key: "confirm",
                text: backend.translate("Continue"),
                type: "primary",
            },
        ]);

        const loadFileIcons = async () => {
            if (!props.visible) {
                fileIcons.value = [];
                return;
            }

            if (props.files.length === 0) {
                fileIcons.value = [];
                return;
            }

            const icons = await Promise.all(
                props.files.map(async (file) => {
                    // 如果已有缓存的图标，直接使用
                    if (file.imgBase64) {
                        return file.imgBase64.startsWith("data:")
                            ? file.imgBase64
                            : `data:image/png;base64,${file.imgBase64}`;
                    }

                    // 否则从文件路径加载图标
                    try {
                        const iconBase64 = (await backend.requestFile(
                            "getFileIconBase64",
                            file.filePath,
                            96,
                            96,
                        )) as string;
                        return iconBase64 ? `data:image/png;base64,${iconBase64}` : "";
                    } catch {
                        return "";
                    }
                }),
            );

            fileIcons.value = icons;
        };

        watch(
            () => [props.visible, props.files],
            () => {
                void loadFileIcons();
            },
            { immediate: true, deep: true },
        );

        onBeforeUnmount(() => {
            fileIcons.value = [];
        });

        const handleCancel = () => {
            emit("close");
        };

        const handleConfirm = () => {
            emit("confirm");
        };

        const handleButtonClick = (key: string) => {
            if (key === "cancel") {
                handleCancel();
            } else if (key === "confirm") {
                handleConfirm();
            }
        };

        return {
            title,
            fileList,
            dialogButtons,
            handleCancel,
            handleConfirm,
            handleButtonClick,
        };
    },

    render() {
        return (
            <Dialog
                visible={this.visible}
                title={this.title}
                dialogClass="expired-files-dialog"
                buttons={this.dialogButtons}
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                <div class="expired-files-dialog__body">
                    <ScrollBar size="small">
                        <div class="expired-files-dialog__list">
                            {this.fileList.map((file) => (
                                <Tooltip content={file.fileName}>
                                    <div key={file.index} class="expired-files-dialog__item">
                                        <div class="expired-files-dialog__icon">
                                            {file.iconUrl ? (
                                                <img src={file.iconUrl} class="expired-files-dialog__icon-image" />
                                            ) : (
                                                <SvgIcon icon="doccard_text" size={[24, 24]} />
                                            )}
                                        </div>
                                        <span class="expired-files-dialog__name">{file.fileName}</span>
                                    </div>
                                </Tooltip>
                            ))}
                        </div>
                    </ScrollBar>
                </div>
            </Dialog>
        );
    },
});
