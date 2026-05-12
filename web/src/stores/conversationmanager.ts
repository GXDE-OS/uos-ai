import { defineStore } from "pinia";

import type {
    ConversationIndex,
    Message,
    Conversation,
    OutlineData,
    Root,
    ConversationIndexWithStatus,
} from "@/types/conversation";
import { ConversationScene, ConversationStatus } from "@/types/conversation";
import type { Assistant } from "@/types/assistant";
import { ConversationRecord, useBackendStore, useAssistantInfosStore, useExtensionPanelStore } from "@/stores";
import { useNotifyStore } from "@/stores/notify";
import { WindowMode } from "@/types/windowinfo";

// 打印组件
// import cloneDeep from "lodash/cloneDeep";

// 最大并发会话数
export const MAX_CONCURRENT_CHATS = 10;

// ConversationRecord 内部状态变化后，需要替换 Map 引用，才能让依赖该 Map 的计算属性重新求值。
const handleConversationRecordStateChange = () => {
    const conversationManagerStore = useConversationManagerStore();
    conversationManagerStore.conversionList = new Map(conversationManagerStore.conversionList);
};

export const useConversationManagerStore = defineStore("conversationmanager", {
    state: () => ({
        conversationIndexList: [] as ConversationIndex[], // 会话索引列表
        conversionList: new Map<string, ConversationRecord>(), // 所有正在回答中的会话，及当前会话
        answeringSession: new Map<string, string>(), // 当前sessionId对应的conversationId, key: sessionId, value: conversationId
        currentConversationId: "", // 当前聊天窗口展示的会话id
        aiReplyNotifyTimer: null as number | null, // 回复完成通知防抖 timer（500ms）
        aiReplyLatestConversationId: "", // 本轮防抖窗口中的最新会话
        aiReplyLatestMessageId: "", // 本轮防抖窗口中的最新消息
        aiReplyNotificationTargets: {} as Record<number, { conversationId: string; messageId: string }>, // 通知ID -> 跳转目标
        aiReplyNotificationListenerInitialized: false, // 系统通知 action 监听是否已注册
    }),

    getters: {
        getConversationById: (state) => (conversationId: string) => state.conversionList.get(conversationId), // 根据id获取会话
        getConversationIndexList: (state) => state.conversationIndexList, // 获取会话索引列表
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
                    ...conversationIndex,
                    updated_at: conversationRecord?.activityAt ?? conversationIndex.updated_at,
                    conversationStatus: this.getConversationStatus(conversationIndex.id),
                });
            }

            for (const [conversationId, conversationRecord] of this.conversionList.entries()) {
                if (!conversationRecord.shouldShowInSidebar || Object.keys(conversationRecord.messages).length === 0) {
                    continue;
                }

                if (conversationIndexMap.has(conversationId)) {
                    continue;
                }

                // 还未落盘的会话只有在“当前会话”或“存在未读/生成中状态”时才需要出现在侧边栏。
                const shouldShowFallbackConversation =
                    this.currentConversationId === conversationId ||
                    this.getConversationStatus(conversationId) !== ConversationStatus.Read;

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
            if (this.aiReplyNotificationListenerInitialized) {
                return;
            }
            const notifyStore = useNotifyStore();
            notifyStore.onSystemNotificationAction(({ notificationId, actionKey }) => {
                void this.handleAiReplyNotificationAction(notificationId, actionKey);
            });
            this.aiReplyNotificationListenerInitialized = true;
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
            this.aiReplyLatestMessageId = messageId;

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
            console.log("[AiReplyNotification] Flushing notification, unreadCount:", unreadCount);
            if (unreadCount <= 0) {
                console.log("[AiReplyNotification] No unread conversations, skip notification");
                return;
            }

            const backend = useBackendStore();
            const notifyStore = useNotifyStore();
            const countText = String(unreadCount);
            const body = backend.translate("You have %1 newly answered chats").replace("%1", countText);

            console.log("[AiReplyNotification] Showing notification, body:", body);
            const notificationId = await notifyStore.showSystemNotification({
                body,
                actions: [
                    { key: "remind_later", text: backend.translate("Remind Me Later") },
                    { key: "view_now", text: backend.translate("View Now") },
                ],
                timeoutMs: 5000,
            });

            console.log("[AiReplyNotification] Notification shown, id:", notificationId);
            if (!notificationId) {
                return;
            }

            this.aiReplyNotificationTargets[notificationId] = {
                conversationId: this.aiReplyLatestConversationId,
                messageId: this.aiReplyLatestMessageId,
            };
            console.log("[AiReplyNotification] Target saved:", this.aiReplyNotificationTargets[notificationId]);
        },

        getLatestUnreadConversationTarget() {
            const unreadList = this.getConversationIndexListWithStatus.filter(
                (conversation) => conversation.conversationStatus === ConversationStatus.Unread,
            );
            if (unreadList.length === 0) {
                return null;
            }

            const latestConversation = unreadList[0];
            if (!latestConversation) {
                return null;
            }
            const latestConversationRecord = this.conversionList.get(latestConversation.id);
            return {
                conversationId: latestConversation.id,
                messageId: latestConversationRecord?.messageId ?? "",
            };
        },

        async handleAiReplyNotificationAction(notificationId: number, actionKey: string) {
            console.log(
                "[AiReplyNotification] Action received, notificationId:",
                notificationId,
                "actionKey:",
                actionKey,
            );
            if (!actionKey) {
                console.log("[AiReplyNotification] No actionKey, skip");
                return;
            }

            const notifyStore = useNotifyStore();
            const target = this.aiReplyNotificationTargets[notificationId] ?? this.getLatestUnreadConversationTarget();
            console.log("[AiReplyNotification] Target:", target);

            if (actionKey === "remind_later") {
                console.log("[AiReplyNotification] User chose remind_later, closing notification");
                await notifyStore.closeSystemNotification(notificationId);
                delete this.aiReplyNotificationTargets[notificationId];
                return;
            }

            if (actionKey !== "view_now") {
                console.log("[AiReplyNotification] Unknown actionKey:", actionKey, ", skip");
                return;
            }

            console.log("[AiReplyNotification] User chose view_now, switching to conversation");
            await notifyStore.closeSystemNotification(notificationId);
            delete this.aiReplyNotificationTargets[notificationId];

            if (!target?.conversationId) {
                console.log("[AiReplyNotification] No target conversationId, skip");
                return;
            }

            console.log("[AiReplyNotification] Switching to conversation:", target.conversationId);
            await useBackendStore().requestWindow("switchMode", WindowMode.Main);
            useExtensionPanelStore().closeExtensionPanel();
            await this.switchConversation(target.conversationId);
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
        },

        detachConversationRecord(conversationRecord: ConversationRecord) {
            conversationRecord.off("conversationStateChange", handleConversationRecordStateChange);
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
