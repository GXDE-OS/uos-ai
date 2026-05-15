import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import type { MainWindowWorkspacePageDefinition } from "@/utils/mainwindow/workspacePages";
import McpServicesPage from "@/views/window/mainwindow/page/settings/mcpservices/McpServicesPage";

export const mcpServicesWorkspacePageDefinition: MainWindowWorkspacePageDefinition = {
    id: MAIN_WINDOW_WORKSPACE_PAGES.MCP_SERVICES,
    component: McpServicesPage,
    backButton: {
        text: "Back",
        fallbackPage: MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
    },
};
