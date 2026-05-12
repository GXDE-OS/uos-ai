import { defineStore } from "pinia";

import { useBackendStore } from "./backend";
import type {
    McpService,
    McpServiceDraft,
} from "@/types/mcp-service";

interface McpServicesResponse {
    success: boolean;
    error?: string;
    services?: McpService[];
    runtimeReady?: boolean;
    thirdPartyAgreementAccepted?: boolean;
}

const getBackendStore = () => {
    const backend = useBackendStore();

    if (!backend.serviceConfigChannel) {
        throw new Error("Service config channel is not available");
    }

    return backend;
};

const unwrapResponse = (response: unknown, fallbackMessage: string): McpServicesResponse => {
    const parsedResponse = response as McpServicesResponse | null;

    if (!parsedResponse || parsedResponse.success !== true) {
        throw new Error(parsedResponse?.error || fallbackMessage);
    }

    return parsedResponse;
};

export const useMcpServicesStore = defineStore("mcpServices", {
    state: () => ({
        services: [] as McpService[],
        isLoading: false,
        isLoaded: false,
        thirdPartyAgreementAccepted: false,
        hasLoadedThirdPartyAgreement: false,
    }),

    getters: {
        customServices: (state) => {
            return state.services.filter((service) => !service.isBuiltIn);
        },
    },

    actions: {
        syncResponseState(response: McpServicesResponse) {
            this.services = response.services || [];
            if (typeof response.thirdPartyAgreementAccepted === "boolean") {
                this.thirdPartyAgreementAccepted = response.thirdPartyAgreementAccepted;
                this.hasLoadedThirdPartyAgreement = true;
            }
        },

        async loadPageData() {
            this.isLoading = true;

            try {
                const backend = getBackendStore();
                const response = unwrapResponse(
                    await backend.requestServiceConfig("getMcpServices"),
                    "加载 MCP 服务失败，请稍后重试。",
                );
                this.syncResponseState(response);
                this.isLoaded = true;
            } catch (error) {
                console.error("[mcpServicesStore] Failed to load MCP services page data", error);
                throw error;
            } finally {
                this.isLoading = false;
            }
        },

        async toggleService(serviceId: string, enabled: boolean) {
            try {
                const backend = getBackendStore();
                const response = unwrapResponse(
                    await backend.requestServiceConfig("setMcpServiceEnabled", serviceId, enabled),
                    "更新 MCP 服务状态失败，请稍后重试。",
                );
                this.syncResponseState(response);
            } catch (error) {
                console.error(`[mcpServicesStore] Failed to toggle service "${serviceId}"`, error);
                throw error;
            }
        },

        async saveCustomService(draft: McpServiceDraft) {
            try {
                const backend = getBackendStore();
                const response = unwrapResponse(
                    await backend.requestServiceConfig(
                        "saveMcpService",
                        draft.jsonConfig,
                        draft.description.trim(),
                        draft.id || "",
                    ),
                    "保存 MCP 服务失败，请稍后重试。",
                );
                this.syncResponseState(response);
            } catch (error) {
                console.error("[mcpServicesStore] Failed to save custom service", error);
                throw error;
            }
        },

        async deleteCustomService(serviceId: string) {
            try {
                const backend = getBackendStore();
                const response = unwrapResponse(
                    await backend.requestServiceConfig("deleteMcpService", serviceId),
                    "删除 MCP 服务失败，请稍后重试。",
                );
                this.syncResponseState(response);
            } catch (error) {
                console.error(`[mcpServicesStore] Failed to delete service "${serviceId}"`, error);
                throw error;
            }
        },

        async acceptThirdPartyAgreement() {
            try {
                const backend = getBackendStore();
                await backend.requestServiceConfig("setMcpThirdPartyAgreement", true);
                this.thirdPartyAgreementAccepted = true;
                this.hasLoadedThirdPartyAgreement = true;
            } catch (error) {
                console.error("[mcpServicesStore] Failed to accept MCP third-party agreement", error);
                throw error;
            }

            try {
                await this.loadPageData();
            } catch (error) {
                console.error("[mcpServicesStore] Failed to refresh MCP services after accepting agreement", error);
            }
        },

        async loadThirdPartyAgreement(force = false) {
            if (this.hasLoadedThirdPartyAgreement && !force) {
                return this.thirdPartyAgreementAccepted;
            }

            try {
                const backend = getBackendStore();
                const agreed = await backend.requestServiceConfig("getMcpThirdPartyAgreement");
                this.thirdPartyAgreementAccepted = Boolean(agreed);
                this.hasLoadedThirdPartyAgreement = true;
                return this.thirdPartyAgreementAccepted;
            } catch (error) {
                console.error("[mcpServicesStore] Failed to load MCP third-party agreement", error);
                throw error;
            }
        },
    },
});
