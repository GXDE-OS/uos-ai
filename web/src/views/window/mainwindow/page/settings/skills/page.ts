import { MAIN_WINDOW_WORKSPACE_PAGES } from "@/types/mainwindow";
import type { MainWindowWorkspacePageDefinition } from "@/utils/mainwindow/workspacePages";
import SkillsPage from "@/views/window/mainwindow/page/settings/skills/SkillsPage";

export const skillsWorkspacePageDefinition: MainWindowWorkspacePageDefinition = {
    id: MAIN_WINDOW_WORKSPACE_PAGES.SKILLS,
    component: SkillsPage,
    backButton: {
        text: "Back",
        fallbackPage: MAIN_WINDOW_WORKSPACE_PAGES.CHAT,
    },
};
