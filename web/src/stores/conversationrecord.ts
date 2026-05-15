import type {
    Conversation as ConversationType,
    Root,
    Message,
    RenderMessageItem,
    ConversationIndex,
    OutlineData,
    OutlineRefData,
    DocCardData,
    ReasoningData,
    AgentStepData,
    AgentStepEntry,
    ToolUseData,
    ErrorMsg,
} from "@/types/conversation";
import { ConversationScene, ConversationStatus, UserType } from "@/types/conversation";
// import cloneDeep from "lodash/cloneDeep";
import { useBackendStore } from "@/stores";
import { getConversationSceneBehavior } from "@/utils/mainwindow/conversationScenes";

type ToolLikeData = Pick<ToolUseData, "name" | "status" | "index" | "params" | "result" | "display_content">;

export class ConversationRecord {
    // 私有的 Conversation 对象
    private _conversation: ConversationType | null = null;
    // 私有的 消息id
    private _messageId: string = "";
    // tempAnswerMessageId: 临时存储回答消息id
    private _tempAnswerMessageId: string = "";
    // 私有的 通话id, 收到end或error后, 清空
    private _sessionId: string = "";
    // 私有的 当前是否正在回答中
    private _isSessionStart: boolean = false;
    // 思考开始时间戳（ms），用于计算 elapsed
    private _thinkingStartTime: number = 0;
    // 私有的 mcp服务器选中状态
    private _conversationMcpSelections: Record<string, string[]> = {}; // conversationId -> selected MCP service ids
    // 当前记录是否从历史记录中恢复，默认false，继续对话时/创建新会话时为false，从历史记录中恢复时为true
    private _isFromHistory: boolean = false;
    // 事件监听器（信号槽机制）
    private _eventListeners: Map<string, Set<Function>> = new Map();
    // 当前会话场景，默认普通会话；不同场景可以决定持久化和展示策略。
    private _scene: ConversationScene = ConversationScene.Default;
    // 会话状态：生成中 / 未读 / 已读
    private _conversationStatus: ConversationStatus = ConversationStatus.Read;
    // 会话活动时间，未发生本地活动时为 null，优先使用后端 updated_at
    private _activityAt: number | null = null;
    // 控制卡片显示状态，默认false，切换会话时隐藏，发送新消息时显示
    private _showCards: boolean = false;

    // 构造函数：初始化会话记录
    constructor(id: string, root: Root) {
        this._conversation = {
            root: root,
            messages: {},
        };
        this.messageId = id; // 更新当前消息id
        // 初始化时，默认从创建会话开始，不是从历史记录中恢复
        this.isFromHistory = false;
    }

    // ========================================================================================================
    /**
     * get & set 方法
     */
    // Getter: 获取当前会话
    get conversation(): ConversationType | null {
        return this._conversation;
    }

    // Setter: 设置当前会话
    set conversation(value: ConversationType | null) {
        this._conversation = value;
    }

    // 获取根节点
    get root(): Root | null {
        return this._conversation?.root || null;
    }

    // 设置根节点
    set root(value: Root) {
        if (this._conversation) {
            this._conversation.root = value;
        }
    }

    // 获取消息列表
    get messages(): Record<string, Message> {
        return this._conversation?.messages || {};
    }

    // 设置消息列表
    set messages(value: Record<string, Message>) {
        if (this._conversation) {
            this._conversation.messages = value;
        }
    }

    // 根据会话id获取消息id
    // 这里获取的是最后一条消息的id，没有消息时返回的是根节点id
    get messageId(): string {
        return this._messageId;
    }

    // 更新当前消息id
    // 重试时，需要将messageId更新为被重试的问题消息id
    set messageId(value: string) {
        this._messageId = value;
    }

    // 获取通话id
    get sessionId(): string {
        return this._sessionId;
    }

    // 设置通话id
    set sessionId(value: string) {
        this._sessionId = value;
    }

    // 获取当前是否正在回答中
    get isSessionStart(): boolean {
        return this._isSessionStart;
    }
    // 设置当前是否正在回答中
    set isSessionStart(value: boolean) {
        this._isSessionStart = value;
    }

    // 获取会话mcp服务器选中状态
    get conversationMcpSelections(): Record<string, string[]> {
        return this._conversationMcpSelections;
    }

