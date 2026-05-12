/**
 * Report event types enum (using numeric values)
 */
export enum ReportEventType {
    // No-argument types
    AiBarPoint = 1,
    WriterPoint = 2,
    ChatwindowPoint = 3,
    ScreenShotClickedPoint = 4,
    DigitalChatPoint = 5,
    PrivateChatClickedPoint = 6,
    ChatwindowStartPoint = 7,
    FollowalongPoint = 8,
    // String parameter types
    WriterFunctionPoint = 9,
    MCPChatPoint = 10,
    AssistantChatTypePoint = 11,
    KnowledgeFunctionPoint = 12,
    PrivateChatPoint = 13,
    FunctioncallPoint = 14,
    KnowledgeFileTypePoint = 15,
    // Integer parameter types
    FollowFunctionPoint = 16,
    KnowledgeFileNumberPoint = 17,
    // Special types with object parameters
    ModelPoint = 18,
    AssistantChatPoint = 19,
}

/**
 * Model point parameters
 */
export interface ModelPointParams {
    model_species: string;
    model_type: string;
}

/**
 * Assistant chat point parameters
 */
export interface AssistantChatPointParams {
    assistant_type: string;
}

/**
 * Report event payload
 */
export interface ReportEventPayload {
    type: ReportEventType;
    params?: string | number | ModelPointParams | AssistantChatPointParams;
}
