import { defineStore } from "pinia";
import type { BatchOperateState, AssistantFilterTag } from "@/types/historyconversation";
import { useConversationManagerStore, useBackendStore } from "@/stores";

export const useHistoryConversationStore = defineStore("historyconversation", {
    state: () => ({
        // historyConversations 不再存储在 state 中，而是通过 getter 直接引用 conversationManagerStore 中的数据
        batchOperateState: {
            isBatchMode: false,
            selectedIds: [],
        } as BatchOperateState, // 批量管理状态
        filterCondition: {
            searchKeyword: "", // 搜索关键词
            selectedAssistants: [] as string[], // 选中的智能体ID列表
        }, // 筛选条件
        assistantFilterTags: [] as AssistantFilterTag[], // 智能体筛选标签列表
        openedMenuId: null as string | null, // 当前打开的菜单 ID，格式为 "source:conversationId"，如 "sidebar:xxx" 或 "history:xxx"
    }),

    getters: {
        // 获取历史会话列表（直接引用 conversationManagerStore）
        getHistoryConversations: () => {
            return useConversationManagerStore().getConversationIndexList;
        },
        // 获取筛选后的历史会话列表
        getFilteredHistoryConversations: (state) => {
            let filtered = useConversationManagerStore().getConversationIndexList;

            // 按搜索关键词筛选
            if (state.filterCondition.searchKeyword) {
                const keyword = state.filterCondition.searchKeyword.toLowerCase();
                filtered = filtered.filter(
                    (conv) =>
                        conv.title.toLowerCase().includes(keyword) || conv.introduction.toLowerCase().includes(keyword),
                );
            }

            // 按智能体筛选
            if (state.filterCondition.selectedAssistants.length > 0) {
                filtered = filtered.filter((conv) => state.filterCondition.selectedAssistants.includes(conv.assistant));
            }

            return filtered;
        },
        // 获取批量管理状态
        getBatchOperateState: (state) => state.batchOperateState,
        // 获取选中的会话数量
        getSelectedCount: (state) => state.batchOperateState.selectedIds.length,
        // 检查是否全选
        isAllSelected: (state) => {
            const filtered = useConversationManagerStore().getConversationIndexList.filter(
                (conv) =>
                    state.filterCondition.selectedAssistants.length === 0 ||
                    state.filterCondition.selectedAssistants.includes(conv.assistant),
            );
            return filtered.length > 0 && state.batchOperateState.selectedIds.length === filtered.length;
        },
    },

    actions: {
        // 获取历史会话数据
        async fetchHistoryConversations() {
            const conversationManagerStore = useConversationManagerStore();
            await conversationManagerStore.loadConversationIndexList(useBackendStore()); // 获取会话索引列表

            // 从会话列表中提取智能体信息，去重后生成筛选标签
            const conversationList = useConversationManagerStore().getConversationIndexList;
            const assistantMap = new Map<string, string>(); // 使用 Map 去重，key 为 assistant id，value 为 assistant_name

            conversationList.forEach((conv) => {
                if (conv.assistant && conv.assistant_name && !assistantMap.has(conv.assistant)) {
                    assistantMap.set(conv.assistant, conv.assistant_name);
                }
            });

            // 初始化智能体筛选标签
            this.assistantFilterTags = Array.from(assistantMap.entries()).map(([id, name]) => ({
                id,
                name,
                isSelected: false,
            }));

            // 添加一个“全部”标签
            this.assistantFilterTags.unshift({
                id: "all",
                name: useBackendStore().translate("All"),
                isSelected: false,
            });

            // 重置智能体筛选标签状态
            this.toggleSelectAllAssistants();
        },

        // 更新批量管理状态
        updateBatchOperateState(isBatchMode: boolean) {
            this.batchOperateState.isBatchMode = isBatchMode;
            if (!isBatchMode) {
                // 关闭批量模式时清空选中列表
                this.batchOperateState.selectedIds = [];
            }
        },

        // 切换会话选中状态
        toggleConversationSelection(conversationId: string) {
            const index = this.batchOperateState.selectedIds.indexOf(conversationId);
            if (index > -1) {
                this.batchOperateState.selectedIds.splice(index, 1);
            } else {
                this.batchOperateState.selectedIds.push(conversationId);
            }
        },

        // 全选/取消全选
        toggleSelectAll(selectAll: boolean) {
            if (selectAll) {
                // 选中所有筛选后的会话
                const filtered = useConversationManagerStore().getConversationIndexList.filter(
                    (conv) =>
                        this.filterCondition.selectedAssistants.length === 0 ||
                        this.filterCondition.selectedAssistants.includes(conv.assistant),
                );
                this.batchOperateState.selectedIds = filtered.map((conv) => conv.id);
            } else {
                this.batchOperateState.selectedIds = [];
            }
        },

        // 删除会话（单个或批量）
        async deleteConversations(conversationIds: string | string[]) {
            if (conversationIds) {
                if (typeof conversationIds === "string") {
                    conversationIds = [conversationIds];
                }
            }

            await useBackendStore().requestConversation("deleteConversation", conversationIds);

            // 保存当前筛选的智能体（排除"all"）
            const previousSelectedAssistants = this.filterCondition.selectedAssistants.filter(
                (id) => id !== "all",
            );

            // 重新获取历史会话数据
            await this.fetchHistoryConversations();

            // 如果之前有特定的智能体筛选，检查是否还有该智能体的会话
            if (previousSelectedAssistants.length > 0) {
                // 过滤出仍然存在的智能体ID
                const remainingAssistantIds = new Set(
                    useConversationManagerStore().getConversationIndexList.map((conv) => conv.assistant),
                );

                // 检查之前筛选的智能体是否还有会话
                const hasRemainingConversations = previousSelectedAssistants.some((id) =>
                    remainingAssistantIds.has(id),
                );

                // 如果没有会话了，重置为全选状态
                if (!hasRemainingConversations) {
                    this.toggleSelectAllAssistants();
                } else {
                    // 否则，重新应用之前的筛选条件（更新标签选中状态）
                    this.assistantFilterTags.forEach((tag) => {
                        tag.isSelected = previousSelectedAssistants.includes(tag.id);
                    });
                    this.filterCondition.selectedAssistants = previousSelectedAssistants;
                }
            }

            return;

            // const idsToDelete = Array.isArray(conversationIds) ? conversationIds : [conversationIds];

            // // 从历史会话列表中删除
            // this.historyConversations = this.historyConversations.filter((conv) => !idsToDelete.includes(conv.id));

            // // 清空选中列表
            // this.batchOperateState.selectedIds = this.batchOperateState.selectedIds.filter(
            //     (id) => !idsToDelete.includes(id),
            // );
        },

        // 更新筛选条件
        updateFilterCondition(condition: { searchKeyword?: string; selectedAssistants?: string[] }) {
            if (condition.searchKeyword !== undefined) {
                this.filterCondition.searchKeyword = condition.searchKeyword;
            }
            if (condition.selectedAssistants !== undefined) {
                this.filterCondition.selectedAssistants = condition.selectedAssistants;
            }
        },

        // 切换智能体标签选中状态（单选）
        toggleAssistantTag(assistantId: string) {
            const tag = this.assistantFilterTags.find((t) => t.id === assistantId);
            if (tag) {
                if (assistantId === "all") {
                    // 点击 "全部" 标签时，根据当前筛选条件判断是否选中
                    this.assistantFilterTags.forEach((t) => {
                        t.isSelected = true;
                    });
                    this.filterCondition.selectedAssistants = this.assistantFilterTags.map((t) => t.id); // 选中所有标签
                } else {
                    // 单选：先取消所有标签的选中状态，再选中当前标签
                    this.assistantFilterTags.forEach((t) => {
                        t.isSelected = false;
                    });
                    tag.isSelected = true;
                    this.filterCondition.selectedAssistants = [assistantId];
                }
            }
        },

        // 重置智能体标签选中状态
        resetAssistantTags() {
            this.assistantFilterTags.forEach((tag) => {
                tag.isSelected = false;
            });
            this.filterCondition.selectedAssistants = [];
        },

        // 全选智能体标签
        toggleSelectAllAssistants() {
            // 选中所有智能体标签
            this.assistantFilterTags.forEach((tag) => (tag.isSelected = true));
            this.filterCondition.selectedAssistants = this.assistantFilterTags.map((tag) => tag.id);
        },

        // 设置当前打开的菜单 ID
        setOpenedMenuId(menuId: string | null) {
            this.openedMenuId = menuId;
        },

        resetPageState() {
            this.updateBatchOperateState(false);
            this.updateFilterCondition({
                searchKeyword: "",
                selectedAssistants: [],
            });
            this.assistantFilterTags = [];
        },
    },
});
