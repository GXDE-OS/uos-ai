import { ref } from "vue";
import { defineStore } from "pinia";
import { useBackendStore } from "./backend";
import { useAssistantInfosStore } from "./assistantinfos";
import { AssistantID } from "@/types/assistant";
import { DEFAULT_MODEL_NETWORK, isModelNetwork, type Model, type ModelNetwork, ModelAbility } from "@/types/model";

type BackendModelPayload = Partial<Record<keyof Model, unknown>>;

const normalizeModelNetwork = (network: unknown): ModelNetwork => {
    return isModelNetwork(network) ? network : DEFAULT_MODEL_NETWORK;
};

const normalizeModel = (model: unknown): Model => {
    const rawModel = model && typeof model === "object" ? (model as BackendModelPayload) : {};

    return {
        id: typeof rawModel.id === "string" ? rawModel.id : String(rawModel.id ?? ""),
        name: typeof rawModel.name === "string" ? rawModel.name : String(rawModel.name ?? ""),
        icon: typeof rawModel.icon === "string" ? rawModel.icon : "",
        network: normalizeModelNetwork(rawModel.network),
        provider: typeof rawModel.provider === "string" ? rawModel.provider : "",
        ability: typeof rawModel.ability === "number" ? rawModel.ability : ModelAbility.MaText,
    };
};

export const useModelInfosStore = defineStore("modelInfos", {
    state: () => ({
        modelList: ref<Model[]>([]),
        loading: ref(true),
        currentModel: ref<Model | null>(null),
        isSignalInitialized: false,
        isHandlingModelListChanged: false,
        pendingModelListRefresh: false,
    }),

    getters: {
        // 获取模型列表
        getModelList: (state) => state.modelList,
        // 获取加载状态
        isLoading: (state) => state.loading,
        // 获取当前模型
        getCurrentModel: (state) => state.currentModel,
    },

    actions: {
        // 初始化监听器
        initializeConnections() {
            const backend = useBackendStore();
            const assistantChannel = backend.assistantChannel as any;

            if (this.isSignalInitialized) {
                return;
            }

            if (!assistantChannel || !assistantChannel.modelListChanged) {
                console.warn("Assistant channel modelListChanged signal is not available");
                return;
            }

            assistantChannel.modelListChanged.connect(() => {
                void this.handleModelListChangedSignal();
            });

            this.isSignalInitialized = true;
        },

        async handleModelListChangedSignal() {
            if (this.isHandlingModelListChanged) {
                this.pendingModelListRefresh = true;
                return;
            }

            this.isHandlingModelListChanged = true;

            try {
                do {
                    this.pendingModelListRefresh = false;

                    const assistantInfosStore = useAssistantInfosStore();
                    const assistantId = assistantInfosStore.getCurrentAssistant?.id || AssistantID.UOS_AI;

                    console.info("Received modelListChanged signal, reloading model list for assistant:", assistantId);
                    await this.loadModelList(assistantId);
                } while (this.pendingModelListRefresh);
            } finally {
                this.isHandlingModelListChanged = false;
            }
        },

        // 设置模型列表
        setModelList(models: Model[]) {
            // models 按name排序
            models.sort((a, b) => a.name.localeCompare(b.name));
            // 先显示uos_ai模型
            this.modelList = models
                .filter((model) => model.provider === "uos_ai")
                .concat(models.filter((model) => model.provider !== "uos_ai"));
        },

        // 设置加载状态
        setLoading(status: boolean) {
            this.loading = status;
        },

        // 设置当前模型
        setCurrentModel(model: Model | null) {
            this.currentModel = model;
        },

        // 从后端获取模型列表（与后端接口保持一致）
        async fetchModelList(assistantId: string): Promise<Model[]> {
            try {
                this.setLoading(true);
                const backend = useBackendStore();
                const result = await backend.requestAssistant("getModelList", assistantId);
                const models = Array.isArray(result) ? result.map((model) => normalizeModel(model)) : [];
                console.info("Loaded model list:", models);
                this.setModelList(models);
                return models;
            } catch (error) {
                console.error("Failed to load model list:", error);
                return [];
            } finally {
                this.setLoading(false);
            }
        },

        // 获取当前选中的模型ID（与后端接口保持一致）
        async getCurrentModelId(assistantId: string): Promise<string> {
            try {
                const backend = useBackendStore();
                const modelId = await backend.requestAssistant("getCurrentModel", assistantId);
                return modelId || "";
            } catch (error) {
                console.error("Failed to get current model:", error);
                return "";
            }
        },

        // 设置当前模型（与后端接口保持一致）
        async setCurrentModelId(modelId: string, assistantId: string): Promise<boolean> {
            try {
                const backend = useBackendStore();
                const success = await backend.requestAssistant("setCurrentModel", modelId, assistantId);
                if (success) {
                    const model = this.modelList.find((m: Model) => m.id === modelId);
                    if (model) {
                        this.setCurrentModel(model);
                    }
                }
                return success;
            } catch (error) {
                console.error("Failed to set current model:", error);
                return false;
            }
        },

        // 加载模型列表和当前模型
        async loadModelList(assistantId: string) {
            await this.fetchModelList(assistantId);
            await this.loadCurrentModel(assistantId);
        },

        // 加载当前选中的模型
        async loadCurrentModel(assistantId: string) {
            if (this.modelList.length === 0) {
                this.setCurrentModel(null);
                return;
            }

            const currentModelId = await this.getCurrentModelId(assistantId);
            if (currentModelId) {
                const model = this.modelList.find((m: Model) => m.id === currentModelId);
                if (model) {
                    this.setCurrentModel(model);
                } else {
                    // 如果保存的模型ID不在列表中，使用第一个模型
                    this.setCurrentModel(this.modelList[0]);
                }
            } else {
                // 没有保存的模型ID，使用第一个模型
                this.setCurrentModel(this.modelList[0]);
            }
        },
    },
});
