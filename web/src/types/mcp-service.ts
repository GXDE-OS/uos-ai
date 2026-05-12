export const MCP_SERVICE_CATEGORY = {
    SYSTEM_BUILT_IN: "systemBuiltIn",
    THIRD_PARTY_BUILT_IN: "thirdPartyBuiltIn",
    CUSTOM: "custom",
} as const;

export type McpServiceCategory =
    (typeof MCP_SERVICE_CATEGORY)[keyof typeof MCP_SERVICE_CATEGORY];

export const MCP_SERVICE_FILTER = {
    ALL: "all",
    BUILT_IN: "builtIn",
    CUSTOM: "custom",
} as const;

export type McpServiceFilter =
    (typeof MCP_SERVICE_FILTER)[keyof typeof MCP_SERVICE_FILTER];

export const MCP_SERVICE_EDITOR_MODE = {
    ADD: "add",
    EDIT: "edit",
} as const;

export type McpServiceEditorMode =
    (typeof MCP_SERVICE_EDITOR_MODE)[keyof typeof MCP_SERVICE_EDITOR_MODE];

export interface McpService {
    id: string;
    name: string;
    description: string;
    category: McpServiceCategory;
    enabled: boolean;
    isBuiltIn: boolean;
    editable: boolean;
    removable: boolean;
    jsonConfig?: string;
}

export interface McpServiceDraft {
    id?: string;
    description: string;
    jsonConfig: string;
}