    // 设置会话mcp服务器选中状态
    set conversationMcpSelections(value: Record<string, string[]>) {
        this._conversationMcpSelections = value;
    }

    // 获取当前记录是否从历史记录中恢复
    get isFromHistory(): boolean {
        return this._isFromHistory;
    }

    // 设置当前记录是否从历史记录中恢复
    set isFromHistory(value: boolean) {
        this._isFromHistory = value;
    }

    // 获取当前会话场景
    get scene(): ConversationScene {
        return this._scene;
    }

    // 设置当前会话场景
    set scene(value: ConversationScene) {
        this._scene = value;
    }

    // 兼容旧调用方：临时会话本质上是一个具体场景，而不是单独的散落布尔值。
    get isTempConversation(): boolean {
        return this.scene === ConversationScene.Temporary;
    }

    // 兼容旧调用方：写入布尔值时映射到对应的场景。
    set isTempConversation(value: boolean) {
        this.scene = value ? ConversationScene.Temporary : ConversationScene.Default;
    }

    // 行为能力统一从 scene 派生，避免以后继续堆布尔状态。
    get shouldPersistHistory(): boolean {
        return getConversationSceneBehavior(this.scene).persistHistory;
    }

    // 行为能力统一从 scene 派生，避免以后继续堆布尔状态。
    get shouldShowInSidebar(): boolean {
        return getConversationSceneBehavior(this.scene).showInSidebar;
    }

    // 获取会话状态
    get conversationStatus(): ConversationStatus {
        return this._conversationStatus;
    }

    // 获取会话活动时间
    get activityAt(): number | null {
        return this._activityAt;
    }

    // 获取卡片显示状态
    get showCards(): boolean {
        return this._showCards;
    }

    // 设置卡片显示状态
    set showCards(value: boolean) {
        this._showCards = value;
    }

    // 获取单个消息
    getMessage(messageId: string): Message | null {
        return this._conversation?.messages[messageId] || null;
    }

    // 设置单个消息
    setMessage(messageId: string, message: Message): void {
        if (this._conversation) {
            // 首次插入时锁定前端 key，后续即使 id 变化（临时 AI 消息切换为真实 ID）也不重挂载
            if (!message.frontendKey) {
                message.frontendKey = messageId;
            }
            // 第一次发送问题时，更新root的cur_next和next列表
            if (Object.keys(this._conversation.messages).length === 0) {
                this._conversation.root.cur_next = messageId;
                this._conversation.root.next.push(messageId);
            }

            // 更新previous的cur_next和next列表
            const previousMessage = this.getMessage(message.previous);
            if (previousMessage) {
                previousMessage.cur_next = messageId;
                previousMessage.next = previousMessage.next.filter((id) => id !== this.messageId);
                previousMessage.next.push(messageId);
            }

            this._conversation.messages[messageId] = message;
            this.messageId = messageId; // 更新当前消息id
        }
    }

    // 删除消息
    deleteMessage(messageId: string): void {
        if (this._conversation) {
            delete this._conversation.messages[messageId];
        }
    }

    // 清空会话
    clear(): void {
        this._conversation = null;
    }

    initAiMessage() {
        this._tempAnswerMessageId = Date.now().toString(); // 初始化 AI 消息，正常收到结束的话，这个id是会被更新掉的
        const lastMessage = this.getMessage(this._messageId); // 找到问题消息
        const newMessage: Message = {
            id: this._tempAnswerMessageId, // 临时ID
            cur_next: "",
            extension: {},
            message: [],
            next: [],
            previous: this._messageId,
            render_message: [
                {
                    data: { content: "" },
                    type: "text",
                },
            ],
            role: UserType.ASSISTANT, // AI role
            model_id: lastMessage?.model_id || "", // 当前助手ID
            model_name: lastMessage?.model_name || "", // 当前助手名称
        };
        this.setMessage(this._tempAnswerMessageId, newMessage); // 添加新的 AI 消息
        this.isFromHistory = false; // 标记当前会话不是从历史记录中恢复的
    }

    // ============================================================
    /**
     * TODO: 处理session消息
     */
    // 处理会话开始事件
    handleSessionStarted(sessionId: string): void {
        this.isSessionStart = true;
    }

