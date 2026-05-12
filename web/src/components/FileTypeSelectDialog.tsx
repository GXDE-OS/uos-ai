import { defineComponent, computed, ref, watch, onBeforeUnmount, type PropType } from "vue";
import Dialog from "@/components/dialog/BaseDialog";
import SvgIcon from "@/components/SvgIcon";
import { useBackendStore } from "@/stores/backend";
import { FileCategory } from "@/types/uploadfile";
import type { DialogButton } from "@/types/dialog";
import type { MainWindowWorkspaceFileDropCategoryButton } from "@/utils/mainwindow/workspacePages";
import "@/assets/styles/components/FileTypeSelectDialog.css";

export default defineComponent({
    name: "FileTypeSelectDialog",

    props: {
        visible: {
            type: Boolean,
            required: true,
        },
        fileCount: {
            type: Number,
            default: 0,
        },
        filePaths: {
            type: Array as PropType<string[]>,
            default: () => [],
        },
        buttons: {
            type: Array as PropType<MainWindowWorkspaceFileDropCategoryButton[]>,
            default: () => [],
        },
    },

    emits: {
        close: () => true,
        select: (index: number) => Number.isInteger(index) && index >= 0,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const fileIcons = ref<string[]>([]);

        const title = computed(() => backend.translate("File Upload"));
        // 与上传链路中的大纲限制提示保持一致，避免弹窗提示和实际拦截文案不一致。
        const hintText = computed(() => backend.translate("Only supports uploading 1 outline file"));
        const stackCount = computed(() => Math.min(Math.max(props.fileCount, 1), 3));
        const isSingleFile = computed(() => props.fileCount <= 1);
        const resolvedButtons = computed(() =>
            props.buttons.map((button) => {
                if (button.category === FileCategory.Material) {
                    return {
                        ...button,
                        text: backend.translate("As Material"),
                    };
                }

                if (button.category === FileCategory.Outline) {
                    return {
                        ...button,
                        text: backend.translate("As Outline"),
                    };
                }

                return button;
            }),
        );

        const dialogButtons = computed<DialogButton[]>(() =>
            resolvedButtons.value.map((button, index) => ({
                key: String(index),
                text: button.text,
                type: button.type,
            })),
        );

        const iconLayers = computed(() =>
            Array.from({ length: stackCount.value }, (_, index) => ({
                key: index,
                className: `file-type-select-dialog__icon-layer--${stackCount.value}-${index}`,
                iconUrl: fileIcons.value[index] || "",
            })),
        );

        const loadFileIcons = async () => {
            if (!props.visible) {
                fileIcons.value = [];
                return;
            }

            const targetPaths = props.filePaths.slice(0, stackCount.value);
            if (targetPaths.length === 0) {
                fileIcons.value = [];
                return;
            }

            const icons = await Promise.all(
                targetPaths.map(async (filePath) => {
                    try {
                        const iconBase64 = (await backend.requestFile("getFileIconBase64", filePath, 96, 96)) as string;
                        return iconBase64 ? `data:image/png;base64,${iconBase64}` : "";
                    } catch {
                        return "";
                    }
                }),
            );

            fileIcons.value = icons;
        };

        watch(
            () => [props.visible, props.filePaths],
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

        const handleSelect = (index: number) => {
            emit("select", index);
        };

        const handleButtonClick = (key: string) => {
            const index = Number(key);
            if (Number.isNaN(index)) return;
            handleSelect(index);
        };

        return {
            title,
            hintText,
            stackCount,
            isSingleFile,
            iconLayers,
            resolvedButtons,
            dialogButtons,
            fileIcons,
            handleCancel,
            handleSelect,
            handleButtonClick,
        };
    },

    render() {
        return (
            <Dialog
                visible={this.visible}
                title={this.title}
                dialogClass="file-type-select-dialog"
                buttons={this.dialogButtons}
                onCancel={this.handleCancel}
                onButtonClick={this.handleButtonClick}
            >
                <div class="file-type-select-dialog__body">
                    <div class="file-type-select-dialog__stack-wrap">
                        <div class="file-type-select-dialog__stack">
                            {this.iconLayers.map((layer) => (
                                <div key={layer.key} class={["file-type-select-dialog__icon-layer", layer.className]}>
                                    {layer.iconUrl ? (
                                        <img src={layer.iconUrl} class="file-type-select-dialog__icon-image" />
                                    ) : (
                                        <SvgIcon
                                            icon="doccard_text"
                                            size={[64, 64]}
                                            class="file-type-select-dialog__icon"
                                        />
                                    )}
                                </div>
                            ))}
                            <div class="file-type-select-dialog__badge">{this.fileCount}</div>
                        </div>
                    </div>

                    {!this.isSingleFile && (
                        <div class="file-type-select-dialog__actions file-type-select-dialog__actions--multi">
                            <div class="file-type-select-dialog__hint">{this.hintText}</div>
                        </div>
                    )}
                </div>
            </Dialog>
        );
    },
});
