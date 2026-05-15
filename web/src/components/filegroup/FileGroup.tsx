import { defineComponent, ref, computed, watch, onMounted, onBeforeUnmount, nextTick, type PropType } from "vue";
import FileItem from "@/components/filegroup/FileItem";
import FilesListPopover from "@/components/filegroup/FilesListPopover";
import IconButton from "@/components/IconButton";
import type { DisplayFile } from "@/types/file";
import { DocParsingFileType, FileAlignment, PopoverPlacement, PopoverAlign } from "@/types/file";
import { ButtonShape } from "@/types/button";
import { sortDisplayFilesForGroup } from "@/utils/filehelper";

// Popover width
const POPOVER_WIDTH = 264;

// FileItem width + margin (184px + 8px = 192px per item, from CSS)
// More button width: 48px
// Gap between items: 8px
const FILE_ITEM_WIDTH = 192; // 184px content + 8px margin
const MORE_BUTTON_WIDTH = 48;

/**
 * FileGroup component displays a group of files
 */
export default defineComponent({
    name: "FileGroup",
    props: {
        fileList: {
            type: Array as PropType<DisplayFile[]>,
            default: () => [],
        },
        assistantType: {
            type: Number,
            default: 0,
        },
        isWindowMode: {
            type: Boolean,
            default: true,
        },
        deletable: {
            type: Boolean,
            default: true,
        },
        showSingleImage: {
            type: Boolean,
            default: true,
        },
        align: {
            type: String as PropType<FileAlignment>,
            default: FileAlignment.Right,
        },
        popoverPlacement: {
            type: String as PropType<PopoverPlacement>,
            default: PopoverPlacement.Bottom,
        },
        popoverAlign: {
            type: String as PropType<PopoverAlign>,
            default: PopoverAlign.Right,
        },
        imageDisplayMode: {
            type: String as PropType<'large' | 'small'>,
            default: 'large',
        },
    },
    emits: ["openFile", "deleteFile"],

    setup(props, { emit }) {
        // Container ref and resize observer
        const containerRef = ref<HTMLElement | null>(null);
        const containerWidth = ref(0);
        let resizeObserver: ResizeObserver | null = null;

        // Popover display state and position
        const showPopover = ref(false);
        const moreBtnRef = ref<HTMLElement | null>(null);
        const popoverStyle = ref({});

        // Alignment class
        const alignmentClass = computed(() => {
            return props.align === FileAlignment.Left ? "file-group--align-left" : "file-group--align-right";
        });

        const orderedFiles = computed(() => {
            return sortDisplayFilesForGroup(props.fileList);
        });

        // Check if single image file and should display as image
        const isSingleImage = computed(() => {
            return (
                props.showSingleImage &&
                orderedFiles.value.length === 1 &&
                orderedFiles.value[0].type === DocParsingFileType.Image
            );
        });

        // Calculate max display count based on container width
        const maxDisplayCount = computed(() => {
            if (containerWidth.value === 0) return orderedFiles.value.length;

            // Available width for file items
            const availableWidth = containerWidth.value;

            // Calculate how many files can fit
            let maxFiles = orderedFiles.value.length;

            // Check if we need to collapse (show "+N" button)
            if (maxFiles * FILE_ITEM_WIDTH + MORE_BUTTON_WIDTH > availableWidth) {
                maxFiles = Math.floor((availableWidth - MORE_BUTTON_WIDTH) / FILE_ITEM_WIDTH);
                maxFiles = Math.max(1, maxFiles);
            }

            return Math.min(maxFiles, orderedFiles.value.length);
        });

        const displayFiles = computed(() => {
            return orderedFiles.value.slice(0, maxDisplayCount.value);
        });

        const remainingCount = computed(() => {
            return Math.max(0, orderedFiles.value.length - maxDisplayCount.value);
        });

        const hiddenFiles = computed(() => {
            return orderedFiles.value.slice(maxDisplayCount.value);
        });

        // Image class based on display mode
        const imageClass = computed(() => {
            return [
                "file-group__image",
                alignmentClass.value,
                props.imageDisplayMode === 'small' && "file-group__image--small"
            ];
        });

        const imageFileClass = computed(() => {
            return [
                "file-group__image-file",
                props.imageDisplayMode === 'small' && "file-group__image-file--small"
            ];
        });

        const handleImageClick = (filePath: string) => {
            emit("openFile", filePath);
        };

        const handleImageDeleteClick = (event: MouseEvent) => {
            event.stopPropagation();
            emit("deleteFile", orderedFiles.value[0]);
        };

        const handleFileClick = (file: DisplayFile) => {
            emit("openFile", file.filePath);
        };

        const togglePopover = () => {
            showPopover.value = !showPopover.value;
            if (showPopover.value) {
                nextTick(updatePopoverPosition);
            }
        };

        const updatePopoverPosition = () => {
            if (moreBtnRef.value) {
                const rect = moreBtnRef.value.getBoundingClientRect();
                const popoverWidth = POPOVER_WIDTH;
                const gap = 9;

                let left: string;
                switch (props.popoverAlign) {
                    case PopoverAlign.Left:
                        left = `${rect.left}px`;
                        break;
                    case PopoverAlign.Right:
                        left = `${rect.right - popoverWidth}px`;
                        break;
                    case PopoverAlign.Center:
                        left = `${rect.left + (rect.width - popoverWidth) / 2}px`;
                        break;
                    default:
                        left = `${rect.right - popoverWidth}px`;
                }

                let position: { top?: string; bottom?: string; left: string };

                if (props.popoverPlacement === PopoverPlacement.Top) {
                    position = {
                        bottom: `${window.innerHeight - rect.top + gap}px`,
                        left,
                    };
                } else {
                    position = {
                        top: `${rect.bottom + gap}px`,
                        left,
                    };
                }

                popoverStyle.value = { ...position, left };
            }
        };

        const handlePopoverFileClick = (file: DisplayFile) => {
            showPopover.value = false;
            handleFileClick(file);
        };

        const handleDeleteFile = (file: DisplayFile) => {
            emit("deleteFile", file);
        };

        const handlePopoverDeleteFile = (file: DisplayFile) => {
            showPopover.value = false;
            handleDeleteFile(file);
        };

        const handleClickOutside = (event: MouseEvent) => {
            if (showPopover.value) {
                const target = event.target as HTMLElement;
                const popover = document.querySelector(".file-group__popover-container");
                const button = document.querySelector(".file-group__more-btn");

                if (popover && !popover.contains(target) && button && !button.contains(target)) {
                    showPopover.value = false;
                }
            }
        };

        const handleResize = () => {
            if (containerRef.value) {
                containerWidth.value = containerRef.value.offsetWidth;
            }
        };

        onMounted(() => {
            document.addEventListener("click", handleClickOutside);

            nextTick(() => {
                if (containerRef.value) {
                    containerWidth.value = containerRef.value.offsetWidth;
                    resizeObserver = new ResizeObserver(handleResize);
                    resizeObserver.observe(containerRef.value);
                }
            });
        });

        onBeforeUnmount(() => {
            document.removeEventListener("click", handleClickOutside);
            if (resizeObserver && containerRef.value) {
                resizeObserver.unobserve(containerRef.value);
                resizeObserver.disconnect();
            }
        });

        return {
            containerRef,
            moreBtnRef,
            showPopover,
            popoverStyle,
            alignmentClass,
            orderedFiles,
            isSingleImage,
            displayFiles,
            remainingCount,
            hiddenFiles,
            imageClass,
            imageFileClass,
            handleImageClick,
            handleImageDeleteClick,
            handleFileClick,
            togglePopover,
            handlePopoverFileClick,
            handleDeleteFile,
            handlePopoverDeleteFile,
        };
    },

    render() {
        return (
            <div ref="containerRef" class={["file-group", this.alignmentClass]}>
                {this.isSingleImage ? (
                    <div
                        class={this.imageClass}
                        onClick={() => this.handleImageClick(this.orderedFiles[0].filePath)}
                    >
                        <img class={this.imageFileClass} src={`file://${this.orderedFiles[0].filePath}`} alt="" />
                        {this.$props.deletable && (
                            <div class="file-group__image-delete" onClick={this.handleImageDeleteClick}>
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
                ) : (
                    <div class={["file-group__items", this.alignmentClass]}>
                        {this.displayFiles.map((file) => (
                            <FileItem
                                key={file.index}
                                fileInfo={file}
                                deletable={this.$props.deletable}
                                onOpenFile={this.handleFileClick}
                                onDeleteFile={this.handleDeleteFile}
                            />
                        ))}

                        {this.remainingCount > 0 && (
                            <div
                                ref="moreBtnRef"
                                class={[
                                    "file-group__more-btn",
                                    this.showPopover && "file-group__more-btn--popover-open",
                                ]}
                                onClick={this.togglePopover}
                            >
                                +{this.remainingCount}
                                {this.showPopover && (
                                    <div class="file-group__popover-container" style={this.popoverStyle}>
                                        <FilesListPopover
                                            fileList={this.hiddenFiles}
                                            deletable={this.$props.deletable}
                                            onFileClick={this.handlePopoverFileClick}
                                            onFileDelete={this.handlePopoverDeleteFile}
                                        />
                                    </div>
                                )}
                            </div>
                        )}
                    </div>
                )}
            </div>
        );
    },
});
