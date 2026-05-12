import { computed, defineComponent, onMounted, inject } from "vue";
import McpServicesSelector from "@/views/window/mainwindow/page/chat/components/McpServicesSelector";
import { useConversationManagerStore, useMcpServicesStore } from "@/stores";
import { CHAT_INPUT_KEY } from "@/types/chat-input";

export default defineComponent({
    name: "McpInputExtension",

    setup() {
        const mcpServicesStore = useMcpServicesStore();
        const conversationManagerStore = useConversationManagerStore();
        const chatInputContext = inject(CHAT_INPUT_KEY);

        const enabledMcpServices = computed(() => {
            return mcpServicesStore.services.filter((service) => service.enabled);
        });

        const enabledMcpServiceIds = computed(() => {
            return enabledMcpServices.value.map((service) => service.id);
        });

        const selectedMcpServiceIds = computed(() => {
            return conversationManagerStore.getSelectedMcpServers(
                conversationManagerStore.getCurrentConversationId,
                enabledMcpServiceIds.value,
            );
        });

        const isInputDisabled = computed(() => {
            return chatInputContext?.isInputDisabled?.() ?? false;
        });

        const isDisabled = computed(() => {
            return isInputDisabled.value || enabledMcpServices.value.length === 0;
        });

        const ensureMcpServicesLoaded = async () => {
            if (mcpServicesStore.isLoaded || mcpServicesStore.isLoading) {
                return;
            }

            try {
                await mcpServicesStore.loadPageData();
            } catch (error) {
                console.error("Failed to load MCP services for selector:", error);
            }
        };

        onMounted(() => {
            ensureMcpServicesLoaded();
        });

        const handleToggleMcpService = (serviceId: string, checked: boolean) => {
            const conversationId = conversationManagerStore.getCurrentConversationId;

            if (!conversationId) {
                return;
            }

            const currentSelectedIds = conversationManagerStore.getSelectedMcpServers(
                conversationId,
                enabledMcpServiceIds.value,
            );

            const nextSelectedIds = checked
                ? [...currentSelectedIds, serviceId]
                : currentSelectedIds.filter((currentId) => currentId !== serviceId);

            conversationManagerStore.setSelectedMcpServers(conversationId, nextSelectedIds, enabledMcpServiceIds.value);
        };

        const handleToggleAllMcpServices = (checked: boolean) => {
            const conversationId = conversationManagerStore.getCurrentConversationId;

            if (!conversationId) {
                return;
            }

            conversationManagerStore.toggleSelectAllMcpServers(conversationId, enabledMcpServiceIds.value, checked);
        };

        return {
            enabledMcpServices,
            selectedMcpServiceIds,
            isDisabled,
            handleToggleMcpService,
            handleToggleAllMcpServices,
        };
    },

    render() {
        return (
            <McpServicesSelector
                services={this.enabledMcpServices}
                selectedServiceIds={this.selectedMcpServiceIds}
                disabled={this.isDisabled}
                onToggleService={this.handleToggleMcpService}
                onToggleSelectAll={this.handleToggleAllMcpServices}
            />
        );
    },
});
