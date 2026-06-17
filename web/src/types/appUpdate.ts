/** 后端应用更新检查返回的信息 */
export interface AppUpdateInfo {
    packageId?: string;
    updateDescription?: string;
    currentVersion?: string;
    availableVersion?: string;
}
