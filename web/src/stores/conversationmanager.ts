import { defineStore } from "pinia";

import type {
    ConversationIndex,
    Message,
    Conversation,
    OutlineData,
    Root,
    ConversationIndexWithStatus,
    BashApproveData,
    FileChangeApproveData,
} from "@/types/conversation";
import { ConversationScene, ConversationStatus, InteractiveCompStatus } from "@/types/conversation";
import type { Assistant } from "@/types/assistant";
import { ConversationRecord, useBackendStore, useAssistantInfosStore, useExtensionPanelStore } from "@/stores";
import { useNotifyStore } from "@/stores/notify";
import { WindowMode } from "@/types/windowinfo";

// ============================================================
// 系统通知 Action 常量定义
// ============================================================

/** Action ID 格式: {scene}:{operation}:{params...} */
const ACTION_SEPARATOR = ":";

/** Scene 枚举 - 通知场景 */
const NotificationScene = {
    AI_REPLY: "ai_reply",
    ICOMP: "icomp",
} as const;

/** Operation 枚举 - 操作类型 */
const NotificationOperation = {
    VIEW_NOW: "view_now",
    REMIND_LATER: "remind_later",
    BASH_APPROVED: "bash_approved",
    BASH_ALWAYS: "bash_always",
    BASH_REJECTED: "bash_rejected",
    FILE_APPLY: "file_apply",
    FILE_REJECTED: "file_rejected",
} as const;

/** IComp 类型映射 */
const ICOMP_TYPE_MAP: Record<string, string> = {
    [NotificationOperation.BASH_APPROVED]: "bash_approve",
    [NotificationOperation.BASH_ALWAYS]: "bash_approve",
    [NotificationOperation.BASH_REJECTED]: "bash_approve",
    [NotificationOperation.FILE_APPLY]: "file_change_approve",
    [NotificationOperation.FILE_REJECTED]: "file_change_approve",
};

/** 审批操作配置 */
const APPROVAL_CONFIG: Record<string, { approved: boolean; alwaysApprove?: boolean }> = {
    [NotificationOperation.BASH_APPROVED]: { approved: true },
    [NotificationOperation.BASH_ALWAYS]: { approved: true, alwaysApprove: true },
    [NotificationOperation.BASH_REJECTED]: { approved: false },
    [NotificationOperation.FILE_APPLY]: { approved: true },
    [NotificationOperation.FILE_REJECTED]: { approved: false },
};

/**
 * 解析 Action ID 第一部分
 * @returns [part, rest] 或 null 如果格式无效
 */
const parseActionPart = (actionId: string): [string, string] | null => {
    const idx = actionId.indexOf(ACTION_SEPARATOR);
    if (idx === -1) return null;
    return [actionId.substring(0, idx), actionId.substring(idx + 1)];
};

// 打印组件
// import cloneDeep from "lodash/cloneDeep";

// 最大并发会话数
export const MAX_CONCURRENT_CHATS = 10;

// ConversationRecord 内部状态变化后，需要替换 Map 引用，才能让依赖该 Map 的计算属性重新求值。
const handleConversationRecordStateChange = () => {
    const conversationManagerStore = useConversationManagerStore();
    conversationManagerStore.conversionList = new Map(conversationManagerStore.conversionList);
};

// ConversationRecord 新增 IComp 时触发系统通知（仅窗口未获焦点时）。
const handleICompAdded = async (
    data: BashApproveData | FileChangeApproveData,
    messageId: string,
    conversationRecord: ConversationRecord,
) => {
    const conversationId = conversationRecord.root?.id;
    if (!conversationId) return;
    const manager = useConversationManagerStore();
    const isActive = await manager.isMainWindowActive();
    if (isActive) return;
    manager.showICompNotification(conversationId, messageId, data);
};

