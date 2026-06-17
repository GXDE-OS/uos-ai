export interface Conversation {
    root: Root;
    messages: Record<string, Message>;
}
export interface Root {
    id: string; // 根节点，对应conversation_id
    assistant: string; // 助手ID
    cur_next: string; // 子节点，对应第一个问题的id, 发送问题时更新
    model: string; // 模型ID
    next: Array<string>; // 下一个问题的id列表, 发送第一个问题时更新, TODO：重新编辑问题时追加
    extension: Record<string, any>; // 扩展字段（如 always_approve 等）
}

export interface Message {
    id?: string; // 消息ID（持久化后由后端分配，用于编辑大纲后的定向更新）
    // 前端渲染稳定 key：消息对象创建后即固定，不随 id 变化（如临时 AI 消息切换为后端真实 ID）
    // 用于 v-for key，避免 id 变更触发组件重挂载导致内部 UI 状态丢失（如 Reasoning 折叠态）
    frontendKey?: string;
    cur_next: string; // 下一个消息的id
    extension: Record<string, any>; // 扩展字段
    message: MessageItem[]; // 消息内容数组
    next: string[]; // 下一个消息的id列表
    previous: string; // 上一个消息的id
    render_message: RenderMessageItem[]; // 渲染消息数组
    role: UserType; // 角色类型：1-用户，2-助手
    model_id: string; // 模型ID
    model_name: string; // 模型名称
}

export interface MessageItem {
    content: ContentItem[]; // 内容数组
    role: string; // 角色：user/assistant
    source: string; // 来源
}

export interface ContentItem {
    content: string; // 文本内容
    type: string; // 类型：text等
}

export type ToolUseValue = string | number | boolean | null | Record<string, unknown> | unknown[];

export enum ToolUseStatus {
    Calling = 0, // 调用中
    Completed = 1, // 完成
    Failed = 2, // 失败
    Canceled = 3, // 取消
}

export interface ToolUseDisplayContentData {
    chat_type?: string; // 兼容旧版 display_content.chat_type
    index?: number; // 工具调用顺序
    name?: string; // 工具名称
    params?: ToolUseValue; // 调用参数
    result?: ToolUseValue; // 调用结果
    status?: number; // 调用状态
}

export interface ToolUseData {
    name?: string; // 标题或工具名称
    status?: number; // 调用状态
    index?: number; // 工具调用顺序
    params?: ToolUseValue; // 调用参数
    result?: ToolUseValue; // 调用结果
    display_content?: ToolUseDisplayContentData;
}

export interface RenderMessageItem {
    data:
        | RenderData
        | OutlineData
        | OutlineRefData
        | ReasoningData
        | AgentStepData
        | DocCardData
        | ToolUseData
        | ErrorMsg
        | CommandCardData
        | WebSearchData
        | BashApproveData
        | FileChangeApproveData; // 渲染数据
    type: string; // 类型：text, thinking, web_search, agent_step, tool, outline, doc_card, error, command_card, interactive_components 等
    isNew?: boolean; // 仅内存标记：流式新生成时为 true，历史加载时永远不携带此字段
}

// 指令卡片类型枚举
export enum CardType {
    None = "none", // 无卡片，直接输出结果
    SwitchCard = "switch_card", // 开关卡片（用于蓝牙、WiFi、勿扰模式等）
    SliderCard = "slider_card", // 滑块卡片（用于屏幕亮度、音量等）
    AppStoreCard = "app_store_card", // 应用商店卡片（用于应用商店搜索结果）
    ScheduleCard = "schedule_card", // 日程卡片
}

// 指令卡片数据结构
export interface CommandCardData {
    cardType: CardType; // 卡片类型
    cardData: any; // 卡片数据（根据卡片类型不同而不同）
    toolName: string; // 工具名称
    message?: string; // 操作结果消息
    errorCode?: number; // 错误码（0表示成功）
    extraData?: any; // 额外数据（如图标、应用名称等）
}

// 开关卡片数据
export interface SwitchCardData {
    switch: boolean; // 开关状态
    title: string; // 标题
    icon: string; // 图标名称
}

// 滑块卡片数据
export interface SliderCardData {
    percent?: number; // 百分比值（用于屏幕亮度、音量等）
    size?: number; // 大小值（用于系统字号等）
    title: string; // 标题
    icon: string; // 图标名称
    min: number; // 最小值
    max: number; // 最大值
    step?: number; // 步长（可选）
}

// 应用商店卡片数据
export interface AppStoreCardData {
    apps: AppInfo[]; // 应用信息列表（最多3个）
    title: string; // 标题
    icon: string; // 图标名称
    hoveredIndex?: number; // 当前悬停的应用索引
}

// 应用信息接口
export interface AppInfo {
    name: string; // 应用名称
    package: string; // 应用包名
    desc: string; // 应用简介
    downloads: number; // 下载次数
    rating: number; // 评分（0-5）
    icon: string; // 图标名称
}

// 日程卡片数据
export interface ScheduleCardData {
    subject: string; // 主题
    startTime: string; // 开始时间
    endTime: string; // 结束时间
    title: string; // 标题
    icon: string; // 图标名称
}

export interface RenderData {
    content: string; // 渲染内容
}

