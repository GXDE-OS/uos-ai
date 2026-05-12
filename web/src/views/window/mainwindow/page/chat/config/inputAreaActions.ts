import type { ChatInputAction, ChatInputActionContext } from "@/types/chat-input";
import type { MenuItem } from "@/types/menu";
import { useBackendStore } from "@/stores";
import { FileCategory } from "@/types/uploadfile";

export const SEPARATOR_INPUT_ACTION: ChatInputAction = {
    id: "separator",
    get menuItem(): MenuItem {
        return {
            type: "separator",
            id: "separator",
        };
    },
};

export const UPLOAD_FILE_INPUT_ACTION: ChatInputAction = {
    id: "upload-file",
    get menuItem(): MenuItem {
        const backend = useBackendStore();
        return {
            type: "item",
            id: "upload-file",
            label: backend.translate("Upload File"),
            icon: "icon_upload_files",
        };
    },
    run: async ({ selectFile }) => {
        try {
            await selectFile({ multiple: true });
        } catch (error) {
            console.error("Failed to select file:", error);
        }
    },
};

export const SCREENSHOT_INPUT_ACTION: ChatInputAction = {
    id: "screenshot",
    get menuItem(): MenuItem {
        const backend = useBackendStore();
        return {
            type: "item",
            id: "screenshot",
            label: backend.translate("Screenshot Q&A"),
            icon: "icon_screenshot",
        };
    },
    run: async ({ startScreenshot }) => {
        try {
            await startScreenshot();
        } catch (error) {
            console.error("Failed to start screenshot:", error);
        }
    },
};

export const REFERENCE_OUTLINE_INPUT_ACTION: ChatInputAction = {
    id: "reference-outline",
    get menuItem(): MenuItem {
        const backend = useBackendStore();
        return {
            type: "item",
            id: "reference-outline",
            label: backend.translate("Reference Outline"),
            icon: "icon_outline_file",
        };
    },
    run: async ({ selectFile }) => {
        try {
            await selectFile({ category: FileCategory.Outline });
        } catch (error) {
            console.error("Failed to select outline file:", error);
        }
    },
};

export const LOCAL_FILE_INPUT_ACTION: ChatInputAction = {
    id: "local-file",
    get menuItem(): MenuItem {
        const backend = useBackendStore();
        return {
            type: "item",
            id: "local-file",
            label: backend.translate("Local File"),
            icon: "icon_upload_files",
        };
    },
    run: async ({ selectFile }) => {
        try {
            await selectFile({ category: FileCategory.Material, multiple: true });
        } catch (error) {
            console.error("Failed to select local files:", error);
        }
    },
};

export const getInputAreaActionMenuItems = (actions: ChatInputAction[] = []): MenuItem[] => {
    return actions.flatMap((action) => {
        const menuItem = { ...action.menuItem };
        return [menuItem];
    });
};

export const executeInputAreaAction = async (
    actions: ChatInputAction[] = [],
    actionId: string,
    context: ChatInputActionContext,
) => {
    const action = actions.find((currentAction) => currentAction.id === actionId);

    if (!action) {
        console.warn("Unknown input action:", actionId);
        return;
    }

    await action.run?.(context);
};