export const useConversationManagerStore = defineStore("conversationmanager", {
    state: () => ({
        conversationIndexList: [] as ConversationIndex[], // 会话索引列表
        historyConversationIndexList: [] as ConversationIndex[], // 历史会话索引列表（搜索）
        conversionList: new Map<string, ConversationRecord>(), // 所有正在回答中的会话，及当前会话
        answeringSession: new Map<string, string>(), // 当前sessionId对应的conversationId, key: sessionId, value: conversationId
        currentConversationId: "", // 当前聊天窗口展示的会话id
        aiReplyNotifyTimer: null as number | null, // 回复完成通知防抖 timer（500ms）
        aiReplyLatestConversationId: "", // 本轮防抖窗口中的最新会话
        systemNotificationListenerInitialized: false, // 系统通知 action 监听是否已注册
    }),

    getters: {
        getConversationById: (state) => (conversationId: string) => state.conversionList.get(conversationId), // 根据id获取会话
        getConversationIndexList: (state) => state.conversationIndexList, // 获取会话索引列表
        getHistoryConversationIndexList: (state) => state.historyConversationIndexList, // 获取历史会话索引列表（搜索）
        getCurrentConversationId: (state) => state.currentConversationId, // 获取当前聊天窗口展示的会话id，新建会话或点击侧边栏或在历史对话管理界面点击会话时，更新当前会话id，当前展示的只会有一个会话
        // 对外暴露当前会话 record / scene，供展示层和行为层复用统一抽象。
        getCurrentConversationRecord: (state) => state.conversionList.get(state.currentConversationId),
        getCurrentConversationScene: (state) =>
            state.conversionList.get(state.currentConversationId)?.scene ?? ConversationScene.Default,
        getMessageIdByConversationId: (state) => (conversationId: string) =>
            state.conversionList.get(conversationId)?.messageId, // 根据会话id获取消息id
        getCurrentMessagesRender: (state) => state.conversionList.get(state.currentConversationId)?.conversation, // 获取当前聊天窗口展示的会话的消息
        getSessionIdByConversationId: (state) => state.conversionList.get(state.currentConversationId)?.sessionId, // 根据会话id获取sessionId, key: conversationId, value: sessionId
        isCurrentSessionRunning: (state) =>
            state.answeringSession.has(state.conversionList.get(state.currentConversationId)?.sessionId as string), // 判断当前会话是否正在回答中(发送问题开始算)
        isSessionStarted: (state) => state.conversionList.get(state.currentConversationId)?.isSessionStart, // 判断当前会话是否正在回答中(收到后端的start开始算)
        getIsFromHistory: (state) => state.conversionList.get(state.currentConversationId)?.isFromHistory, // 判断当前会话是否是从历史记录中恢复的
        isConversationGenerating:
            (state) =>
            (conversationId: string): boolean => {
                const sessionId = state.conversionList.get(conversationId)?.sessionId;
                if (!sessionId) {
                    return false;
                }

                return state.answeringSession.get(sessionId) === conversationId;
            },
        hasActivePendingApproval:
            (state) =>
            (conversationId: string): boolean => {
                const conversationRecord = state.conversionList.get(conversationId);
                if (!conversationRecord?.hasPendingApproval) {
                    return false;
                }

                const sessionId = conversationRecord.sessionId;
                return !!sessionId && state.answeringSession.get(sessionId) === conversationId;
            },
        // 统一从 ConversationRecord 读取会话状态，避免在 manager 中再维护一份重复状态。
        getConversationStatus:
            (state) =>
            (conversationId: string): ConversationStatus => {
                const sessionId = state.conversionList.get(conversationId)?.sessionId;
                if (sessionId && state.answeringSession.get(sessionId) === conversationId) {
                    return ConversationStatus.Generating;
                }

                const conversationRecord = state.conversionList.get(conversationId);
                return conversationRecord?.conversationStatus ?? ConversationStatus.Read;
            },
        // 将后端索引与内存中的运行态会话合并，给侧边栏提供一份可直接渲染的列表数据。
        getConversationIndexListWithStatus(): ConversationIndexWithStatus[] {
            const conversationIndexMap = new Map<string, ConversationIndexWithStatus>();

            for (const conversationIndex of this.conversationIndexList) {
                const conversationRecord = this.conversionList.get(conversationIndex.id);
                conversationIndexMap.set(conversationIndex.id, {
                    id: conversationIndex.id,
                    title: conversationIndex.title,
                    assistant: conversationIndex.assistant,
                    assistant_name: conversationIndex.assistant_name,
                    introduction: conversationIndex.introduction,
                    updated_at: conversationRecord?.activityAt ?? conversationIndex.updated_at,
                    conversationStatus: this.getConversationStatus(conversationIndex.id),
                    hasPendingApproval: this.hasActivePendingApproval(conversationIndex.id),
                });
            }

            for (const [conversationId, conversationRecord] of this.conversionList.entries()) {
                if (!conversationRecord.shouldShowInSidebar || Object.keys(conversationRecord.messages).length === 0) {
                    continue;
                }

                if (conversationIndexMap.has(conversationId)) {
                    continue;
                }

                // 还未落盘的会话只有在“当前会话”、存在未读/生成中状态或活跃待审批时才需要出现在侧边栏。
                const shouldShowFallbackConversation =
                    this.currentConversationId === conversationId ||
                    this.getConversationStatus(conversationId) !== ConversationStatus.Read ||
                    this.hasActivePendingApproval(conversationId);

                if (!shouldShowFallbackConversation) {
                    continue;
                }

                conversationIndexMap.set(conversationId, this.createConversationIndexWithStatus(conversationId));
            }

            return Array.from(conversationIndexMap.values()).sort((a, b) => b.updated_at - a.updated_at);
        },
    },

    actions: {
        // 设置会话索引列表
        setConversationIndexList(conversationIndexList: ConversationIndex[]) {
            this.conversationIndexList = conversationIndexList;
        },

        // 设置历史会话索引列表
        setHistoryConversationIndexList(historyConversationIndexList: ConversationIndex[]) {
            this.historyConversationIndexList = historyConversationIndexList;
        },

        // 加载会话索引列表
        async loadConversationIndexList(backend: any) {
            try {
                const result = await backend.requestConversation("getConversationIndexes");
                console.info("Loaded conversation indexes:", result);
                this.setConversationIndexList(result ? JSON.parse(result as string) : []);
                console.info("Loaded conversation index list:", this.conversationIndexList);
            } catch (error) {
                console.error("Failed to load conversation index list:", error);
                this.setConversationIndexList([]);
            }
        },

        // 加载历史会话索引列表（搜索）
        async loadHistoryConversationIndexList(backend: any) {
            try {
                const result = await backend.requestConversation("getHistoryConversationIndexes");
                console.info("Loaded history conversation indexes:", result);
                this.setHistoryConversationIndexList(result ? JSON.parse(result as string) : []);
                console.info("Loaded history conversation index list:", this.historyConversationIndexList);
            } catch (error) {
                console.error("Failed to load history conversation index list:", error);
                this.setHistoryConversationIndexList([]);
            }
        },

        /**
         * 创建会话
         * @param conversationId 会话id
         * @param assistant 助手id
         * @param model 模型id
         * 点击侧边栏创建会话或点击任意智能体时，创建会话
         */
        async createConversation(
            conversationId: string,
            assistant: string,
            model: string,
            scene: ConversationScene = ConversationScene.Default,
        ) {
            // scene 决定该会话的运行规则和展示规则，创建阶段就写入 record，
            // 避免先以默认场景初始化再二次切换。
            // 新建会话前先剔除从未发送过消息的空白会话，避免侧边栏里出现多个无内容占位项。
            const persistedConversationEntries = [...this.conversionList.entries()].filter(
                ([, record]) => record.conversation && Object.keys(record.messages).length > 0, // 过滤空会话
            );
            const removedConversationRecords = [...this.conversionList.entries()]
                .filter(([, record]) => !record.conversation || Object.keys(record.messages).length === 0)
                .map(([, record]) => record);
            removedConversationRecords.forEach((record) => this.detachConversationRecord(record));

            const conversationRecord = new ConversationRecord(conversationId, {
                id: conversationId,
                assistant: assistant,
                cur_next: "",
                model: model,
                next: [],
            } as Root);
            conversationRecord.scene = scene;
            this.attachConversationRecord(conversationRecord);

            // 使用展开运算符过滤空会话并添加新会话
            this.conversionList = new Map([
                ...persistedConversationEntries,
                [conversationId, conversationRecord], // 添加新会话
            ]);
            this.currentConversationId = conversationId; // 更新当前会话id，新建会话时，展示新会话
            // 新建会话只是进入编辑态，尚未产生新回复，因此默认视为已读。
            conversationRecord.markAsRead();

            // 清理过期会话
            this.cleanExpiredRecords();
        },

        // 创建临时会话
        createTempConversation(conversationId: string, assistant: string, model: string) {
            this.createConversation(conversationId, assistant, model, ConversationScene.Temporary);
        },

        // 删除会话
        removeConversation(conversationId: string) {
            const conversationRecord = this.conversionList.get(conversationId);
            if (conversationRecord) {
                this.detachConversationRecord(conversationRecord);
            }
            this.conversionList.delete(conversationId);
        },

        // 新增会话消息
        async addConversationMessage(conversationId: string, messageId: string, message: Message, sessionId: string) {
            // 这是浅拷贝，会话记录会被修改
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (conversationRecord) {
                conversationRecord.setMessage(messageId, message);
                conversationRecord.sessionId = sessionId; // 设置通话id
                conversationRecord.initAiMessage(); // 初始化AI 消息
                this.answeringSession.set(sessionId, conversationId); // 设置当前sessionId对应的conversationId, 正在回答中的会话
                // 用户发送问题后，侧边栏立即进入“生成中”，这样无需等待首个流式分片也能反馈状态。
                conversationRecord.markAsGenerating();
            }
        },

        // 切换会话
        async switchConversation(conversationId: string) {
            console.info("Switch conversation to:", conversationId);
            if (!this.conversionList.has(conversationId)) {
                // 切换的目标会话已经回答完成，从后端获取会话内容
                const result = await useBackendStore().requestConversation("getConversation", conversationId);
                // 将result中的messages根据cur_next和previous，更新state.answeringMessages[conversationId]中
                if (result) {
                    const conversation = result as Conversation;
                    const messagesMap = conversation.messages as Record<string, Message>;

                    // 嵌入消息ID，供编辑大纲时定向持久化
                    for (const messageId in messagesMap) {
                        if (messagesMap[messageId]) {
                            messagesMap[messageId].id = messageId;
                            // 历史消息 ID 已稳定，直接作为前端 key
                            messagesMap[messageId].frontendKey = messageId;
                        }
                    }

                    // 新增会话记录
                    const conversationRecord = new ConversationRecord(conversationId, conversation.root);
                    conversationRecord.messages = messagesMap;
                    // 从root.cur_next开始遍历，找到last_message, 并设置为当前会话的messageId，以供继续对话
                    let lastMessageId = conversation.root.cur_next;
                    while (lastMessageId && conversation.messages[lastMessageId]) {
                        const message = conversation.messages[lastMessageId];
                        if (message) {
                            if (message.cur_next === "") {
                                // 找到last_message，跳出循环
                                break;
                            }
                            lastMessageId = message.cur_next;
                        } else {
                            break;
                        }
                    }
                    conversationRecord.messageId = lastMessageId;
                    // 切换会话时，默认从历史记录中恢复
                    conversationRecord.isFromHistory = true; // 设置为从历史记录中恢复
                    this.attachConversationRecord(conversationRecord);
                    this.conversionList.set(conversationId, conversationRecord);
                }
            }

            this.currentConversationId = conversationId; // 设置当前会话id，切换会话时，展示新会话
            const conversationRecord = this.conversionList.get(conversationId);
            if (!conversationRecord) {
                return;
            }
            // 用户主动切入该会话时，非生成中的未读提示应立即消除；
            // 但如果会话仍在回答中，需要继续维持 loading 展示。
            if (!this.isConversationGenerating(conversationId)) {
                conversationRecord.markAsRead();
            }
            // 切换会话时隐藏卡片
            conversationRecord.showCards = false;
            // 切换为当前会话助手 conversation.root.assistant
            useAssistantInfosStore().setCurrentAssistant(
                useAssistantInfosStore().getAssistantById(conversationRecord?.root?.assistant as string) as Assistant,
            );
        },

        // 重试
        async handleRetry(conversationId: string, messageId: string, sessionId: string) {
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (conversationRecord) {
                conversationRecord.messageId = messageId;
                conversationRecord.sessionId = sessionId; // 设置通话id
                conversationRecord.initAiMessage(); // 初始化AI 消息
                this.answeringSession.set(sessionId, conversationId); // 设置当前sessionId对应的conversationId, 正在回答中的会话
                this.currentConversationId = conversationId; // 设置当前会话id
                // 重试本质上也是重新生成回复，状态流转与普通发送保持一致。
                conversationRecord.markAsGenerating();
                // 重试时也需要显示卡片
                conversationRecord.showCards = true;
            }
        },

        initializeAiReplyNotificationHandlers() {
            if (this.systemNotificationListenerInitialized) {
                return;
            }
            const notifyStore = useNotifyStore();
            notifyStore.onSystemNotificationAction(({ actionId }) => {
                void this.handleSystemNotificationAction(actionId);
            });
            this.systemNotificationListenerInitialized = true;
        },

        async isMainWindowActive() {
            try {
                const active = await useBackendStore().requestWindow("isMainWindowActive");
                return Boolean(active);
            } catch (error) {
                console.warn("Failed to query main window active status:", error);
                return true;
            }
        },

        getUnreadConversationCount() {
            return this.getConversationIndexListWithStatus.filter(
                (conversation) => conversation.conversationStatus === ConversationStatus.Unread,
            ).length;
        },

        scheduleAiReplyCompletedNotification(conversationId: string, messageId: string) {
            this.initializeAiReplyNotificationHandlers();
            this.aiReplyLatestConversationId = conversationId;

            if (this.aiReplyNotifyTimer !== null) {
                clearTimeout(this.aiReplyNotifyTimer);
            }

            this.aiReplyNotifyTimer = window.setTimeout(() => {
                this.aiReplyNotifyTimer = null;
                void this.flushAiReplyCompletedNotification();
            }, 500);
        },

        async flushAiReplyCompletedNotification() {
            const unreadCount = this.getUnreadConversationCount();
            if (unreadCount <= 0) {
                return;
            }

            const backend = useBackendStore();
            const notifyStore = useNotifyStore();
            const countText = String(unreadCount);
            const body = backend.translate("You have %1 newly answered chats").replace("%1", countText);

            const conversationId = this.aiReplyLatestConversationId;
            await notifyStore.showSystemNotification({
                body,
                actions: [
                    {
                        key: `${NotificationScene.AI_REPLY}:${NotificationOperation.REMIND_LATER}`,
                        text: backend.translate("Remind Me Later"),
                    },
                    {
                        key: `${NotificationScene.AI_REPLY}:${NotificationOperation.VIEW_NOW}:${conversationId}`,
                        text: backend.translate("View Now"),
                    },
                ],
                timeoutMs: 5000,
            });
        },

        /**
         * 切换到指定会话（公共辅助方法）
         */
        async switchToConversation(conversationId: string) {
            await useBackendStore().requestWindow("switchMode", WindowMode.Main);
            useExtensionPanelStore().closeExtensionPanel();
            await this.switchConversation(conversationId);
        },

        async handleSystemNotificationAction(actionId: string) {
            if (!actionId) return;

            const parsed = parseActionPart(actionId);
            if (!parsed) return;
            const [scene, rest] = parsed;

            switch (scene) {
                case NotificationScene.AI_REPLY:
                    await this.handleAiReplyAction(rest);
                    break;
                case NotificationScene.ICOMP:
                    await this.handleICompAction(rest);
                    break;
            }
        },

        async handleAiReplyAction(rest: string) {
            const parsed = parseActionPart(rest);
            if (!parsed) return;
            const [operation, params] = parsed;

            if (operation !== NotificationOperation.VIEW_NOW) return;

            const conversationId = params;
            if (!conversationId) return;

            await this.switchToConversation(conversationId);
        },

        // ============================================================
        // IComp 系统通知
        initializeICompNotificationHandlers() {
            this.initializeAiReplyNotificationHandlers();
        },

        async showICompNotification(
            conversationId: string,
            messageId: string,
            data: BashApproveData | FileChangeApproveData,
        ) {
            this.initializeICompNotificationHandlers();
            const notifyStore = useNotifyStore();
            const backend = useBackendStore();
            const requestId = data.id;

            const actions: Array<{ key: string; text: string }> = [];

            if (data.ic_type === "bash_approve") {
                actions.push(
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.VIEW_NOW}:${conversationId}`,
                        text: backend.translate("View Now"),
                    },
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.BASH_APPROVED}:${requestId}:${conversationId}:${messageId}`,
                        text: backend.translate("Allow Once"),
                    },
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.BASH_ALWAYS}:${requestId}:${conversationId}:${messageId}`,
                        text: backend.translate("Allow Chat"),
                    },
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.BASH_REJECTED}:${requestId}:${conversationId}:${messageId}`,
                        text: backend.translate("Reject"),
                    },
                );
            } else if (data.ic_type === "file_change_approve") {
                actions.push(
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.VIEW_NOW}:${conversationId}`,
                        text: backend.translate("View Now"),
                    },
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.FILE_APPLY}:${requestId}:${conversationId}:${messageId}`,
                        text: backend.translate("Apply"),
                    },
                    {
                        key: `${NotificationScene.ICOMP}:${NotificationOperation.FILE_REJECTED}:${requestId}:${conversationId}:${messageId}`,
                        text: backend.translate("Reject"),
                    },
                );
            }

            let body = data.title;
            if (data.ic_type === "file_change_approve") {
                const changes = (data as FileChangeApproveData).changes;
                const created = changes.filter((c) => c.kind.trim().toLowerCase() === "created").length;
                const modified = changes.filter((c) => c.kind.trim().toLowerCase() === "modified").length;
                const deleted = changes.filter((c) => c.kind.trim().toLowerCase() === "deleted").length;
                body = backend
                    .translate("%1 file changes (%2 added, %3 modified, %4 deleted)")
                    .replace("%1", String(changes.length))
                    .replace("%2", String(created))
                    .replace("%3", String(modified))
                    .replace("%4", String(deleted));
            }

            await notifyStore.showSystemNotification({
                title: backend.translate("Awaiting Approval"),
                body,
                actions,
                timeoutMs: 5000,
            });
        },

        async handleICompAction(rest: string) {
            const parsed = parseActionPart(rest);
            if (!parsed) return;
            const [operation, params] = parsed;

            // view_now:conversationId
            if (operation === NotificationOperation.VIEW_NOW) {
                const conversationId = params;
                if (!conversationId) return;
                await this.switchToConversation(conversationId);
                return;
            }

            // 审批类操作: requestId:conversationId:messageId
            const parts = params.split(ACTION_SEPARATOR);
            if (parts.length < 3) return;
            const [requestId, conversationId, messageId] = parts;

            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord | undefined;
            if (!conversationRecord) return;

            // 查找操作配置
            const config = APPROVAL_CONFIG[operation];
            if (!config) return;

            const icType = ICOMP_TYPE_MAP[operation];
            if (!icType) return;

            const status = config.approved ? InteractiveCompStatus.APPROVED : InteractiveCompStatus.REJECTED;
            conversationRecord.updateICompStatus(messageId, requestId, status);

            const sessionId = conversationRecord.sessionId;
            if (sessionId) {
                const backend = useBackendStore();
                backend.requestSession(
                    "invokeAction",
                    sessionId,
                    JSON.stringify({
                        request_id: requestId,
                        type: icType,
                        approved: config.approved,
                        always_approve: config.alwaysApprove ?? false,
                        reject_msg: "",
                    }),
                );
            }
        },

        // 清理过期会话
        cleanExpiredRecords() {
            // 收集需要删除的会话ID并清理
            const conversationIds = [...this.conversionList.values()]
                .filter(
                    (record) =>
                        record.root?.id !== this.currentConversationId &&
                        record.conversationStatus === ConversationStatus.Read,
                )
                .map((record) => record.root?.id as string);

            // 重新构建 Map，排除待删除的会话
            if (conversationIds.length > 0) {
                const deleteIdSet = new Set(conversationIds);
                this.conversionList = new Map([...this.conversionList].filter(([id]) => !deleteIdSet.has(id)));

                // 清理后端过期会话记录
                useBackendStore().requestConversation("releaseConversation", conversationIds);

                console.log(
                    "[ConversationManager cleanExpiredRecords] After cleaning expired records:",
                    conversationIds,
                );
            }
        },

        // ============================================================
        /**
         * 处理session消息
         */
        // 处理会话开始事件
        handleSessionStarted(sessionId: string) {
            const [isValid, conversationRecord] = this.validateSessionId(sessionId); // 校验sessionId是否有效
            if (!isValid || !conversationRecord) {
                return; // 校验失败，直接返回
            }
            // 前面所有的校验都通过，才处理会话开始事件
            conversationRecord.handleSessionStarted(sessionId);
        },

        // 处理会话消息事件
        handleSessionMessage(sessionId: string, message: string) {
            const [isValid, conversationRecord] = this.validateSessionId(sessionId); // 校验sessionId是否有效
            if (!isValid || !conversationRecord) {
                return; // 校验失败，直接返回
            }

            // 前面所有的校验都通过，才处理会话消息事件
            conversationRecord.handleSessionMessage(sessionId, message);
        },

        // 处理会话完成事件
        async handleSessionFinished(sessionId: string, message: string) {
            const [isValid, conversationRecord] = this.validateSessionId(sessionId); // 校验sessionId是否有效
            if (!isValid || !conversationRecord) {
                return; // 校验失败，直接返回
            }
            const conversationId = this.answeringSession.get(sessionId);
            await conversationRecord.handleSessionFinished(sessionId, message);
            if (conversationId) {
                const isCurrentConversation = this.currentConversationId === conversationId;
                // 当前正在查看的会话一旦完成回复，应立即转为已读。
                if (isCurrentConversation) {
                    conversationRecord.markAsRead(true);
                } else {
                    conversationRecord.markAsUnread();
                    this.scheduleAiReplyCompletedNotification(conversationId, conversationRecord.messageId);
                }
            }
            await this.handleEndSession(sessionId);
        },

        // 处理会话错误事件
        async handleSessionError(sessionId: string, message: string) {
            const [isValid, conversationRecord] = this.validateSessionId(sessionId); // 校验sessionId是否有效
            if (!isValid || !conversationRecord) {
                return; // 校验失败，直接返回
            }
            const conversationId = this.answeringSession.get(sessionId);
            await conversationRecord.handleSessionError(sessionId, message);
            if (conversationId) {
                const isCurrentConversation = this.currentConversationId === conversationId;
                // 错误回复同样算一次新的会话结果，读状态处理与正常结束保持一致。
                if (isCurrentConversation) {
                    conversationRecord.markAsRead(true);
                } else {
                    conversationRecord.markAsUnread();
                }
            }
            // await this.handleEndSession(sessionId);
        },

        // 处理会话结束事件
        async handleEndSession(sessionId: string) {
            this.answeringSession.delete(sessionId); // 删除当前sessionId对应的conversationId, 结束回答中的会话
            // 加载会话索引列表，更新会话列表
            await this.loadConversationIndexList(useBackendStore());

            // TODO: 清理conversionList中的过期会话
        },

        // 校验sessionId是否有效
        validateSessionId(sessionId: string): [boolean, ConversationRecord | null] {
            // 找到当前sessionId对应的conversationId
            const conversationId = this.answeringSession.get(sessionId);
            if (!conversationId) {
                console.error("validateSessionId Failed to find conversationId from session:", sessionId);
                return [false, null];
            }

            // 找到对应的会话记录，
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (!conversationRecord) {
                console.error(
                    "validateSessionId Failed to find conversation record from conversationId:",
                    conversationId,
                );
                return [false, null];
            }

            if (conversationRecord.sessionId !== sessionId) {
                // 会话id不匹配，跳过
                console.error("validateSessionId Session id not match:", conversationRecord.sessionId, sessionId);
                return [false, null];
            }

            return [true, conversationRecord];
        },

        attachConversationRecord(conversationRecord: ConversationRecord) {
            // 重复 attach 时先解绑，避免同一个 record 叠加多个监听器。
            conversationRecord.off("conversationStateChange", handleConversationRecordStateChange);
            conversationRecord.on("conversationStateChange", handleConversationRecordStateChange);
            conversationRecord.off("iCompAdded", handleICompAdded);
            conversationRecord.on("iCompAdded", handleICompAdded);
        },

        detachConversationRecord(conversationRecord: ConversationRecord) {
            conversationRecord.off("conversationStateChange", handleConversationRecordStateChange);
            conversationRecord.off("iCompAdded", handleICompAdded);
        },

        createConversationIndexWithStatus(conversationId: string): ConversationIndexWithStatus {
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord | undefined;
            const assistantId = conversationRecord?.root?.assistant || "";
            const assistantInfo = assistantId ? useAssistantInfosStore().getAssistantById(assistantId) : null;

            return {
                id: conversationId,
                // 对尚未落盘的会话，标题取首条用户问题，保证侧边栏有可读名称。
                title: this.getSidebarConversationTitle(conversationRecord),
                updated_at: conversationRecord?.activityAt ?? 0,
                assistant: assistantId,
                assistant_name: assistantInfo?.name || "",
                introduction: "",
                conversationStatus: conversationRecord?.conversationStatus ?? ConversationStatus.Read,
                hasPendingApproval: this.hasActivePendingApproval(conversationId),
            };
        },

        getSidebarConversationTitle(conversationRecord?: ConversationRecord | null) {
            if (!conversationRecord?.root?.cur_next) {
                return "新对话";
            }

            const firstMessage = conversationRecord.getMessage(conversationRecord.root.cur_next);
            // 优先从渲染文本中取标题，避免后续富文本或附件结构影响侧边栏展示。
            const renderText = firstMessage?.render_message
                .filter((item) => item.type === "text")
                .map((item) => {
                    const content = (item.data as { content?: string })?.content;
                    return typeof content === "string" ? content : "";
                })
                .join("")
                .replace(/\s+/g, " ")
                .trim();

            if (renderText) {
                return renderText;
            }

            const messageText = firstMessage?.message
                .flatMap((item) => item.content)
                .filter((content) => content.type === "text")
                .map((content) => content.content)
                .join("")
                .replace(/\s+/g, " ")
                .trim();

            return messageText || "新对话";
        },
        // ============================================================
        /**
         * 持久化用户编辑后的文章内容（单写 workspace）
         */
        async updateDocCardContent(conversationId: string, articleId: string, newContent: string): Promise<boolean> {
            const backend = useBackendStore();
            try {
                const result = await backend.requestConversation(
                    "updateWorkspaceArticle",
                    conversationId,
                    articleId,
                    newContent,
                );
                console.log("[conversations] Saved article content:", articleId, "length:", newContent.length);
                return result as boolean;
            } catch (error) {
                console.error("[conversations] Failed to save article content:", error);
                return false;
            }
        },

        // 持久化用户编辑后的大纲数据
        updateOutlineData(conversationId: string, messageId: string, outlineData: OutlineData) {
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (!conversationRecord) {
                console.error(
                    "updateOutlineData Failed to find conversation record from conversationId:",
                    conversationId,
                );
                return false;
            }
            return conversationRecord.updateOutlineData(messageId, outlineData);
        },

        // 更新当前会话目标消息的cur_next
        async updateCurrentMessageCurrNext(conversationId: string, messageId: string, newCurrNext: string) {
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (!conversationRecord) {
                console.error(
                    "updateCurrentMessageCurrNext Failed to find conversation record from conversationId:",
                    conversationId,
                );
                return false;
            }
            return conversationRecord.updateCurrentMessageCurrNext(messageId, newCurrNext);
        },

        /**
         * 从 workspace 加载文章内容
         */
        async getWorkspaceArticle(
            conversationId: string,
            articleId: string,
            version?: number,
        ): Promise<{
            id: string;
            title: string;
            content: string;
            version: number;
            updated_at?: string; // ISO format timestamp string (current version only)
            created_at?: string; // ISO format timestamp string (requested version)
            references: Array<{
                index: number;
                title: string;
                url: string;
                website: string;
                icon: string;
                snippet: string;
            }>;
        } | null> {
            const backend = useBackendStore();
            try {
                const result = await backend.requestConversation(
                    "getWorkspaceArticle",
                    conversationId,
                    articleId,
                    version ?? -1,
                );
                if (result) {
                    return JSON.parse(result as string);
                }
                return null;
            } catch (error) {
                console.error("[conversations] Failed to load workspace article:", error);
                return null;
            }
        },

        /**
         * 从 workspace 加载大纲数据
         */
        async getWorkspaceOutline(conversationId: string, articleId: string): Promise<OutlineData | null> {
            const backend = useBackendStore();
            try {
                const result = await backend.requestConversation("getWorkspaceOutline", conversationId, articleId);
                if (result) {
                    return JSON.parse(result as string) as OutlineData;
                }
                return null;
            } catch (error) {
                console.error("[conversations] Failed to load workspace outline:", error);
                return null;
            }
        },

        /**
         * 导出文章到本地文件（触发 C++ 文件保存对话框）
         */
        async exportWorkspaceArticle(conversationId: string, articleId: string, format: string): Promise<boolean> {
            // ExportMenu 格式 → C++ 后端格式
            const formatMap: Record<string, string> = {
                markdown: "md",
                word: "docx",
                pdf: "pdf",
            };
            const backendFormat = formatMap[format] ?? "md";
            const backend = useBackendStore();
            try {
                const result = await backend.requestConversation(
                    "saveWorkspaceArticleToFile",
                    conversationId,
                    articleId,
                    backendFormat,
                );
                return result as boolean;
            } catch (error) {
                console.error("[conversations] Failed to export article:", error);
                return false;
            }
        },

        // ============================================================
        // mcp
        getSelectedMcpServers(conversationId: string, enabledServiceIds: string[]) {
            if (!conversationId) {
                return [];
            }

            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (!conversationRecord) {
                console.error(
                    "getSelectedMcpServers Failed to find conversation record from conversationId:",
                    conversationId,
                );
                return [];
            }

            const hasStoredSelection = Object.prototype.hasOwnProperty.call(
                conversationRecord.conversationMcpSelections,
                conversationId,
            );
            const enabledIdSet = new Set(enabledServiceIds);

            if (!hasStoredSelection) {
                return [...enabledServiceIds];
            }

            const conversationMcpSelections = conversationRecord.conversationMcpSelections;
            if (!conversationMcpSelections || !conversationMcpSelections[conversationId]) {
                return [];
            }
            return conversationMcpSelections[conversationId].filter((serviceId) => enabledIdSet.has(serviceId));
        },

        setSelectedMcpServers(conversationId: string, serviceIds: string[], enabledServiceIds: string[]) {
            const conversationRecord = this.conversionList.get(conversationId) as ConversationRecord;
            if (!conversationRecord) {
                console.error(
                    "setSelectedMcpServers Failed to find conversation record from conversationId:",
                    conversationId,
                );
                return;
            }
            conversationRecord.setSelectedMcpServers(serviceIds, enabledServiceIds);
        },

        toggleSelectAllMcpServers(conversationId: string, enabledServiceIds: string[], selectAll: boolean) {
            this.setSelectedMcpServers(conversationId, selectAll ? enabledServiceIds : [], enabledServiceIds);
        },

        /**
         * 检查会话数限制，超限时显示 Toast 提示
         * @returns 允许创建返回 true，已达上限返回 false
         */
        async canCreateNewConversation(): Promise<boolean> {
            if (this.answeringSession.size === MAX_CONCURRENT_CHATS) {
                const notifyStore = useNotifyStore();
                notifyStore.showToast({
                    type: "warning",
                    message: useBackendStore().translate(
                        "Maximum of 10 concurrent chats reached. Please try again later.",
                    ),
                });
                return false;
            }
            return true;
        },
    },
});
