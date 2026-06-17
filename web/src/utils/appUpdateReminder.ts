import type { DialogButton } from "@/types/dialog";

export interface AppUpdateReminderInfoLike {
    packageId?: string;
    updateDescription?: string;
}

export const APP_UPDATE_STORE_PACKAGE_ID = "uos-ai";
export const APP_UPDATE_REMINDER_VIEW_FEATURES_ACTION = "view_features";
export const APP_UPDATE_UPGRADE_ACTION = "upgrade_now";
export const APP_UPDATE_CANCEL_ACTION = "cancel";

export function getAppUpdateStorePackageId(info?: AppUpdateReminderInfoLike | null): string {
    const packageId = info?.packageId?.trim();
    return packageId || APP_UPDATE_STORE_PACKAGE_ID;
}

export function getAppUpdateFeatureDialogButtons(translate: (key: string) => string): DialogButton[] {
    return [
        {
            key: APP_UPDATE_CANCEL_ACTION,
            text: translate("Later"),
            type: "default",
        },
        {
            key: APP_UPDATE_UPGRADE_ACTION,
            text: translate("Upgrade now"),
            type: "primary",
            suggested: true,
        },
    ];
}
