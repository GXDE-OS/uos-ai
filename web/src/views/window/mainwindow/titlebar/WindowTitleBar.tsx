import { defineComponent, ref, onMounted, onUnmounted, computed, nextTick, watch, type VNodeRef } from "vue";
import { useBackendStore, useMainWindowStore, useModelInfosStore, useAssistantInfosStore } from "@/stores";
import IconButton from "@/components/IconButton";
import ComboBox from "@/components/combobox/ComboBox";
import Menu from "@/components/menu/Menu";
import SvgIcon from "@/components/SvgIcon";
import ModelSelectorDropdown from "./components/ModelSelectorDropdown";
import { ButtonShape } from "@/types/button";
import { AssistantID } from "@/types/assistant";
import type { ModelOption } from "@/types/combobox";
import type { MenuItem } from "@/types/menu";
import { MODEL_NETWORK_ICON_MAP, type Model } from "@/types/model";
import { SettingNav } from "@/types/conversation";
import { getMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { createDefaultConversation } from "@/utils/mainwindow/conversationActions";
import { useThemeIcon } from "@/utils/loadThemeIcon";
import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";

export default defineComponent({
    name: "WindowTitleBar",
    components: {
        Menu,
    },
    setup() {
        // 与侧边栏宽度过渡保持一致，避免标题栏内元素和主布局不同步。
        const SIDEBAR_LAYOUT_TRANSITION_MS = 300;
        const TITLE_BAR_SPLITTER_GAP = 4;
        const NEW_CONVERSATION_SLOT_WIDTH = 34;
        const backend = useBackendStore();
        const mainWindowStore = useMainWindowStore();
        const modelInfosStore = useModelInfosStore();
        const assistantInfosStore = useAssistantInfosStore();

        // 加载应用图标
        const appIconName = ref("UosAiAssistant");
        const appIconUrl = useThemeIcon(appIconName, { width: 30, height: 30 });
        // 左侧区域、吸附区、右侧按钮区的位置都会参与标题栏自适应布局计算。
        const titleBarLeftRef = ref<HTMLElement | null>(null);
        const titleBarLeftRight = ref(0);
        const splitterAttachedRef = ref<HTMLElement | null>(null);
        const titleBarRightRef = ref<HTMLElement | null>(null);
        const titleBarMenuTriggerRef = ref<HTMLElement | null>(null);
        const titleBarMenuVisible = ref(false);
        // 当中间模型选择器和右侧窗口按钮可能发生碰撞时，切到紧凑模式。
        const isModelSelectorCompact = ref(false);
        const isSidebarLayoutAnimating = ref(false);
        const currentThemeOption = ref<number>(0);
        let titleBarLeftResizeObserver: ResizeObserver | null = null;
        let layoutUpdateFrame: number | null = null;
        let sidebarLayoutAnimationTimeout: number | null = null;

        // 模型选择器默认值
        const modelSelectDefault = computed(() => backend.translate("No account"));

        // 窗口状态监听
        backend.windowChannel["windowStateChanged"].connect((state: Number) => {
            // Qt::WindowState
            console.log("window state changed:", state);
            if (state === 2) {
                isMaximized.value = true;
            } else {
                isMaximized.value = false;
            }
        });

        const isMaximized = ref(false);

        // 窗口控制功能
        const handleMinimize = () => {
            backend.requestWindow("minimize");
        };

        const handleMaximize = () => {
            backend.requestWindow("maximize");
        };

        const handleRestore = () => {
            backend.requestWindow("restore");
        };

        const handleClose = () => {
            backend.requestWindow("close");
        };

        const handleOpenSettings = () => {
            backend.requestWindow("showConfig", SettingNav.NORMAL);
        };

        const handleOpenHelp = () => {
            backend.requestWindow("showHelpWindow");
        };

        const handleOpenAbout = () => {
            backend.requestWindow("showAboutWindow");
        };

        const handleAddModel = () => {
            backend.requestAssistant("requestAddModel");
        };

        // 使用 sidebar store 的状态和方法
        const toggleSidebarCollapse = () => {
            startSidebarLayoutAnimation();
            mainWindowStore.toggleSidebarCollapse();
        };

        const handleCreateConversation = async () => {
            await createDefaultConversation();
        };

        const observeTitleBarLeft = () => {
            if (!titleBarLeftResizeObserver) {
                return;
            }

            titleBarLeftResizeObserver.disconnect();
            if (titleBarLeftRef.value) {
                titleBarLeftResizeObserver.observe(titleBarLeftRef.value);
            }
        };

        const updateTitleBarLayout = () => {
            updateTitleBarLeftRight();
            updateModelSelectorCompact();
        };

        const scheduleTitleBarLayoutUpdate = () => {
            if (layoutUpdateFrame !== null) {
                return;
            }

            // 标题栏布局会同时读写多个 DOM 尺寸，统一收敛到一帧内处理，避免 resize 期间抖动或卡顿。
            layoutUpdateFrame = window.requestAnimationFrame(() => {
                layoutUpdateFrame = null;
                updateTitleBarLayout();
            });
        };

        const stopSidebarLayoutAnimation = () => {
            if (sidebarLayoutAnimationTimeout !== null) {
                window.clearTimeout(sidebarLayoutAnimationTimeout);
                sidebarLayoutAnimationTimeout = null;
            }

            isSidebarLayoutAnimating.value = false;
        };

        const startSidebarLayoutAnimation = () => {
            if (sidebarLayoutAnimationTimeout !== null) {
                window.clearTimeout(sidebarLayoutAnimationTimeout);
            }

            isSidebarLayoutAnimating.value = true;

            sidebarLayoutAnimationTimeout = window.setTimeout(() => {
                stopSidebarLayoutAnimation();
            }, SIDEBAR_LAYOUT_TRANSITION_MS + 40);
        };

        const setTitleBarLeftRef = (element: Element | { $el?: Element } | null) => {
            titleBarLeftRef.value = ((element as { $el?: Element } | null)?.$el ?? element) as HTMLElement | null;
            observeTitleBarLeft();
            scheduleTitleBarLayoutUpdate();
        };

        const updateTitleBarLeftRight = () => {
            const titleBarLeft = titleBarLeftRef.value;
            if (!titleBarLeft) {
                titleBarLeftRight.value = 0;
                return;
            }

            const titleBar = titleBarLeft.parentElement;
            // 以左侧最后一个真实可见元素作为锚点，
            // 这样左侧按钮数量变化时，吸附区会自动避让，不需要再维护硬编码宽度。
            const anchorElement = (titleBarLeft.lastElementChild as HTMLElement | null) || titleBarLeft;
            const anchorOffsetRight = anchorElement.offsetLeft - titleBarLeft.offsetLeft + anchorElement.offsetWidth;
            const titleBarLeftRect = titleBarLeft.getBoundingClientRect();

            if (!titleBar) {
                titleBarLeftRight.value = Math.ceil(titleBarLeftRect.left + anchorOffsetRight);
                return;
            }

            const titleBarRect = titleBar.getBoundingClientRect();
            // 这里使用布局盒模型数据而不是 getBoundingClientRect，
            // 避免左侧按钮做 transform 渐显时把锚点“带偏”，导致模型选择器在动画收尾阶段回弹。
            titleBarLeftRight.value = Math.ceil(titleBarLeftRect.left - titleBarRect.left + anchorOffsetRight);
        };

        const updateModelSelectorCompact = () => {
            if (currentWorkspaceBackButton.value) {
                isModelSelectorCompact.value = false;
                return;
            }

            const splitterAttached = splitterAttachedRef.value;
            const titleBarRight = titleBarRightRef.value;
            if (!splitterAttached || !titleBarRight) {
                isModelSelectorCompact.value = false;
                return;
            }

            const splitterRect = splitterAttached.getBoundingClientRect();
            const rightRect = titleBarRight.getBoundingClientRect();
            const rightGap = rightRect.left - splitterRect.right;
            const availableWidth = rightRect.left - splitterRect.left;
            const expandedMinWidth = 148;
            const expandRestoreWidth = 172;

            // 收窄时尽早切紧凑，放宽时留一点回滞，避免临界点附近来回闪烁。
            if (!isModelSelectorCompact.value && (rightGap < 8 || availableWidth < expandedMinWidth)) {
                isModelSelectorCompact.value = true;
                return;
            }

            if (isModelSelectorCompact.value && availableWidth >= expandRestoreWidth) {
                isModelSelectorCompact.value = false;
            }
        };

        const setSplitterAttachedRef = (element: Element | { $el?: Element } | null) => {
            splitterAttachedRef.value = ((element as { $el?: Element } | null)?.$el ?? element) as HTMLElement | null;
            scheduleTitleBarLayoutUpdate();
        };

        const setTitleBarRightRef = (element: Element | { $el?: Element } | null) => {
            titleBarRightRef.value = ((element as { $el?: Element } | null)?.$el ?? element) as HTMLElement | null;
            scheduleTitleBarLayoutUpdate();
        };

        let startX = 0;
        let startY = 0;
        const handleMouseDown = (event: MouseEvent) => {
            // 检查是否为左键点击
            if (event.button !== 0) return;

            // 检查是否点击在按钮上，如果是则不触发拖动
            const target = event.target as HTMLElement;
            if (target.closest(".title-bar__button")) {
                return;
            }
            startX = event.clientX;
            startY = event.clientY;

            // 添加全局鼠标事件监听器
            document.addEventListener("mousemove", handleMouseMove);
            document.addEventListener("mouseup", handleMouseUp);

            // 阻止默认行为和事件冒泡
            event.preventDefault();
            event.stopPropagation();
        };

        const handleMouseUp = (event: MouseEvent) => {
            // 检查是否为左键释放
            if (event.button !== 0) return;

            // 移除全局事件监听器
            document.removeEventListener("mousemove", handleMouseMove);
            document.removeEventListener("mouseup", handleMouseUp);
        };

        const handleMouseMove = (event: MouseEvent) => {
            // 检查是否为左键拖动
            if (event.button !== 0) return;

            // 阻止默认行为和事件冒泡
            event.preventDefault();
            event.stopPropagation();

            // 启动窗口移动
            backend.requestWindow("startMove", startX, startY, event.clientX, event.clientY);
        };

        // 右键菜单
        const handleContextMenu = (event: MouseEvent) => {
            console.log("右键菜单事件", event.clientX, event.clientY);

            // 阻止默认上下文菜单
            event.preventDefault();
            event.stopPropagation();

            backend.requestWindow("systemMenu");
        };

        // 左键双击事件
        const handleDoubleClick = (event: MouseEvent) => {
            // 检查是否为左键双击
            if (event.button !== 0) return;
            console.log("左键双击事件", event.clientX, event.clientY);

            // 阻止默认行为和事件冒泡
            event.preventDefault();
            event.stopPropagation();

            if (isMaximized.value) handleRestore();
            else handleMaximize();
        };

        // 将模型列表转换为combobox选项
        const modelOptions = computed<ModelOption[]>(() => {
            const models = modelInfosStore.getModelList;
            console.log("modelOptions:", models);
            return models.map((model: Model) => ({
                value: model.id,
                label: model.name,
                icon: MODEL_NETWORK_ICON_MAP[model.network],
                group: model.network,
                provider: model.provider,
            }));
        });

        // 当前选中模型的ID，直接从store获取
        const selectedModelId = computed(() => {
            const currentModel = modelInfosStore.getCurrentModel;
            return currentModel?.id || "";
        });

        // 吸附于分界线的容器样式
        const splitterAttachedStyle = computed(() => {
            const sidebarWidth = mainWindowStore.sidebarWidth;
            const leadingReserve = mainWindowStore.isCollapsed
                ? TITLE_BAR_SPLITTER_GAP + NEW_CONVERSATION_SLOT_WIDTH + TITLE_BAR_SPLITTER_GAP
                : TITLE_BAR_SPLITTER_GAP;
            const leftAreaRight =
                titleBarLeftRight.value > 0 ? titleBarLeftRight.value + leadingReserve : 90 + leadingReserve;
            const left = Math.max(sidebarWidth + TITLE_BAR_SPLITTER_GAP, leftAreaRight);
            return {
                left: `${left}px`,
            };
        });

        const currentWorkspacePage = computed(() => {
            return getMainWindowWorkspacePage(mainWindowStore.workspacePage);
        });

        const currentWorkspaceBackButton = computed(() => {
            const backButton = currentWorkspacePage.value?.backButton;
            if (!backButton) {
                return null;
            }

            return {
                ...backButton,
                text: backend.translate(backButton.text),
            };
        });

        // 处理模型选择
        const handleModelSelect = async (value: string | number) => {
            const selectedModel = modelInfosStore.getModelList.find((model: Model) => model.id === value);
            if (selectedModel) {
                const currentAssistant = assistantInfosStore.getCurrentAssistant;
                const assistantId = currentAssistant?.id || AssistantID.UOS_AI;
                await modelInfosStore.setCurrentModelId(selectedModel.id, assistantId);
            }
        };

        // 处理模型选择下拉框展开/收起
        const handleModelDropdownChange = async (isOpen: boolean) => {
            if (isOpen) {
                const currentAssistant = assistantInfosStore.getCurrentAssistant;
                const assistantId = currentAssistant?.id || AssistantID.UOS_AI;
                await modelInfosStore.fetchModelList(assistantId);
            }
        };

        const handleWorkspaceBack = () => {
            void mainWindowStore.goBackWorkspacePage();
        };

        const handleNoDragMouseDown = (event: MouseEvent) => {
            event.stopPropagation();
        };

        const handleInteractiveDoubleClick = (event: MouseEvent) => {
            // 标题栏根节点保留“双击最大化/还原”，
            // 但交互控件区域必须吃掉双击，避免事件透传。
            event.preventDefault();
            event.stopPropagation();
        };

        const handleTitleBarMenuClick = (event: MouseEvent) => {
            event.preventDefault();
            event.stopPropagation();
            titleBarMenuVisible.value = !titleBarMenuVisible.value;
        };

        const handleTitleBarMenuSelect = (item: MenuItem) => {
            if (item.type !== "item") {
                return;
            }

            switch (item.id) {
                case "settings":
                    handleOpenSettings();
                    break;
                case "theme-light":
                    handleSwitchTheme(1);
                    break;
                case "theme-dark":
                    handleSwitchTheme(2);
                    break;
                case "theme-system":
                    handleSwitchTheme(0);
                    break;
                case "help":
                    handleOpenHelp();
                    break;
                case "about":
                    handleOpenAbout();
                    break;
                default:
                    break;
            }
        };

        const handleToggleDigitalHumanPage = () => {
            mainWindowStore.toggleDigitalHumanPage();
        };

        const fetchThemeOption = async () => {
            const option = await backend.requestSystem("themeColorOption");
            currentThemeOption.value = option as number;
        };

        const handleSwitchTheme = async (value: number) => {
            await backend.requestSystem("switchThemeColor", value);
            currentThemeOption.value = value;
        };

        // 检测当前是否在数字人页面
        const isDigitalHumanPage = computed(() => {
            return mainWindowStore.workspacePage === MAIN_WINDOW_WORKSPACE_PAGES.DIGITAL_HUMAN;
        });

        const setTitleBarMenuTriggerRef: VNodeRef = (element) => {
            titleBarMenuTriggerRef.value = element as HTMLElement | null;
        };

        const titleBarMenuItems = computed<MenuItem[]>(() => [
            {
                type: "item",
                id: "settings",
                label: backend.translate("Settings"),
            },
            {
                type: "submenu",
                id: "theme",
                label: backend.translate("Theme"),
                children: [
                    {
                        type: "item",
                        id: "theme-light",
                        label: backend.translate("Light Theme"),
                        checked: currentThemeOption.value === 1,
                    },
                    {
                        type: "item",
                        id: "theme-dark",
                        label: backend.translate("Dark Theme"),
                        checked: currentThemeOption.value === 2,
                    },
                    {
                        type: "item",
                        id: "theme-system",
                        label: backend.translate("System Theme"),
                        checked: currentThemeOption.value === 0,
                    },
                ],
            },
            {
                type: "separator",
            },
            {
                type: "item",
                id: "help",
                label: backend.translate("Help"),
            },
            {
                type: "item",
                id: "about",
                label: backend.translate("About"),
            },
        ]);

        onMounted(() => {
            void fetchThemeOption();
            void nextTick(() => {
                if (typeof ResizeObserver !== "undefined") {
                    // 这里只观察左侧按钮区；其他区域改走按帧测量，
                    // 避免模型选择器模式切换再次触发观察回路。
                    titleBarLeftResizeObserver = new ResizeObserver(() => {
                        scheduleTitleBarLayoutUpdate();
                    });
                    observeTitleBarLeft();
                }
                scheduleTitleBarLayoutUpdate();
            });
            window.addEventListener("resize", scheduleTitleBarLayoutUpdate);
        });

        onUnmounted(() => {
            window.removeEventListener("resize", scheduleTitleBarLayoutUpdate);
            titleBarLeftResizeObserver?.disconnect();
            titleBarLeftResizeObserver = null;
            if (layoutUpdateFrame !== null) {
                window.cancelAnimationFrame(layoutUpdateFrame);
                layoutUpdateFrame = null;
            }
            stopSidebarLayoutAnimation();
        });

        watch(
            () => [
                mainWindowStore.isCollapsed,
                currentWorkspaceBackButton.value,
                selectedModelId.value,
                modelOptions.value.length,
            ],
            () => {
                // 这些状态会改变标题栏实际占位，统一在下一帧后重算布局。
                void nextTick(scheduleTitleBarLayoutUpdate);
            },
        );

        return {
            handleMinimize,
            handleMaximize,
            handleRestore,
            handleClose,
            handleMouseDown,
            handleContextMenu,
            handleDoubleClick,
            isMaximized,
            toggleSidebarCollapse,
            handleCreateConversation,
            appIconUrl,
            isSidebarCollapsed: () => mainWindowStore.isCollapsed,
            modelList: () => modelInfosStore.getModelList,
            modelOptions,
            selectedModelId,
            handleModelSelect,
            handleModelDropdownChange,
            currentWorkspaceBackButton,
            handleNoDragMouseDown,
            handleInteractiveDoubleClick,
            handleTitleBarMenuClick,
            handleTitleBarMenuSelect,
            handleWorkspaceBack,
            handleToggleDigitalHumanPage,
            splitterAttachedStyle,
            setTitleBarLeftRef,
            setSplitterAttachedRef,
            setTitleBarRightRef,
            setTitleBarMenuTriggerRef,
            titleBarMenuVisible,
            titleBarMenuItems,
            titleBarMenuTriggerRef,
            isModelSelectorCompact,
            isSidebarLayoutAnimating,
            handleAddModel,
            isDarkMode: computed(() => mainWindowStore.isDarkMode),
            modelSelectDefault,
            isDigitalHumanPage,
        };
    },
    render() {
        const isSidebarCollapsed = this.isSidebarCollapsed();
        return (
            <div
                class="title-bar"
                onMousedown={this.handleMouseDown}
                onContextmenu={this.handleContextMenu}
                onDblclick={this.handleDoubleClick}
            >
                {/* 标题栏左侧区域，可以放置应用名称或图标 */}
                <div
                    class="title-bar-left"
                    ref={this.setTitleBarLeftRef}
                    onDblclick={this.handleInteractiveDoubleClick}
                >
                    {/* 应用图标 */}
                    {this.appIconUrl && (
                        <img
                            src={this.appIconUrl}
                            alt="UOS AI"
                            class="app-icon"
                            onMousedown={this.handleNoDragMouseDown}
                        />
                    )}
                    {/* 折叠/展开侧边栏按钮 */}
                    <IconButton
                        class="title-bar__button"
                        icon={"icon_titlebar_sidebar"}
                        tooltip={
                            isSidebarCollapsed
                                ? useBackendStore().translate("Expand")
                                : useBackendStore().translate("Collapse")
                        }
                        iconSize={[16, 16]}
                        size={30}
                        shape={ButtonShape.Rounded}
                        onClick={this.toggleSidebarCollapse}
                    />
                </div>

                {/* 标题栏中间区域 */}
                <div class="title-bar-center"></div>

                {/* 吸附于分界线的容器 */}
                <div
                    class={[
                        "title-bar-splitter-attached",
                        this.isSidebarLayoutAnimating && "title-bar-splitter-attached--animated",
                    ]}
                    style={this.splitterAttachedStyle}
                    ref={this.setSplitterAttachedRef}
                    onDblclick={this.handleInteractiveDoubleClick}
                >
                    <div class="title-bar__new-conversation-slot">
                        <div
                            class={[
                                "title-bar__new-conversation-shell",
                                isSidebarCollapsed && "title-bar__new-conversation-shell--visible",
                                this.isSidebarLayoutAnimating && "title-bar__new-conversation-shell--animated",
                            ]}
                        >
                            <IconButton
                                class="title-bar__button title-bar__new-conversation-button"
                                icon={this.isDarkMode ? "icon_new_conversation_dark" : "icon_new_conversation"}
                                tooltip={useBackendStore().translate("New Chat")}
                                iconSize={[16, 16]}
                                size={30}
                                shape={ButtonShape.Rounded}
                                disabled={!isSidebarCollapsed}
                                onClick={this.handleCreateConversation}
                            />
                        </div>
                    </div>
                    {this.currentWorkspaceBackButton ? (
                        <button
                            class="title-bar__back-button"
                            type="button"
                            onClick={this.handleWorkspaceBack}
                            onMousedown={this.handleNoDragMouseDown}
                        >
                            <SvgIcon class="title-bar__back-button-icon" icon="icon_arrow" size={[16, 16]} />
                            <span class="title-bar__back-button-text">{this.currentWorkspaceBackButton.text}</span>
                        </button>
                    ) : (
                        <ComboBox
                            options={this.modelOptions}
                            value={this.selectedModelId}
                            compact={this.isModelSelectorCompact}
                            customClass="model-selector"
                            onUpdateValue={this.handleModelSelect}
                            onChangeDropdown={this.handleModelDropdownChange}
                            defaultValue={this.modelSelectDefault}
                        >
                            {{
                                dropdown: (slotProps: any) => (
                                    <ModelSelectorDropdown
                                        {...slotProps}
                                        onSelect={slotProps.onSelect}
                                        onClose={slotProps.onClose}
                                        onAddModel={this.handleAddModel}
                                    />
                                ),
                            }}
                        </ComboBox>
                    )}
                </div>

                {/* 标题栏右侧按钮区域 */}
                <div
                    class="title-bar-right"
                    ref={this.setTitleBarRightRef}
                    onDblclick={this.handleInteractiveDoubleClick}
                >
                    {/* 数字人按钮 */}
                    <IconButton
                        icon="digital_human"
                        class={this.isDigitalHumanPage ? "title-bar__button--digital_human_active" : ""}
                        tooltip={
                            this.isDigitalHumanPage
                                ? useBackendStore().translate("Exit Voice Chat")
                                : useBackendStore().translate("Voice Chat")
                        }
                        iconSize={[16, 16]}
                        size={40}
                        onClick={this.handleToggleDigitalHumanPage}
                    />
                    {/* 菜单按钮 */}
                    <div ref={this.setTitleBarMenuTriggerRef}>
                        <IconButton
                            class="title-bar__button"
                            icon="icon_titlebar_menu"
                            iconSize={[16, 16]}
                            size={40}
                            onClick={this.handleTitleBarMenuClick}
                        />
                    </div>
                    {/* 最小化按钮 */}
                    <IconButton
                        class="title-bar__button"
                        icon="icon_titlebar_minimize"
                        iconSize={[16, 16]}
                        size={40}
                        onClick={this.handleMinimize}
                    />

                    {/* 根据isMaximized的值来判断显示最大化按钮还是还原按钮 */}
                    {!this.isMaximized ? (
                        <IconButton
                            class="title-bar__button"
                            icon="icon_titlebar_maximize"
                            iconSize={[16, 16]}
                            size={40}
                            onClick={this.handleMaximize}
                        />
                    ) : (
                        <IconButton
                            class="title-bar__button"
                            icon="icon_titlebar_unmaxmize"
                            iconSize={[16, 16]}
                            size={40}
                            onClick={this.handleRestore}
                        />
                    )}
                    {/* 关闭按钮 */}
                    <IconButton
                        class="title-bar__button"
                        icon="icon_titlebar_close"
                        iconSize={[16, 16]}
                        size={40}
                        onClick={this.handleClose}
                    />
                </div>

                <Menu
                    items={this.titleBarMenuItems}
                    visible={this.titleBarMenuVisible}
                    triggerRef={this.titleBarMenuTriggerRef}
                    placement="bottom"
                    checkable={true}
                    onUpdateVisible={(visible) => (this.titleBarMenuVisible = visible)}
                    onSelectItem={this.handleTitleBarMenuSelect}
                />
            </div>
        );
    },
});
