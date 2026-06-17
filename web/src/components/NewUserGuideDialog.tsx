import { defineComponent, computed, nextTick, onBeforeUnmount, onMounted, ref, watch, type PropType } from "vue";
import CommonButton from "@/components/CommonButton";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";
import { useBackendStore } from "@/stores/backend";
import "@/assets/styles/components/NewUserGuideDialog.css";

export interface NewUserGuidePage {
    image: string;
    alt?: string;
    title: string;
    description: string[];
}

export default defineComponent({
    name: "NewUserGuideDialog",

    props: {
        visible: {
            type: Boolean,
            required: true,
        },
        pages: {
            type: Array as PropType<NewUserGuidePage[]>,
            default: () => [],
        },
        currentIndex: {
            type: Number,
            default: 0,
        },
    },

    emits: {
        cancel: () => true,
        previous: () => true,
        next: () => true,
        updateRecord: () => true,
    },

    setup(props, { emit }) {
        const backend = useBackendStore();
        const isEnableAdvancedCssFeatures = computed(() => backend.isEnableAdvancedCssFeatures);
        const closeText = computed(() => backend.translate("Guide Close"));
        const previousText = computed(() => backend.translate("Previous"));
        const nextText = computed(() => backend.translate("Next"));
        const updateRecordText = computed(() => backend.translate("Changelog"));
        const dialogRef = ref<HTMLElement | null>(null);
        const previousActiveElement = ref<HTMLElement | null>(null);

        const totalPages = computed(() => props.pages.length);
        const currentPageIndex = computed(() => {
            if (totalPages.value === 0) {
                return 0;
            }

            return Math.min(Math.max(props.currentIndex, 0), totalPages.value - 1);
        });
        const currentPage = computed(() => props.pages[currentPageIndex.value] ?? null);
        const hasPrevious = computed(() => totalPages.value > 1 && currentPageIndex.value > 0);
        const hasNext = computed(() => totalPages.value > 1 && currentPageIndex.value < totalPages.value - 1);
        const showCloseButton = computed(() => totalPages.value > 0 && currentPageIndex.value === totalPages.value - 1);
        const showUpdateRecordEntry = computed(() => showCloseButton.value);
        const showFooter = computed(() => totalPages.value > 0);

        const focusDialog = () => {
            if (!props.visible) {
                return;
            }

            if (!previousActiveElement.value && document.activeElement instanceof HTMLElement) {
                previousActiveElement.value = document.activeElement;
            }

            nextTick(() => {
                dialogRef.value?.focus();
            });
        };

        watch(
            () => props.visible,
            (visible) => {
                if (visible) {
                    focusDialog();
                    return;
                }

                nextTick(() => {
                    previousActiveElement.value?.focus?.();
                    previousActiveElement.value = null;
                });
            },
            { immediate: true },
        );

        onMounted(() => {
            focusDialog();
        });

        onBeforeUnmount(() => {
            previousActiveElement.value?.focus?.();
            previousActiveElement.value = null;
        });

        const handleClose = () => {
            emit("cancel");
        };

        const handleOverlayClick = () => {
            // handleClose();
        };

        const handleDialogKeyDown = (event: KeyboardEvent) => {
            if (event.key !== "Escape") {
                return;
            }

            event.preventDefault();
            emit("cancel");
        };

        const handlePrevious = () => {
            if (!hasPrevious.value) {
                return;
            }

            emit("previous");
        };

        const handleNext = () => {
            if (!hasNext.value) {
                return;
            }

            emit("next");
        };

        const handleUpdateRecord = () => {
            emit("updateRecord");
            handleClose();
        };

        const handleUpdateRecordKeyDown = (event: KeyboardEvent) => {
            if (event.key !== "Enter" && event.key !== " ") {
                return;
            }

            event.preventDefault();
            event.stopPropagation();
            handleUpdateRecord();
        };

        return {
            dialogRef,
            totalPages,
            currentPageIndex,
            currentPage,
            hasPrevious,
            hasNext,
            showCloseButton,
            showUpdateRecordEntry,
            showFooter,
            handleClose,
            handleOverlayClick,
            handleDialogKeyDown,
            handlePrevious,
            handleNext,
            handleUpdateRecord,
            handleUpdateRecordKeyDown,
            isEnableAdvancedCssFeatures,
            closeText,
            previousText,
            nextText,
            updateRecordText,
        };
    },

    render() {
        if (!this.visible || this.totalPages === 0 || !this.currentPage) {
            return null;
        }

        return (
            <div class="new-user-guide-dialog__overlay" onClick={this.handleOverlayClick}>
                <div
                    ref="dialogRef"
                    class={["new-user-guide-dialog", this.isEnableAdvancedCssFeatures && "new-user-guide-dialog--blur"]}
                    onClick={(event: MouseEvent) => event.stopPropagation()}
                    onKeydown={this.handleDialogKeyDown}
                >
                    <IconButton
                        class="new-user-guide-dialog__close-button"
                        icon="icon_titlebar_close"
                        iconSize={[16, 16]}
                        size={[40, 40]}
                        shape={ButtonShape.Square}
                        tooltip={this.closeText}
                        onClick={this.handleClose}
                    />

                    <section class="new-user-guide-dialog__media">
                        <img
                            class="new-user-guide-dialog__media-image"
                            src={this.currentPage.image}
                            alt={this.currentPage.alt || this.currentPage.title}
                            draggable={false}
                            onDragStart={(event: DragEvent) => event.preventDefault()}
                            loading="eager"
                        />
                    </section>

                    <section class="new-user-guide-dialog__content">
                        <div class="new-user-guide-dialog__header">
                            <h3 class="new-user-guide-dialog__title">{this.currentPage.title}</h3>
                            <span class="new-user-guide-dialog__page-count">
                                {this.currentPageIndex + 1}/{this.totalPages}
                            </span>
                        </div>

                        <ul class="new-user-guide-dialog__description">
                            {this.currentPage.description.map((descriptionLine, index) => (
                                <li class="new-user-guide-dialog__description-item" key={`${index}-${descriptionLine}`}>
                                    <span class="new-user-guide-dialog__description-bullet" aria-hidden="true">
                                        ·
                                    </span>
                                    <span class="new-user-guide-dialog__description-text">{descriptionLine}</span>
                                </li>
                            ))}
                        </ul>

                        {this.showFooter && (
                            <div class="new-user-guide-dialog__footer">
                                {this.showUpdateRecordEntry && (
                                    <div
                                        class="new-user-guide-dialog__update-record-button"
                                        role="button"
                                        tabindex={0}
                                        onClick={this.handleUpdateRecord}
                                        onKeydown={this.handleUpdateRecordKeyDown}
                                    >
                                        <span class="new-user-guide-dialog__update-record-button-text">
                                            {this.updateRecordText}
                                        </span>
                                    </div>
                                )}

                                <div class="new-user-guide-dialog__footer-actions">
                                    {this.hasPrevious && (
                                        <CommonButton
                                            class="new-user-guide-dialog__nav-button"
                                            text={this.previousText}
                                            variant="default"
                                            onClick={this.handlePrevious}
                                        />
                                    )}

                                    {this.hasNext && (
                                        <CommonButton
                                            class="new-user-guide-dialog__nav-button"
                                            text={this.nextText}
                                            variant="filled"
                                            onClick={this.handleNext}
                                        />
                                    )}

                                    {this.showCloseButton && (
                                        <CommonButton
                                            class="new-user-guide-dialog__nav-button"
                                            text={this.closeText}
                                            variant="filled"
                                            onClick={this.handleClose}
                                        />
                                    )}
                                </div>
                            </div>
                        )}
                    </section>
                </div>
            </div>
        );
    },
});
