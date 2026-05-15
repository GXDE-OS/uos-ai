import { defineComponent, computed, onMounted, h } from "vue";
import { useMainWindowStore, useNotifyStore } from "@/stores";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import { getMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { ensureBuiltInWorkspacePagesRegistered } from "@/views/window/mainwindow/page/builtinPages";
import WindowTitleBar from "@/views/window/mainwindow/titlebar/WindowTitleBar";
import WindowSidebar from "@/views/window/mainwindow/sidebar/WindowSidebar";
import Splitter from "@/components/Splitter";
import Workspace from "@/components/Workspace";
import NotifyDialogRenderer from "@/components/notify/NotifyDialogRenderer";
import ToastRenderer from "@/components/toast/ToastRenderer";

export default defineComponent({
    name: "MainWindow",
    components: {
        WindowTitleBar,
        WindowSidebar,
        Splitter,
        Workspace,
    },
    setup() {
        ensureBuiltInWorkspacePagesRegistered();

        const mainWindowStore = useMainWindowStore();
        const currentWorkspacePage = computed(() => {
            return (
                getMainWindowWorkspacePage(mainWindowStore.workspacePage) ??
                getMainWindowWorkspacePage(MAIN_WINDOW_WORKSPACE_PAGES.CHAT)
            );
        });
        const acceptFileDrop = computed(() => currentWorkspacePage.value?.acceptFileDrop ?? false);
        const handleFileDrop = computed(() => currentWorkspacePage.value?.handleFileDrop);

        const handleSplitterMouseDown = (e: MouseEvent) => {
            mainWindowStore.startDrag(e);
        };

        // Initialize CSS variable
        onMounted(() => {
            mainWindowStore.updateCssVariable();
            void mainWindowStore.runWorkspacePageEnter(mainWindowStore.workspacePage, null);
        });

        return {
            currentWorkspacePage,
            acceptFileDrop,
            handleFileDrop,
            handleSplitterMouseDown,
        };
    },
    render() {
        const currentWorkspacePageComponent = this.currentWorkspacePage?.component;

        return (
            <div class="main-window">
                <WindowTitleBar />
                <div class="two-column-layout">
                    <WindowSidebar />
                    <Splitter onResize-start={this.handleSplitterMouseDown} handleWidth={1} />
                    <Workspace acceptFileDrop={this.acceptFileDrop} fileDropHandler={this.handleFileDrop}>
                        <div class="content-body">
                            {currentWorkspacePageComponent ? h(currentWorkspacePageComponent) : null}
                        </div>
                    </Workspace>
                </div>
                {/* Toast 通知渲染器 */}
                <ToastRenderer />
                {/* 应用内通知渲染器 */}
                <NotifyDialogRenderer />
            </div>
        );
    },
});
