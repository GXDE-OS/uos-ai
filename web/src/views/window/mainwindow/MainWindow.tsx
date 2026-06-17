import { defineComponent, computed, onMounted, h } from "vue";
import { useAppUpdateStore, useBackendStore, useMainWindowStore } from "@/stores";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import NewUserGuideDialog, { type NewUserGuidePage } from "@/components/NewUserGuideDialog";
import { getNewUserGuidePages } from "@/assets/configs/newUserGuide";
import { getMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { ensureBuiltInWorkspacePagesRegistered } from "@/views/window/mainwindow/page/builtinPages";
import WindowTitleBar from "@/views/window/mainwindow/titlebar/WindowTitleBar";
import WindowSidebar from "@/views/window/mainwindow/sidebar/WindowSidebar";
import Splitter from "@/components/Splitter";
import Workspace from "@/components/Workspace";
import NotifyDialogRenderer from "@/components/notify/NotifyDialogRenderer";
import AppUpdateFeatureDialog from "@/components/dialog/AppUpdateFeatureDialog";
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
        const backendStore = useBackendStore();
        const appUpdateStore = useAppUpdateStore();
        const guidePages = computed<NewUserGuidePage[]>(() =>
            getNewUserGuidePages(
                (key) => backendStore.translate(key),
                mainWindowStore.isDarkMode,
                backendStore.isChineseLanguage,
            ),
        );
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

        const handleOpenUpdateRecord = () => {
            void backendStore.requestWindow("showUpdateLogWindow").catch((error) => {
                console.warn("[MainWindow] Failed to open update log window:", error);
            });
        };

        // Initialize CSS variable
        onMounted(() => {
            mainWindowStore.updateCssVariable();
            void mainWindowStore.runWorkspacePageEnter(mainWindowStore.workspacePage, null);
            appUpdateStore.initializeAppUpdateReminderChannel(backendStore.systemChannel);
        });

        return {
            guidePages,
            currentWorkspacePage,
            acceptFileDrop,
            handleFileDrop,
            handleSplitterMouseDown,
            handleOpenUpdateRecord,
            mainWindowStore,
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
                {/* 更新特性弹窗 */}
                <AppUpdateFeatureDialog />
                <NewUserGuideDialog
                    visible={this.mainWindowStore.newUserGuideDialogVisible}
                    pages={this.guidePages}
                    currentIndex={this.mainWindowStore.newUserGuideDialogPageIndex}
                    onCancel={() => this.mainWindowStore.closeNewUserGuideDialog()}
                    onPrevious={() => this.mainWindowStore.goToPreviousNewUserGuideDialogPage()}
                    onNext={() => this.mainWindowStore.goToNextNewUserGuideDialogPage(this.guidePages.length)}
                    onUpdateRecord={this.handleOpenUpdateRecord}
                />
            </div>
        );
    },
});
