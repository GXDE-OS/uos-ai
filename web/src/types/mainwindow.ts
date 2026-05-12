export const MAIN_WINDOW_WORKSPACE_PAGES = {
    CHAT: "chat",
    HISTORY_CONVERSATION: "historyConversation",
    MCP_SERVICES: "mcpServices",
    SKILLS: "skills",
    DIGITAL_HUMAN: "digitalHuman",
} as const;

export type BuiltInMainWindowWorkspacePage =
    (typeof MAIN_WINDOW_WORKSPACE_PAGES)[keyof typeof MAIN_WINDOW_WORKSPACE_PAGES];

export type MainWindowWorkspacePage = BuiltInMainWindowWorkspacePage | (string & {});