    // 处理会话消息事件
    handleSessionMessage(sessionId: string, message: string) {
        try {
            const parsedMessage = JSON.parse(message);
            const messageType = parsedMessage.type || "text";
            console.log("<<<<<<<<<<    parsedMessage:", parsedMessage);

            if (messageType === "outline") {
                // 处理大纲类型消息
                const outlineData: OutlineData = parsedMessage.data;
                console.log("<<<<<<<<<<    outlineData:", outlineData);
                this.updateAnsweringOutlineMessage(outlineData);
            } else if (messageType === "thinking") {
                // 模型思考流：累积到 CntReasoning 渲染块
                const content = parsedMessage.data?.reasoning_content || "";
                this.appendThinkingContent(content);
            } else if (messageType === "agent_step") {
                // Agent 任务步骤：管理 CntAgentStep 渲染块
                const title = (parsedMessage.data?.title as string) || "";
                const status = (parsedMessage.data?.status as number) ?? 0;
                const content = (parsedMessage.data?.content as string) || "";
                this.handleAgentStepMessage(title, status, content);
            } else if (messageType === "text") {
                this.finalizeThinkingBlockIfNeeded();
                const content = parsedMessage.data?.content || "";
                const activeBlock = this.getActiveAgentStepBlock();
                if (activeBlock) {
                    this.appendAgentStepContent(activeBlock.data as AgentStepData, content);
                } else {
                    this.updateAnsweringMessage(content);
                }
            } else if (messageType === "tool") {
                const activeBlock = this.getActiveAgentStepBlock();
                if (activeBlock) {
                    const toolData = this.normalizeToolUseData(parsedMessage.data);
                    if (toolData) this.upsertAgentStepToolItem(activeBlock.data as AgentStepData, toolData);
                } else {
                    this.handleToolLikeMessage(parsedMessage.data);
                }
            } else if (messageType === "doc_card") {
                // doc_card 始终推到 render_message 末端，不进入推理块细节
                this.addDocCardRenderItem(parsedMessage.data);
            } else if (messageType === "command_card") {
                // command_card 处理，添加到 render_message
                this.addCommandCardRenderItem(parsedMessage.data);
            } else {
                // 其他未知类型：仅处理 string content
                const content = parsedMessage.data?.content;
                if (content && typeof content === "string") {
                    this.updateAnsweringMessage(content);
                }
            }
        } catch (error) {
            console.error("Failed to parse session event message:", error, message);
        }
    }

    // 处理会话结束事件
    async handleSessionFinished(sessionId: string, message: string) {
        this.isSessionStart = false;
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息
        if (!lastMessage) return;
        // 更新消息ID
        try {
            const parsedMessage = JSON.parse(message); // 这次返回的是当前消息的messageID
            // 删除临时消息
            this.deleteMessage(this.messageId);
            // 保存当前消息
            lastMessage.id = parsedMessage.id;
            this.setMessage(parsedMessage.id, lastMessage);

            // 将模型思考块标记为完成，补写 elapsed（模型流无显式结束信号）
            const elapsedSec = this._thinkingStartTime ? Math.round((Date.now() - this._thinkingStartTime) / 1000) : 0;
            this._thinkingStartTime = 0;
            for (const item of lastMessage.render_message) {
                if (item.type === "thinking") {
                    const d = item.data as ReasoningData;
                    d.status = 1;
                    if (elapsedSec > 0 && d.elapsed == null) d.elapsed = elapsedSec;
                }
                // agent_step 块由显式 status=1 消息关闭，无需在此 patch
            }
            // 清理当前通话ID
            this.sessionId = "";

            // 保存渲染消息到后端
            const backend = useBackendStore();
            await backend.requestConversation(
                "setConversationRender",
                this.root?.id,
                parsedMessage.id,
                JSON.stringify(lastMessage.render_message),
            );

            // 某些场景只保留内存态，不应落历史文件。
            if (!this.shouldPersistHistory) {
                return;
            }

            // 通知后端写历史会话文件
            await backend.requestConversation("saveConversation", this.root?.id);

            // 触发会话完成信号
            this.emit("sessionFinished", this.root?.id, parsedMessage.id);
        } catch (error) {
            console.error("Failed to parse session event message:", error, message);
        }
    }

