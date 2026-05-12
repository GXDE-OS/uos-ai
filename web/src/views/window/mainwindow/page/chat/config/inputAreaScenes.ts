import { AssistantID } from "@/types/assistant";
import type { ChatInputSceneConfig } from "@/types/chat-input";
import { FileCategory } from "@/types/uploadfile";
import McpInputExtension from "@/views/window/mainwindow/page/chat/components/inputExtensions/McpInputExtension";
import {
    LOCAL_FILE_INPUT_ACTION,
    REFERENCE_OUTLINE_INPUT_ACTION,
    SCREENSHOT_INPUT_ACTION,
    UPLOAD_FILE_INPUT_ACTION,
    SEPARATOR_INPUT_ACTION,
} from "@/views/window/mainwindow/page/chat/config/inputAreaActions";

const DEFAULT_INPUT_AREA_ACTIONS = [UPLOAD_FILE_INPUT_ACTION, SCREENSHOT_INPUT_ACTION];
const WRITING_INPUT_AREA_ACTIONS = [REFERENCE_OUTLINE_INPUT_ACTION, SEPARATOR_INPUT_ACTION, LOCAL_FILE_INPUT_ACTION];

const DEFAULT_INPUT_AREA_SCENE_CONFIG: ChatInputSceneConfig = {
    actions: DEFAULT_INPUT_AREA_ACTIONS,
};

const INPUT_AREA_SCENE_CONFIG_MAP: Partial<Record<AssistantID, ChatInputSceneConfig>> = {
    [AssistantID.UOS_AI_WRITING]: {
        actions: WRITING_INPUT_AREA_ACTIONS,
        resolveFileDrop: (paths) => {
            if (paths.length <= 1) {
                return {
                    type: "file-category-select",
                    buttons: [
                        {
                            text: "",
                            type: "default",
                            category: FileCategory.Material,
                        },
                        {
                            text: "",
                            type: "default",
                            category: FileCategory.Outline,
                        },
                    ],
                };
            }

            return {
                type: "file-category-select",
                buttons: [
                    {
                        text: "",
                        type: "default",
                        category: FileCategory.Material,
                    },
                ],
            };
        },
    },
    [AssistantID.UOS_AI_MCP_AND_SKILL]: {
        actions: DEFAULT_INPUT_AREA_ACTIONS,
        actionExtension: McpInputExtension,
        resolveParams: ({ getSelectedMcpServiceIds }) => ({
            mcpServers: getSelectedMcpServiceIds(),
        }),
    },
};

export const getInputAreaSceneConfig = (assistantId?: string | null): ChatInputSceneConfig => {
    const sceneConfig = assistantId ? INPUT_AREA_SCENE_CONFIG_MAP[assistantId as AssistantID] : undefined;

    return {
        ...DEFAULT_INPUT_AREA_SCENE_CONFIG,
        ...sceneConfig,
        actions: [...(sceneConfig?.actions || DEFAULT_INPUT_AREA_ACTIONS)],
    };
};
