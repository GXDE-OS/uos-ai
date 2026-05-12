export enum SessionEvent {
    SeUnknown = 0,
    SeStarted = 1,
    SeFinished = 2,
    SeError = 3,
    SeMessage = 4,
}

export enum CopyDataType {
    CopyText = 0, // 复制文本
    CopyImage = 1, // 复制图片
}

export enum ContentType {
    CntText = "text", // 文本
    CntImage = "image", // 文生图
    CntFile = "file", // 文件
    CntTool = "tool", // 工具调用
    CntInstruction = "instruction", // 指令
    CntReasoning = "thinking", // 模型思考流（thinking token）
    CntAgentStep = "agent_step", // Agent 任务步骤进度
    CntOutline = "outline", // 大纲
    CntDocCard = "doc_card", // 文档卡片
    CntGuessYouWant = "guess_you_want", // 猜你想要
    CntError = "error", // 错误
}

/**
 * 会话事件处理器类型
 */
export type SessionEventHandler = (event: SessionEvent, sessionId: string, message: string) => void;

/**
 * 聊天消息类型定义
 */
export interface ChatMessage {
    session_id: string;
    conversation_id: string;
    assistant: string;
    model: string;
    model_name: string;
    user: string;
    params: Object;
    message: {
        id: string; // 消息ID，时间戳
        previous: string; // 重试时上一条消息的message.id
        content: Array<{
            type: string;
            data: object;
        }>;
        extension?: Record<string, any>;
    };
}

/**
 * parans
 */
export interface Params {
    [key: string]: any;
}

/**
 * 是否重试
 */
export interface Retry {
    is_retry: boolean; // 是否重试
}