    // 处理会话错误事件
    async handleSessionError(sessionId: string, message: string) {
        this.isSessionStart = false;
        // 异常结束时也要清理会话运行态，避免前端继续把它识别成"生成中"。
        // this.sessionId = "";
        // TODO: 处理错误事件，例如记录日志、通知用户等
        console.error("Session error:", sessionId, message);
        try {
            const parsedMessage = JSON.parse(message);
            this.addErrorMsgRenderItem(parsedMessage as ErrorMsg);
        } catch (error) {
            console.error("Failed to parse session error message:", error, message);
            return;
        }

        // TODO : 没收到start，直接收到error， 需要保存历史会话记录
        // 保存渲染消息到后端
        // const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息
        // if (!lastMessage) return;
        // const backend = useBackendStore();
        // await backend.requestConversation(
        //     "setConversationRender",
        //     this.root?.id,
        //     lastMessage.id,
        //     JSON.stringify(lastMessage.render_message),
        // );

        // // 通知后端写历史会话文件
        // await backend.requestConversation("saveConversation", this.root?.id);
    }
    // ============================================================
    /**
     * 处理会话消息事件的内部方法
     */
    // 更新正在回答的大纲消息
    updateAnsweringOutlineMessage(outlineData: OutlineData) {
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息
        if (!lastMessage) return;

        // 在 render_message 数组中查找 outline 类型的消息
        const outlineMessage = lastMessage.render_message.find((item) => item.type === "outline");
        if (outlineMessage) {
            // 更新现有大纲消息
            outlineMessage.data = outlineData;
        } else {
            // 没有找到大纲消息，创建新的大纲消息
            lastMessage.render_message.push({
                data: outlineData,
                type: "outline",
            });
        }
    }

    // 返回当前 active（status=0）agent_step block，不存在则返回 null
    getActiveAgentStepBlock(): RenderMessageItem | null {
        const lastMessage = this.getMessage(this._messageId);
        if (!lastMessage) return null;
        return (
            lastMessage.render_message.find(
                (item) => item.type === "agent_step" && (item.data as AgentStepData).status === 0,
            ) ?? null
        );
    }

    // 将流式文本追加到 agent_step block 的 entries（追加到最后一个文本段，或新建）
    appendAgentStepContent(data: AgentStepData, chunk: string) {
        if (!data.entries) data.entries = [];
        const last = data.entries[data.entries.length - 1];
        if (last && last.kind === "text") {
            last.content += chunk;
        } else {
            const trimmed = chunk.trimStart();
            if (trimmed) data.entries.push({ kind: "text", content: trimmed });
        }
    }

    // 将工具消息内嵌到 agent_step block 的 entries，支持按 index 合并更新
    upsertAgentStepToolItem(data: AgentStepData, toolData: ToolUseData) {
        if (!data.entries) data.entries = [];
        const existing = this.findMatchingToolLikeEntry(
            data.entries.filter((e): e is Extract<AgentStepEntry, { kind: "tool" }> => e.kind === "tool"),
            toolData,
            (entry) => entry.data,
        );
        if (existing) {
            this.mergeToolLikeData(existing.data, toolData);
        } else {
            data.entries.push({ kind: "tool", data: { ...toolData } });
        }
    }

    // 处理顶层 tool 消息
    handleToolLikeMessage(rawData: unknown) {
        const toolData = this.normalizeToolUseData(rawData);
        if (!toolData) {
            console.warn("Invalid tool-like message:", rawData);
            return;
        }
        this.upsertAnsweringStandaloneToolMessage(toolData);
    }

    normalizeToolUseData(rawData: unknown): ToolUseData | null {
        if (!rawData || typeof rawData !== "object" || Array.isArray(rawData)) {
            return null;
        }

        const toolData = { ...(rawData as Record<string, unknown>) } as ToolUseData;
        if (toolData.display_content && !this.isPlainObject(toolData.display_content)) {
            delete toolData.display_content;
        } else if (toolData.display_content) {
            toolData.display_content = { ...toolData.display_content };
        }

        return toolData;
    }

