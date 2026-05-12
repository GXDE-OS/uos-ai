import { ref } from "vue";
import { defineStore } from "pinia";
import type { Assistant } from "@/types/assistant";
import { AssistantID } from "@/types/assistant";
import { DependencyPackage } from "@/types/conversation";
import { useBackendStore } from "./backend";
import { useMcpServicesStore } from "./mcpservices";
import { useNotifyStore } from "./notify";
import {
    DEFAULT_ASSISTANT_VISIBLE_COUNT,
    MAX_ASSISTANT_VISIBLE_COUNT,
    getAssistantSidebarItemIds,
    normalizeAssistantVisibleCount,
    sortSidebarItemsByOrder,
} from "@/utils/mainwindow/sidebarAssistantOrder";

type AssistantOrderNormalizationResult = {
    order: string[];
    hasMismatch: boolean;
};

type AssistantListLoadOptions = {
    shouldExposeNewAssistants?: boolean;
};

type NewAssistantExposureResult = {
    order: string[];
    visibleCount: number;
    exposedAssistantIds: string[];
};

// 仅保留非空且不重复的侧边栏项 id，避免把脏数据直接传给后端。
const normalizeAssistantOrderValue = (orderValue: unknown): string[] => {
    if (!Array.isArray(orderValue)) {
        return [];
    }

    const normalizedOrder: string[] = [];
    const seen = new Set<string>();

    for (const item of orderValue) {
        if (typeof item !== "string" || item.length === 0 || seen.has(item)) {
            continue;
        }
        seen.add(item);
        normalizedOrder.push(item);
    }

    return normalizedOrder;
};

// 用当前助手列表校正后端返回的顺序配置。
// 这里会过滤不存在的项、去重，并把漏掉的助手区固定项补到末尾。
const normalizeAssistantOrder = (assistants: Assistant[], orderValue: unknown): AssistantOrderNormalizationResult => {
    const sidebarItemIds = getAssistantSidebarItemIds(assistants);
    const sidebarItemIdSet = new Set(sidebarItemIds);
    const storedOrder = Array.isArray(orderValue) ? orderValue : [];
    const normalizedOrder: string[] = [];
    const seen = new Set<string>();
    const hasStoredOrder = storedOrder.length > 0;
    let hasMismatch = false;

    for (const item of storedOrder) {
        if (typeof item !== "string" || item.length === 0) {
            hasMismatch = hasStoredOrder;
            continue;
        }

        if (seen.has(item) || !sidebarItemIdSet.has(item)) {
            hasMismatch = hasStoredOrder;
            continue;
        }

        seen.add(item);
        normalizedOrder.push(item);
    }

    for (const sidebarItemId of sidebarItemIds) {
        if (!seen.has(sidebarItemId)) {
            normalizedOrder.push(sidebarItemId);
            if (hasStoredOrder) {
                hasMismatch = true;
            }
        }
    }

    return {
        order: normalizedOrder,
        hasMismatch,
    };
};

// 按持久化顺序排序助手；顺序配置里可能包含助手区固定项，排序时直接忽略即可。
const sortAssistantsByOrder = (assistants: Assistant[], order: string[]): Assistant[] => {
    return sortSidebarItemsByOrder(assistants, order);
};

// assistantChanged 刷新后只对真正新增的侧边栏助手做常驻区曝光，内置通用助手不参与侧边栏排序。
const getNewSidebarAssistantIds = (previousAssistants: Assistant[], nextAssistants: Assistant[]): string[] => {
    const previousAssistantIds = new Set(previousAssistants.map((assistant) => assistant.id));
    const nextAssistantIds: string[] = [];
    const seen = new Set<string>();

    for (const assistant of nextAssistants) {
        const assistantId = assistant.id;

        if (
            assistantId === AssistantID.UOS_AI ||
            previousAssistantIds.has(assistantId) ||
            seen.has(assistantId)
        ) {
            continue;
        }

        seen.add(assistantId);
        nextAssistantIds.push(assistantId);
    }

    return nextAssistantIds;
};