// 模型思考流数据结构（CntReasoning / type="thinking"）
// 键名 reasoning_content 与 C++ STR_KEY_REASONING_CONTENT 保持一致，避免持久化时内容丢失
export interface ReasoningData {
    reasoning_content: string; // 思考文本（流式累积）
    status: number; // 0=思考中，1=完成
    elapsed?: number; // 思考耗时（秒），完成时填入
}

// Agent 任务步骤条目：文本段 或 工具调用
export interface AgentStepTextEntry {
    kind: "text";
    content: string;
}

export interface AgentStepToolEntry {
    kind: "tool";
    data: ToolUseData;
}

export type AgentStepEntry = AgentStepTextEntry | AgentStepToolEntry;

// Agent 任务步骤数据结构（CntAgentStep / type="agent_step"）
export interface AgentStepData {
    title: string; // 步骤标题
    status: number; // 0=进行中，1=完成
    content?: string; // emitStep 携带的静态说明文本
    entries?: AgentStepEntry[]; // 有序条目：流式文本段 + 工具调用
}

// Online Search 数据结构（CntWebSearch/ type="web_search"）
export interface WebSearchData {
    title: string; // 搜索标题
    status: WebSearchStatus; // "searching"| "searched" | "reading" | "completed" | "failed"
    content?: WebSearchContent[]; // 搜索结果列表
}

export enum WebSearchStatus {
    SEARCHING = 0,
    READING = 1,
    COMPLETED = 2,
    FAILED = 3,
}

export interface WebSearchContent {
    url: string; // 搜索结果URL
    title: string; // 搜索结果标题
}

// 大纲引用数据（render_message 中存储的引用格式）
export interface OutlineRefData {
    id: string; // 关联的文章 ID
    title: string; // 大纲标题快照
}

// 大纲完整数据结构（从 workspace 加载后使用）
export interface OutlineData {
    paragraphs: OutlineParagraph[]; // 章节列表
    title: string; // 大纲标题
}

export interface OutlineParagraph {
    title: string; // 大章节标题
    content: OutlineSection[]; // 小章节列表
}

export interface OutlineSection {
    title: string; // 小章节标题
}

// 文章引用条目（由 getWorkspaceArticle 返回）
export interface ArticleReference {
    index: number; // 脚注编号（1 开始）
    title: string;
    url: string;
    website: string;
    icon: string; // base64 favicon
    snippet: string;
}

// 文档卡片数据结构（引用格式，实际内容从 workspace 加载）
export interface DocCardData {
    id: string; // 关联的文章 ID
    title: string; // 文档标题快照
    version?: number; // 生成时的版本号（-1 表示最新）
}

// 错误信息
export type ErrorMsg = {
    error: number; // 错误码
    error_message: string;
    http_error?: number; // HTTP 错误码 （仅当 error 为 HttpError 时有效）
};

// 交互组件状态 "pending":待批准；"reject":拒绝；"accept":接受
export enum InteractiveCompStatus {
    PENDING = "pending", // 待确认
    APPROVED = "approved", // 已确认
    REJECTED = "rejected", // 已拒绝
    CANCELED = "canceled", // 已取消（用户点击停止按钮）
}

// 交互组件子类型
export type InteractiveCompType = "bash_approve" | "file_change_approve";

// Bash 命令确认卡片数据
export interface BashApproveData {
    id: string; // request_id
    ic_type: InteractiveCompType;
    title: string; // 卡片标题
    command: string; // bash 命令
    status: InteractiveCompStatus; // 状态
}

// 文件变更确认卡片数据
export interface FileChangeApproveData {
    id: string; // request_id
    ic_type: InteractiveCompType;
    title: string; // 卡片标题
    status: InteractiveCompStatus; // 状态
    changes: Array<{
        path: string;
        kind: string; // "created" | "modified" | "deleted"
        is_dir: boolean;
    }>;
}

// 设置界面导航参数
export enum SettingNav {
    NORMAL = 0, // 普通设置
    MODEL = 1, // 模型设置
    KNOWLEDGE = 2, // 知识库设置
}

// 依赖包名
export enum DependencyPackage {
    RAG = "uos-ai-rag", // 向量化插件
    AGENT = "uos-ai-agent", // agent
}

export enum UserType {
    USER = 1, // 用户
    ASSISTANT = 2, // 助手
}

export interface ConversationIndex {
    id: string; // 会话ID
    title: string; // 会话标题
    updated_at: number; // 最后更新时间（时间戳）
    assistant: string; // 助手ID
    assistant_name: string; // 助手名称
    introduction: string; // 会话简介
}

// 会话在前端展示层的附加状态：
// 生成中用于展示 loading，未读用于提示用户有新回复，已读时不再显示额外标识。
export enum ConversationStatus {
    Generating = "generating",
    Unread = "unread",
    Read = "read",
}

// 侧边栏等列表视图使用的会话索引结构，在基础索引上附带当前展示状态。
export interface ConversationIndexWithStatus extends ConversationIndex {
    conversationStatus: ConversationStatus;
    hasPendingApproval: boolean;
}

export enum ConversationScene {
    Default = "default",
    Temporary = "temporary",
}