    // 将未完成的 thinking 块标记为完成（提前于 SeFinished，当正文开始时调用）
    finalizeThinkingBlockIfNeeded() {
        const lastMessage = this.getMessage(this._messageId);
        if (!lastMessage) return;
        const thinking = lastMessage.render_message.find((item) => item.type === "thinking");
        if (!thinking) return;
        const d = thinking.data as ReasoningData;
        if (d.status === 1) return;
        d.status = 1;
        if (this._thinkingStartTime) {
            d.elapsed = Math.round((Date.now() - this._thinkingStartTime) / 1000);
            this._thinkingStartTime = 0;
        }
    }

    // 追加模型思考文本到 CntReasoning 渲染块，不存在则创建
    appendThinkingContent(content: string) {
        const lastMessage = this.getMessage(this._messageId);
        if (!lastMessage) return;

        const existing = lastMessage.render_message.find((item) => item.type === "thinking");
        if (!existing) {
            this._thinkingStartTime = Date.now();
            lastMessage.render_message.unshift({
                data: { reasoning_content: content, status: 0 } as ReasoningData,
                type: "thinking",
            });
            return;
        }

        (existing.data as ReasoningData).reasoning_content += content;
    }

    // 处理 Agent 步骤消息（CntAgentStep）
    // status=0：开始新步骤或更新当前步骤标题；status=1：关闭当前步骤
    handleAgentStepMessage(title: string, status: number, content: string) {
        const lastMessage = this.getMessage(this._messageId);
        if (!lastMessage) return;

        const existing = lastMessage.render_message.find(
            (item) => item.type === "agent_step" && (item.data as AgentStepData).status === 0,
        );

        if (status === 0) {
            if (!existing) {
                // 新步骤开始：创建新 block
                lastMessage.render_message.push({
                    data: { title, status: 0, content, entries: [] } as AgentStepData,
                    type: "agent_step",
                });
            } else {
                // 已有活跃步骤：更新标题和内容
                const d = existing.data as AgentStepData;
                d.title = title;
                d.content = content;
            }
        } else {
            // status 非 0：关闭活跃步骤（Completed=1 / Failed=2 / Canceled=3）
            if (existing) {
                const d = existing.data as AgentStepData;
                if (title) d.title = title;
                d.content = content;
                d.status = status;
            }
        }
    }

    // 将独立工具消息插入到 render_message，后续状态更新时按同一工具项合并
    upsertAnsweringStandaloneToolMessage(toolData: ToolUseData) {
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息
        if (!lastMessage) return;

        const existingItem = this.findMatchingToolLikeEntry(lastMessage.render_message, toolData, (item) => {
            return item.type === "tool" ? (item.data as ToolUseData) : null;
        });

        if (existingItem) {
            this.mergeToolLikeData(existingItem.data as ToolUseData, toolData);
            return;
        }

        // 顶层工具卡片沿用 tool 类型，兼容现有渲染和持久化解析逻辑
        lastMessage.render_message.push({
            data: { ...toolData },
            type: "tool",
        });
    }

    findMatchingToolLikeEntry<T>(
        items: T[],
        incoming: ToolLikeData,
        getData: (item: T) => ToolLikeData | null,
    ): T | null {
        const incomingIndex = this.getToolLikeIndex(incoming);
        if (incomingIndex !== undefined) {
            for (let i = items.length - 1; i >= 0; i--) {
                const item = items[i];
                if (item === undefined) continue;
                const data = getData(item);
                if (data && this.getToolLikeIndex(data) === incomingIndex) {
                    return item;
                }
            }
            return null;
        }

        const incomingName = this.getToolLikeName(incoming);
        if (!incomingName) {
            return null;
        }

        for (let i = items.length - 1; i >= 0; i--) {
            const item = items[i];
            if (item === undefined) continue;
            const data = getData(item);
            if (!data) continue;
            if (this.getToolLikeName(data) !== incomingName) continue;
            if (this.getToolLikeStatus(data) === 0) {
                return item;
            }
        }

        return null;
    }

    mergeToolLikeData(target: ToolLikeData, incoming: ToolLikeData) {
        if (incoming.name !== undefined) target.name = incoming.name;
        if (incoming.status !== undefined) target.status = incoming.status;
        if (incoming.index !== undefined) target.index = incoming.index;
        if (incoming.params !== undefined) target.params = incoming.params;
        if (incoming.result !== undefined) target.result = incoming.result;

        if (incoming.display_content !== undefined) {
            target.display_content = {
                ...(target.display_content ?? {}),
                ...incoming.display_content,
            };
        }
    }

