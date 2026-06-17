import { computed, defineComponent, onMounted, ref } from "vue";
import ComboBox from "@/components/combobox/ComboBox";
import TextButton from "@/components/TextButton";
import ScrollBar from "@/components/ScrollBar";
import { useMcpServicesStore, useBackendStore, useNotifyStore } from "@/stores";
import { ComboBoxDropdownAlign, type ComboboxOption } from "@/types/combobox";
import {
    MCP_SERVICE_CATEGORY,
    MCP_SERVICE_EDITOR_MODE,
    MCP_SERVICE_FILTER,
    type McpServiceEditorMode,
    type McpServiceDraft,
    type McpServiceFilter,
} from "@/types/mcp-service";
import McpServiceEditorDialog from "@/views/window/mainwindow/page/settings/mcpservices/components/McpServiceEditorDialog";
import ToolManagementList from "@/views/window/mainwindow/page/settings/common/components/ToolManagementList";
import { convertMcpServiceToToolItem } from "@/views/window/mainwindow/page/settings/common/types";

export default defineComponent({
    name: "McpServicesPage",

    components: {
        ComboBox,
        ScrollBar,
        McpServiceEditorDialog,
        ToolManagementList,
    },

    setup() {
        const mcpServicesStore = useMcpServicesStore();
        const backendStore = useBackendStore();
        const notifyStore = useNotifyStore();

        const currentFilter = ref<McpServiceFilter>(MCP_SERVICE_FILTER.ALL);
        const dialogVisible = ref(false);
        const dialogMode = ref<McpServiceEditorMode>(MCP_SERVICE_EDITOR_MODE.ADD);
        const editingServiceId = ref("");
        const dialogSubmitError = ref("");
        const filterOptions = computed<ComboboxOption[]>(() => {
            return [
                {
                    value: MCP_SERVICE_FILTER.ALL,
                    label: backendStore.translate("All"),
                },
                {
                    value: MCP_SERVICE_FILTER.BUILT_IN,
                    label: backendStore.translate("Built-in Only"),
                },
                {
                    value: MCP_SERVICE_FILTER.CUSTOM,
                    label: backendStore.translate("Custom Added Only"),
                },
            ];
        });

        const editingService = computed(() => {
            return mcpServicesStore.services.find((service) => service.id === editingServiceId.value) || null;
        });

        const filteredServices = computed(() => {
            const orderedServices = [...mcpServicesStore.services].sort((leftService, rightService) => {
                const categoryOrderMap = {
                    [MCP_SERVICE_CATEGORY.SYSTEM_BUILT_IN]: 0,
                    [MCP_SERVICE_CATEGORY.THIRD_PARTY_BUILT_IN]: 1,
                    [MCP_SERVICE_CATEGORY.CUSTOM]: 2,
                };

                const categoryDiff = categoryOrderMap[leftService.category] - categoryOrderMap[rightService.category];

                if (categoryDiff !== 0) {
                    return categoryDiff;
                }

                return leftService.name.localeCompare(rightService.name);
            });

            if (currentFilter.value === MCP_SERVICE_FILTER.BUILT_IN) {
                return orderedServices.filter((service) => service.isBuiltIn);
            }

            if (currentFilter.value === MCP_SERVICE_FILTER.CUSTOM) {
                return orderedServices.filter((service) => !service.isBuiltIn);
            }

            return orderedServices;
        });

        // 转换为通用列表组件格式
        const toolItems = computed(() => {
            return filteredServices.value.map(convertMcpServiceToToolItem);
        });

        const dialogDraft = computed<McpServiceDraft | null>(() => {
            if (!editingService.value) {
                return null;
            }

            return {
                id: editingService.value.id,
                description: editingService.value.description,
                jsonConfig: editingService.value.jsonConfig || "",
            };
        });

        const loadPageData = async () => {
            if (mcpServicesStore.isLoaded) {
                return;
            }

            await mcpServicesStore.loadPageData().catch(() => undefined);
        };

        const handleFilterChange = (filterValue: McpServiceFilter) => {
            currentFilter.value = filterValue;
        };

        const handleFilterOptionClick = (option: ComboboxOption) => {
            handleFilterChange(option.value as McpServiceFilter);
        };

        const openDialog = (mode: McpServiceEditorMode, serviceId = "") => {
            dialogMode.value = mode;
            editingServiceId.value = serviceId;
            dialogSubmitError.value = "";
            dialogVisible.value = true;
        };

        const handleAddService = () => {
            openDialog(MCP_SERVICE_EDITOR_MODE.ADD);
        };

        const handleEditService = (serviceId: string) => {
            openDialog(MCP_SERVICE_EDITOR_MODE.EDIT, serviceId);
        };

        const handleDialogClose = () => {
            dialogVisible.value = false;
            editingServiceId.value = "";
            dialogSubmitError.value = "";
        };

        const handleSaveService = async (draft: McpServiceDraft) => {
            try {
                await mcpServicesStore.saveCustomService(draft);
                handleDialogClose();
            } catch (error) {
                dialogSubmitError.value =
                    error instanceof Error
                        ? error.message
                        : backendStore.translate("Save failed, please try again later.");
            }
        };

        const handleDeleteService = async (serviceId: string) => {
            const targetService = mcpServicesStore.services.find((service) => service.id === serviceId);

            if (!targetService) {
                return;
            }

            const result = await notifyStore.showDialog({
                title: backendStore.translate("Confirm deletion") + " " + targetService.name + "?",
                content: backendStore.translate(
                    "After deletion, this server will be unavailable. Proceed with caution.",
                ),
                buttons: [
                    { key: "cancel", text: backendStore.translate("Cancel"), type: "default" },
                    { key: "confirm", text: backendStore.translate("Delete"), type: "danger" },
                ],
            });

            if (result.key !== "confirm") {
                return;
            }

            await mcpServicesStore.deleteCustomService(serviceId).catch(() => undefined);
        };

        const handleToggleService = async (serviceId: string, enabled: boolean) => {
            await mcpServicesStore.toggleService(serviceId, enabled).catch(() => undefined);
        };

        onMounted(() => {
            void loadPageData();
        });

        const titleText = computed(() => {
            return backendStore.translate("My MCP Server");
        });
        const addButtonText = computed(() => {
            return backendStore.translate("Add MCP Server");
        });

        return {
            mcpServicesStore,
            currentFilter,
            filterOptions,
            toolItems,
            dialogVisible,
            dialogMode,
            editingService,
            dialogDraft,
            dialogSubmitError,
            titleText,
            addButtonText,
            handleFilterOptionClick,
            handleAddService,
            handleEditService,
            handleDialogClose,
            handleSaveService,
            handleDeleteService,
            handleToggleService,
        };
    },

    render() {
        return (
            <div class="mcp-services-page">
                <div class="mcp-services-page__header-container">
                    <div class="mcp-services-page__container">
                        <div class="mcp-services-page__header">
                            <div class="mcp-services-page__header-left">
                                <div class="mcp-services-page__header-content">
                                    <div class="mcp-services-page__title">{this.titleText}</div>
                                </div>
                            </div>

                            <div class="mcp-services-page__actions">
                                <TextButton text={this.addButtonText} onClick={this.handleAddService} />
                                <ComboBox
                                    dropdownAlign={ComboBoxDropdownAlign.Right}
                                    onClickOption={this.handleFilterOptionClick}
                                    options={this.filterOptions}
                                    value={this.currentFilter}
                                />
                            </div>
                        </div>
                    </div>
                </div>

                <div class="mcp-services-page__content">
                    <ScrollBar class="mcp-services-page__scroll" edgeBounce momentum>
                        <div class="mcp-services-page__content-container">
                            <div class="mcp-services-page__container">
                                <section class="mcp-services-page__section">
                                    <ToolManagementList
                                        isLoading={this.mcpServicesStore.isLoading}
                                        items={this.toolItems}
                                        showEditButton={true}
                                        loadingText="loading MCP services..."
                                        emptyText="no MCP services available."
                                        onToggleItem={this.handleToggleService}
                                        onEditItem={this.handleEditService}
                                        onDeleteItem={this.handleDeleteService}
                                    />
                                </section>
                            </div>
                        </div>
                    </ScrollBar>
                </div>

                <McpServiceEditorDialog
                    initialDraft={this.dialogDraft}
                    mode={this.dialogMode}
                    onClose={this.handleDialogClose}
                    onSaveService={this.handleSaveService}
                    serviceName={this.editingService?.name || ""}
                    submitError={this.dialogSubmitError}
                    visible={this.dialogVisible}
                />
            </div>
        );
    },
});
