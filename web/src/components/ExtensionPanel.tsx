import { defineComponent, ref, computed } from "vue";
import SvgIcon from "@/components/SvgIcon";
import Footer from "@/views/window/mainwindow/footer/Footer";
import { useExtensionPanelStore } from "@/stores/extensionPanel";

export default defineComponent({
    name: "ExtensionPanel",
    components: {
        SvgIcon,
    },
    props: {
        visible: {
            type: Boolean,
            default: false,
        },
    },
    emits: ["close"],
    setup(props, { emit }) {
        const hasError = ref(false);
        const errorInfo = ref<Error | null>(null);
        const isPanelFullscreen = computed(() => useExtensionPanelStore().isPanelFullscreen);

        const handleClose = () => {
            emit("close");
        };

        const handleRetry = () => {
            hasError.value = false;
            errorInfo.value = null;
        };

        const panelClass = computed(() => ({
            "extension-panel": true,
            "extension-panel--visible": props.visible,
        }));

        return {
            handleClose,
            handleRetry,
            panelClass,
            hasError,
            errorInfo,
            isPanelFullscreen,
        };
    },
    onErrorCaptured(err: Error) {
        console.error("[ExtensionPanel] Content render error:", err);
        this.hasError = true;
        this.errorInfo = err;
        return false; // Prevent error from propagating
    },
    render() {
        return (
            <div class={this.panelClass}>
                <div class="extension-panel__content">
                    {this.hasError ? (
                        <div class="extension-panel__error">
                            <div class="extension-panel__error-icon">⚠️</div>
                            <p class="extension-panel__error-text">Failed to load content</p>
                            {this.errorInfo && <p class="extension-panel__error-detail">{this.errorInfo.message}</p>}
                            <button class="extension-panel__retry-btn" onClick={this.handleRetry}>
                                Retry
                            </button>
                        </div>
                    ) : this.$slots.default ? (
                        this.$slots.default()
                    ) : (
                        <div class="extension-panel__empty">
                            <div class="extension-panel__empty-icon">📦</div>
                            <p class="extension-panel__empty-text">No content to display</p>
                        </div>
                    )}
                </div>
                {this.isPanelFullscreen && (
                    <Footer isPanelFullscreen={this.isPanelFullscreen} style="max-width: 100%;" />
                )}
            </div>
        );
    },
});