    getToolLikeName(data: ToolLikeData): string {
        const name = data.display_content?.name ?? data.name;
        return typeof name === "string" ? name.trim() : "";
    }

    getToolLikeStatus(data: ToolLikeData): number | undefined {
        const status = data.display_content?.status ?? data.status;
        return typeof status === "number" ? status : undefined;
    }

    getToolLikeIndex(data: ToolLikeData): number | undefined {
        const index = data.display_content?.index ?? data.index;
        return typeof index === "number" ? index : undefined;
    }

    getToolLikeParams(data: ToolLikeData) {
        return data.display_content?.params ?? data.params;
    }

    getToolLikeResult(data: ToolLikeData) {
        return data.display_content?.result ?? data.result;
    }

    isPlainObject(value: unknown): value is Record<string, unknown> {
        return !!value && typeof value === "object" && !Array.isArray(value);
    }

    // 更新正在回答的消息内容
    // 向后查找最后一个 text 类型条目，兼容推理块夹在中间的情况
    updateAnsweringMessage(content: string) {
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息
        if (!lastMessage) return;

        const lastItem = lastMessage.render_message[lastMessage.render_message.length - 1];

        // 如果最后一个条目是text且内容不为空，追加内容
        if (lastItem && lastItem.type === "text" && "content" in lastItem.data) {
            lastItem.data.content += content;
        } else {
            // 否则创建新的text条目
            lastMessage.render_message.push({
                data: { content },
                type: "text",
            });
        }

        // 同时更新 message 内容
        const lastMessageItem = lastMessage.message[lastMessage.message.length - 1];
        if (lastMessageItem) {
            const lastContentItem = lastMessageItem.content[lastMessageItem.content.length - 1];
            if (lastContentItem) {
                lastContentItem.content += content;
            }
        }
    }

    // 将 doc_card 直接推送到 render_message 末端（绕过推理块）
    addDocCardRenderItem(data: any) {
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息

        if (!lastMessage) return;
        lastMessage.render_message.push({
            data,
            isNew: true,
            type: "doc_card",
        });
    }

    addCommandCardRenderItem(data: any) {
        const lastMessage = this.getMessage(this._messageId);

        if (!lastMessage) return;
        console.log("Adding command_card to render_message:", data);
        lastMessage.render_message.push({
            data,
            type: "command_card",
        });
    }

    // 持久化用户编辑后的大纲数据
    updateOutlineData(messageId: string, outlineData: OutlineData) {
        // 1. 在内存中定位消息
        const targetMessage = this.getMessage(messageId);
        if (!targetMessage) {
            console.error("updateOutlineData: message not found", messageId);
            return;
        }

        // 2. 更新 render_message 中的 outline 引用标题（保持引用格式）
        const outlineItem = targetMessage.render_message.find((item) => item.type === "outline");
        if (outlineItem) {
            const refData = outlineItem.data as OutlineRefData;
            refData.title = outlineData.title; // 只更新标题，保持引用格式
        } else {
            console.warn("updateOutlineData: no outline item found in render_message");
            return;
        }

        // 3. 同步更新 WritingWorkspace 中的完整大纲数据
        const backend = useBackendStore();
        backend.requestConversation("updateWorkspaceOutline", this.root?.id || "", JSON.stringify(outlineData));
    }

