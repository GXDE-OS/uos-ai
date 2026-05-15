import { chatWorkspacePageDefinition } from "@/views/window/mainwindow/page/chat/page";
import { historyConversationWorkspacePageDefinition } from "@/views/window/mainwindow/page/historyconversation/page";
import { mcpServicesWorkspacePageDefinition } from "@/views/window/mainwindow/page/settings/mcpservices/page";
import { skillsWorkspacePageDefinition } from "@/views/window/mainwindow/page/settings/skills/page";
import { registerMainWindowWorkspacePage } from "@/utils/mainwindow/workspacePages";
import { digitalHumanWorkspacePageDefinition } from "@/views/window/mainwindow/page/digitalhuman/page";

let isBuiltInWorkspacePagesRegistered = false;

export const ensureBuiltInWorkspacePagesRegistered = () => {
    if (isBuiltInWorkspacePagesRegistered) {
        return;
    }

    registerMainWindowWorkspacePage(chatWorkspacePageDefinition);
    registerMainWindowWorkspacePage(historyConversationWorkspacePageDefinition);
    registerMainWindowWorkspacePage(mcpServicesWorkspacePageDefinition);
    registerMainWindowWorkspacePage(skillsWorkspacePageDefinition);
    registerMainWindowWorkspacePage(digitalHumanWorkspacePageDefinition);

    isBuiltInWorkspacePagesRegistered = true;
};