// 将新增助手插入常驻区边界：0 个时扩到 1 个，未满 4 个时扩 1 个，已满 4 个时替换第四个展示位。
const exposeNewAssistantsInPersistentArea = (
    order: string[],
    newAssistantIds: string[],
    visibleCount: number,
): NewAssistantExposureResult => {
    let nextOrder = [...order];
    let nextVisibleCount = normalizeAssistantVisibleCount(visibleCount);
    const exposedAssistantIds: string[] = [];

    for (const assistantId of newAssistantIds) {
        const assistantIndex = nextOrder.indexOf(assistantId);

        if (assistantIndex < 0) {
            continue;
        }

        nextOrder = nextOrder.filter((itemId) => itemId !== assistantId);
        const insertIndex = Math.min(
            nextOrder.length,
            Math.max(0, Math.min(nextVisibleCount, MAX_ASSISTANT_VISIBLE_COUNT - 1)),
        );
        nextOrder.splice(insertIndex, 0, assistantId);

        if (nextVisibleCount < MAX_ASSISTANT_VISIBLE_COUNT) {
            nextVisibleCount = normalizeAssistantVisibleCount(nextVisibleCount + 1);
        }

        exposedAssistantIds.push(assistantId);
    }

    return {
        order: nextOrder,
        visibleCount: nextVisibleCount,
        exposedAssistantIds,
    };
};

// 拖拽发生在“某个助手被临时插到常驻区”的展示列表上，这里把展示顺序还原成可持久化的真实顺序。
const buildPersistedOrderFromDisplayOrder = (
    displayOrder: string[],
    currentOrder: string[],
    temporarilyInsertedAssistantId?: string | null,
): string[] => {
    const persistedOrder = [...currentOrder];
    const sidebarItemIdSet = new Set(persistedOrder);
    const normalizedDisplayOrder: string[] = [];
    const seen = new Set<string>();

    for (const itemId of displayOrder) {
        if (!sidebarItemIdSet.has(itemId) || seen.has(itemId)) {
            continue;
        }

        seen.add(itemId);
        normalizedDisplayOrder.push(itemId);
    }

    for (const itemId of persistedOrder) {
        if (!seen.has(itemId)) {
            normalizedDisplayOrder.push(itemId);
        }
    }

    if (!temporarilyInsertedAssistantId || !sidebarItemIdSet.has(temporarilyInsertedAssistantId)) {
        return normalizedDisplayOrder;
    }

    const temporarilyInsertedAssistantIndex = persistedOrder.indexOf(temporarilyInsertedAssistantId);
    if (temporarilyInsertedAssistantIndex < 0) {
        return normalizedDisplayOrder;
    }

    const orderWithoutInsertedAssistant = normalizedDisplayOrder.filter(
        (assistantId) => assistantId !== temporarilyInsertedAssistantId,
    );
    const nextPersistedOrder: string[] = [];
    let orderWithoutInsertedAssistantIndex = 0;

    for (let index = 0; index < persistedOrder.length; index += 1) {
        if (index === temporarilyInsertedAssistantIndex) {
            nextPersistedOrder.push(temporarilyInsertedAssistantId);
            continue;
        }

        const assistantId = orderWithoutInsertedAssistant[orderWithoutInsertedAssistantIndex];
        if (assistantId) {
            nextPersistedOrder.push(assistantId);
            orderWithoutInsertedAssistantIndex += 1;
        }
    }

    return nextPersistedOrder;
};