    // 更新当前会话目标消息的cur_next
    async updateCurrentMessageCurrNext(messageId: string, newCurrNext: string) {
        const message = this.getMessage(messageId);

        if (!message) {
            console.error("updateOutlineData: message not found", messageId);
            return;
        }

        message.cur_next = newCurrNext;

        // 从root.cur_next开始遍历，设置当前最后一个消息为currentMessageId
        let currentId = this.root?.cur_next;
        while (currentId && this.messages[currentId]) {
            const message = this.messages[currentId];
            if (message) {
                currentId = message.cur_next;
                if (currentId === "") {
                    // 下一个为空
                    this.messageId = message.id || "";
                }
            } else {
                break;
            }
        }

        // TODO: 通知后端同步修改cur_next
        const backend = useBackendStore();
        await backend.requestConversation("switchMessageNext", this.root?.id || "", messageId, newCurrNext);
        if (!this.shouldPersistHistory) {
            return;
        }

        await backend.requestConversation("saveConversation", this.root?.id || ""); // 保存会话
    }
    // ============================================================
    // mcp
    /**
     * 设置选中的MCP服务器列表
     * @param serviceIds - 需要设置的MCP服务器ID数组
     * @param enabledServiceIds - 已启用的MCP服务器ID数组，这些ID将用于排序
     */
    setSelectedMcpServers(serviceIds: string[], enabledServiceIds: string[]) {
        const enabledOrder = new Map(enabledServiceIds.map((serviceId, index) => [serviceId, index]));
        const normalizedIds = Array.from(new Set(serviceIds))
            .filter((serviceId) => enabledOrder.has(serviceId))
            .sort((leftId, rightId) => {
                return (
                    (enabledOrder.get(leftId) ?? Number.MAX_SAFE_INTEGER) -
                    (enabledOrder.get(rightId) ?? Number.MAX_SAFE_INTEGER)
                );
            });

        this.conversationMcpSelections = {
            ...this.conversationMcpSelections,
            [this.root?.id || ""]: normalizedIds,
        };
    }

    // ============================================================
    // 错误信息
    addErrorMsgRenderItem(data: ErrorMsg) {
        const lastMessage = this.getMessage(this._messageId); // 找到当前待填充消息

        if (!lastMessage) return;
        lastMessage.render_message.push({
            data: data,
            type: "error",
        });
    }

    // ============================================================
    // 会话状态
    updateConversationState(options: { status?: ConversationStatus; updatedAt?: number | null; touch?: boolean }) {
        // touch=true 表示本次状态切换应顺带刷新活动时间，用于列表排序。
        const nextStatus = options.status ?? this._conversationStatus;
        const nextUpdatedAt =
            options.updatedAt !== undefined ? options.updatedAt : options.touch ? Date.now() : this._activityAt;
        const hasChanged = nextStatus !== this._conversationStatus || nextUpdatedAt !== this._activityAt;

        this._conversationStatus = nextStatus;
        this._activityAt = nextUpdatedAt;

        if (hasChanged) {
            // 复用现有信号机制通知外层 store，让列表层自己决定如何刷新展示。
            this.emit("conversationStateChange", {
                status: this._conversationStatus,
                updatedAt: this._activityAt,
            });
        }
    }

    markAsGenerating() {
        // 进入生成态时总是刷新活动时间，确保会话在列表中提升到最新位置。
        this.updateConversationState({
            status: ConversationStatus.Generating,
            touch: true,
        });
    }

    markAsUnread() {
        // 新回复对用户不可见时标记未读，并同步刷新活动时间。
        this.updateConversationState({
            status: ConversationStatus.Unread,
            touch: true,
        });
    }

    markAsRead(touch: boolean = false) {
        // 已读场景下是否刷新活动时间由调用方决定，例如会话结束时需要，普通切换会话时不需要。
        this.updateConversationState({
            status: ConversationStatus.Read,
            touch,
        });
    }

    touchActivityAt(updatedAt: number = Date.now()) {
        this.updateConversationState({
            updatedAt,
        });
    }

    // ============================================================
    // 事件监听器（信号槽机制）
    /**
     * 注册事件监听器
     * @param eventName - 事件名称
     * @param callback - 回调函数
     */
    on(eventName: string, callback: Function): void {
        if (!this._eventListeners.has(eventName)) {
            this._eventListeners.set(eventName, new Set());
        }
        this._eventListeners.get(eventName)!.add(callback);
    }

    /**
     * 移除事件监听器
     * @param eventName - 事件名称
     * @param callback - 回调函数
     */
    off(eventName: string, callback: Function): void {
        const listeners = this._eventListeners.get(eventName);
        if (listeners) {
            listeners.delete(callback);
            if (listeners.size === 0) {
                this._eventListeners.delete(eventName);
            }
        }
    }

    /**
     * 触发事件
     * @param eventName - 事件名称
     * @param args - 传递给回调函数的参数
     */
    emit(eventName: string, ...args: any[]): void {
        const listeners = this._eventListeners.get(eventName);
        if (listeners) {
            listeners.forEach((callback) => {
                try {
                    callback(...args);
                } catch (error) {
                    console.error(`Error in event listener for "${eventName}":`, error);
                }
            });
        }
    }
}
