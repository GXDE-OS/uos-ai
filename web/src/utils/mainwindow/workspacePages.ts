import { markRaw, shallowReactive } from "vue";
import type { Component } from "vue";
import type { MainWindowWorkspacePage } from "@/types/mainwindow";
import type { DialogButtonType } from "@/types/dialog";
import { FileCategory } from "@/types/uploadfile";

export interface MainWindowWorkspacePageBackButtonDefinition {
    // 这里保存原始文案键，真正渲染时再做国际化，避免页面定义在注册阶段就提前固化翻译结果。
    text: string;
    fallbackPage?: MainWindowWorkspacePage;
}

export interface MainWindowWorkspacePageEnterContext {
    fromPage: MainWindowWorkspacePage | null;
    toPage: MainWindowWorkspacePage;
}

export interface MainWindowWorkspacePageLeaveContext {
    fromPage: MainWindowWorkspacePage;
    toPage: MainWindowWorkspacePage;
}

export interface MainWindowWorkspaceFileDropContext {
    paths: string[];
}

export interface MainWindowWorkspaceFileDropUploadAction {
    type: "upload";
    category?: FileCategory;
}

export interface MainWindowWorkspaceFileDropCategoryButton {
    text: string;
    type: DialogButtonType;
    category?: FileCategory;
}

export interface MainWindowWorkspaceFileDropCategorySelectAction {
    type: "file-category-select";
    buttons: MainWindowWorkspaceFileDropCategoryButton[];
}

export type MainWindowWorkspaceFileDropAction =
    | MainWindowWorkspaceFileDropUploadAction
    | MainWindowWorkspaceFileDropCategorySelectAction;

export interface MainWindowWorkspacePageDefinition {
    id: MainWindowWorkspacePage;
    component: Component;
    acceptFileDrop?: boolean;
    handleFileDrop?: (
        context: MainWindowWorkspaceFileDropContext,
    ) =>
        | MainWindowWorkspaceFileDropAction
        | null
        | undefined
        | Promise<MainWindowWorkspaceFileDropAction | null | undefined>;
    backButton?: MainWindowWorkspacePageBackButtonDefinition;
    enter?: (context: MainWindowWorkspacePageEnterContext) => void | Promise<void>;
    leave?: (context: MainWindowWorkspacePageLeaveContext) => void | Promise<void>;
}

export interface RegisteredMainWindowWorkspacePageDefinition extends MainWindowWorkspacePageDefinition {
    acceptFileDrop: boolean;
}

const workspacePageRegistry = shallowReactive<Record<string, RegisteredMainWindowWorkspacePageDefinition>>({});

export const registerMainWindowWorkspacePage = (pageDefinition: MainWindowWorkspacePageDefinition) => {
    const normalizedPageDefinition: RegisteredMainWindowWorkspacePageDefinition = {
        ...pageDefinition,
        component: markRaw(pageDefinition.component),
        acceptFileDrop: pageDefinition.acceptFileDrop ?? false,
    };

    workspacePageRegistry[pageDefinition.id] = normalizedPageDefinition;
    return normalizedPageDefinition;
};

export const unregisterMainWindowWorkspacePage = (pageId: MainWindowWorkspacePage) => {
    delete workspacePageRegistry[pageId];
};

export const getMainWindowWorkspacePage = (pageId: MainWindowWorkspacePage) => {
    return workspacePageRegistry[pageId];
};