export const useAssistantInfosStore = defineStore("assistantInfos", {
    state: () => ({
        assistantList: ref<Assistant[]>([]),
        loading: ref(true),
        currentAssistant: ref<Assistant | null>(null),
        assistantOrder: ref<string[]>([]),
        assistantVisibleCount: ref(DEFAULT_ASSISTANT_VISIBLE_COUNT),
        hasLoadedAssistantList: false,
        isSignalInitialized: false,
        isHandlingAssistantChanged: false,
        pendingAssistantRefresh: false,
    }),

    getters: {
        // 获取助手列表
        getAssistantList: (state) => state.assistantList,
        // 获取助手区侧边栏顺序（包含固定入口项）
        getAssistantOrder: (state) => state.assistantOrder,
        // 获取助手区常驻显示数量
        getAssistantVisibleCount: (state) => state.assistantVisibleCount,
        // 获取加载状态
        isLoading: (state) => state.loading,
        // 获取当前助手
        getCurrentAssistant: (state) => state.currentAssistant,
    },

    actions: {
        // 监听后端助手列表变化，保证插件增删后前端列表能自动刷新并重新套用排序。
        initializeConnections() {
            const backend = useBackendStore();
            const assistantChannel = backend.assistantChannel as any;

            if (this.isSignalInitialized) {
                return;
            }

            if (!assistantChannel || !assistantChannel.assistantChanged) {
                console.warn("Assistant channel assistantChanged signal is not available");
                return;
            }

            assistantChannel.assistantChanged.connect(() => {
                void this.handleAssistantChangedSignal();
            });

            this.isSignalInitialized = true;
        },

        // 合并连续的 assistantChanged 信号，避免短时间内重复刷新。
        async handleAssistantChangedSignal() {
            if (this.isHandlingAssistantChanged) {
                this.pendingAssistantRefresh = true;
                return;
            }

            this.isHandlingAssistantChanged = true;

            try {
                do {
                    this.pendingAssistantRefresh = false;
                    console.info("Received assistantChanged signal, reloading assistant list");
                    await this.loadAssistantList(useBackendStore(), { shouldExposeNewAssistants: true });
                } while (this.pendingAssistantRefresh);
            } finally {
                this.isHandlingAssistantChanged = false;
            }
        },

        // 设置助手列表
        setAssistantList(assistants: Assistant[]) {
            this.assistantList = assistants;
        },

        // 设置加载状态
        setLoading(status: boolean) {
            this.loading = status;
        },

        // 设置当前助手
        setCurrentAssistant(assistant: Assistant | null) {
            if (!assistant) {
                this.currentAssistant = null;
                return;
            }

            this.currentAssistant = this.getAssistantById(assistant.id) || assistant;

            this.checkEnvironment();
        },

        // 根据id获取助手
        getAssistantById(id: string) {
            // 从助手列表中查找助手
            return this.assistantList.find((assistant) => assistant.id === id);
        },

        // 加载助手列表
        async loadAssistantList(backend: any = useBackendStore(), options: AssistantListLoadOptions = {}) {
            try {
                this.setLoading(true);
                const previousAssistants = [...this.assistantList];
                const shouldExposeNewAssistants = options.shouldExposeNewAssistants && this.hasLoadedAssistantList;
                const result = await backend.requestAssistant("getAssistantList");
                const assistants = Array.isArray(result) ? result : [];
                // 给每个助手添加 envExists 字段
                assistants.forEach((assistant: Assistant) => {
                    assistant.envExists = true;
                });
                // 用本地已加载的顺序配置校正后端助手列表，兼容顺序配置与助手列表不一致的场景。
                const { order: normalizedOrder, hasMismatch } = normalizeAssistantOrder(
                    assistants,
                    this.assistantOrder,
                );

                let nextOrder = normalizedOrder;
                let nextVisibleCount = normalizeAssistantVisibleCount(this.assistantVisibleCount);
                const previousVisibleCount = nextVisibleCount;
                let exposedAssistantIds: string[] = [];
                const newAssistantIds = shouldExposeNewAssistants
                    ? getNewSidebarAssistantIds(previousAssistants, assistants)
                    : [];

                if (newAssistantIds.length > 0) {
                    const exposureResult = exposeNewAssistantsInPersistentArea(
                        normalizedOrder,
                        newAssistantIds,
                        nextVisibleCount,
                    );
                    nextOrder = exposureResult.order;
                    nextVisibleCount = exposureResult.visibleCount;
                    exposedAssistantIds = exposureResult.exposedAssistantIds;
                    console.info("Exposed new assistants in persistent sidebar area:", exposedAssistantIds);
                }

                this.assistantOrder = nextOrder;
                this.assistantVisibleCount = nextVisibleCount;
                this.setAssistantList(sortAssistantsByOrder(assistants, nextOrder));

                const currentAssistantId = this.currentAssistant?.id;
                // 优先保留当前助手；如果当前助手已失效，则回退到 UOS_AI，再退回列表第一个。
                const nextCurrentAssistant =
                    (currentAssistantId ? this.getAssistantById(currentAssistantId) : undefined) ||
                    this.getAssistantById(AssistantID.UOS_AI) ||
                    this.assistantList[0] ||
                    null;

                this.setCurrentAssistant(nextCurrentAssistant);
                console.info("Loaded assistant list:", this.assistantList);
                this.hasLoadedAssistantList = true;

                if (hasMismatch || exposedAssistantIds.length > 0) {
                    // 顺序被归一化或新增助手进入常驻区后，立即回写后端，避免下次启动读到旧配置。
                    if (hasMismatch) {
                        console.warn("Assistant order does not match assistant list, normalizing persisted order");
                    }
                    await this.saveAssistantOrder(nextOrder, backend);
                }

                if (nextVisibleCount !== previousVisibleCount) {
                    await this.saveAssistantVisibleCount(nextVisibleCount, backend);
                }
            } catch (error) {
                console.error("Failed to load assistant list:", error);
                this.setAssistantList([]);
                this.setCurrentAssistant(null);
                this.hasLoadedAssistantList = false;
            } finally {
                this.setLoading(false);
            }
        },

        // 重新排序助手列表
        async reorderAssistants(
            order: string[],
            temporarilyInsertedAssistantId?: string | null,
            visibleCount?: number,
        ) {
            const { order: normalizedCurrentOrder } = normalizeAssistantOrder(this.assistantList, this.assistantOrder);
            // 先把展示顺序转换为真实持久化顺序，再保存到后端。
            const nextOrder = buildPersistedOrderFromDisplayOrder(
                order,
                normalizedCurrentOrder,
                temporarilyInsertedAssistantId,
            );
            // 常驻数量只在跨常驻/隐藏区拖拽时传入；普通组内排序不改该配置。
            const shouldUpdateVisibleCount = typeof visibleCount === "number";
            const nextVisibleCount = shouldUpdateVisibleCount
                ? normalizeAssistantVisibleCount(visibleCount)
                : this.assistantVisibleCount;
            this.assistantOrder = nextOrder;
            this.assistantVisibleCount = nextVisibleCount;
            this.setAssistantList(sortAssistantsByOrder(this.assistantList, nextOrder));
            await this.saveAssistantOrder(nextOrder);
            if (shouldUpdateVisibleCount) {
                await this.saveAssistantVisibleCount(nextVisibleCount);
            }
        },

        // 更新助手区常驻显示数量。只在拖拽完成后调用，避免预览态落库。
        async updateAssistantVisibleCount(visibleCount: number) {
            const nextVisibleCount = normalizeAssistantVisibleCount(visibleCount);
            this.assistantVisibleCount = nextVisibleCount;
            await this.saveAssistantVisibleCount(nextVisibleCount);
        },

        // 加载排序
        async loadAssistantOrder(backend: any = useBackendStore()) {
            try {
                const result = await backend.requestAssistant("getAssistantOrder");
                this.assistantOrder = normalizeAssistantOrderValue(result);
            } catch (error) {
                console.error("Failed to load assistant order:", error);
                this.assistantOrder = [];
            }
        },

        // 加载助手区常驻显示数量
        async loadAssistantVisibleCount(backend: any = useBackendStore()) {
            try {
                const result = await backend.requestAssistant("getAssistantVisibleCount");
                // 后端返回值按宽松类型处理，避免旧配置或空值影响侧边栏渲染。
                this.assistantVisibleCount = normalizeAssistantVisibleCount(Number(result));
            } catch (error) {
                console.error("Failed to load assistant visible count:", error);
                this.assistantVisibleCount = DEFAULT_ASSISTANT_VISIBLE_COUNT;
            }
        },

        // 保存排序
        async saveAssistantOrder(order: string[] = this.assistantOrder, backend: any = useBackendStore()) {
            const normalizedOrder = normalizeAssistantOrderValue(order);
            this.assistantOrder = normalizedOrder;

            try {
                await backend.requestAssistant("setAssistantOrder", normalizedOrder);
            } catch (error) {
                console.error("Failed to save assistant order:", error);
            }
        },

        // 保存助手区常驻显示数量
        async saveAssistantVisibleCount(
            visibleCount: number = this.assistantVisibleCount,
            backend: any = useBackendStore(),
        ) {
            const normalizedVisibleCount = normalizeAssistantVisibleCount(visibleCount);
            this.assistantVisibleCount = normalizedVisibleCount;

            try {
                await backend.requestAssistant("setAssistantVisibleCount", normalizedVisibleCount);
            } catch (error) {
                console.error("Failed to save assistant visible count:", error);
            }
        },

        // 检查环境是否支持运行
        async checkEnvironment() {
            // 先关闭之前的 toast 弹窗
            useNotifyStore().closeAllToasts();

            const assistant = this.currentAssistant as Assistant;
            let isShowToast = false;
            let message = "";
            let app = "";
            const backend = useBackendStore();

            if (assistant.id === AssistantID.UOS_AI_KNOWLEDGE_BASE) {
                const isKnowledgeBaseReady = await backend.requestServiceConfig("checkEmbeddingPlugins");
                if (!isKnowledgeBaseReady) {
                    assistant.envExists = false;
                    isShowToast = true;
                    message = backend.translate(
                        "To use AI Knowledge Base, install Embedding Plugins from App Store first.",
                    );
                    app = DependencyPackage.RAG;
                }
            }

            if (assistant.id === AssistantID.UOS_AI_MCP_AND_SKILL) {
                const isMcpRuntimeReady = await backend.requestServiceConfig("isMcpRuntimeReady");

                if (!isMcpRuntimeReady) {
                    assistant.envExists = false;
                    isShowToast = true;
                    message = backend.translate("To use MCP&Skills, install UOS AI Agent from the App Store first.");
                    app = DependencyPackage.AGENT;
                } else {
                    assistant.envExists = true;
                    await useMcpServicesStore()
                        .loadThirdPartyAgreement(true)
                        .catch(() => undefined);
                }
            }

            if (!isShowToast) {
                useNotifyStore().closeAllToasts();
                return;
            }

            const { id, promise } = useNotifyStore().showToast({
                type: "warning",
                message: message,
                duration: 0,
                actions: [{ key: "installNow", text: backend.translate("Install Now") }],
                showClose: true,
            });
            const result = await promise;
            if (result.key === "installNow") {
                backend.requestServiceConfig("installApp", app);
            }
        },

        // 依赖环境变化信号connect
        initializeEnvChannel(serviceConfigChannel: any) {
            if (!serviceConfigChannel) {
                console.error("ServiceConfig channel is not available");
                return;
            }

            // 监听 embeddingPluginsChanged 信号
            if (serviceConfigChannel.embeddingPluginsChanged) {
                console.log("embeddingPluginsChanged signal connected");
                serviceConfigChannel.embeddingPluginsChanged.connect(async (envExists: boolean) => {
                    console.log("embeddingPluginsChanged signal received:", envExists);
                    // 更新助手环境状态
                    this.assistantList.forEach((assistant) => {
                        if (assistant.id === AssistantID.UOS_AI_KNOWLEDGE_BASE) {
                            assistant.envExists = envExists;
                            if (this.currentAssistant?.id === assistant.id) {
                                this.setCurrentAssistant(assistant);
                            }
                        }
                    });
                });
            }

            // 监听 mcpPluginChanged 信号
            if (serviceConfigChannel.mcpPluginChanged) {
                console.log("mcpPluginChanged signal connected");
                serviceConfigChannel.mcpPluginChanged.connect(async (envExists: boolean) => {
                    console.log("mcpPluginChanged signal received:", envExists);
                    // 更新助手环境状态
                    this.assistantList.forEach((assistant) => {
                        if (assistant.id === AssistantID.UOS_AI_MCP_AND_SKILL) {
                            assistant.envExists = envExists;
                            if (this.currentAssistant?.id === assistant.id) {
                                this.setCurrentAssistant(assistant);
                            }
                        }
                    });
                });
            }
        },
    },
});
