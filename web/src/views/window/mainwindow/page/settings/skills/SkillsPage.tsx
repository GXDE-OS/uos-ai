import { computed, defineComponent, onMounted } from "vue";
import ScrollBar from "@/components/ScrollBar";
import { useSkillsStore, useBackendStore, useNotifyStore } from "@/stores";
import ToolManagementList from "@/views/window/mainwindow/page/settings/common/components/ToolManagementList";
import type { SkillItem } from "@/types/skill";
import type { ToolManagementCustomAction } from "@/views/window/mainwindow/page/settings/common/types";

export default defineComponent({
    name: "SkillsPage",

    components: {
        ScrollBar,
        ToolManagementList,
    },

    setup() {
        const skillsStore = useSkillsStore();
        const backendStore = useBackendStore();
        const notifyStore = useNotifyStore();

        // 使用 store 的 toolItems getter，后端已排好序
        const sortedToolItems = computed(() => {
            return skillsStore.toolItems;
        });

        const loadPageData = async () => {
            if (skillsStore.isLoaded) {
                return;
            }
            await skillsStore.loadPageData().catch(() => undefined);
        };

        const handleRefresh = async () => {
            await skillsStore.refreshSkills().catch(() => undefined);
        };

        /**
         * 导入 Skill
         * 调用后端接口弹出文件选择对话框，支持压缩包和 markdown 文件
         */
        const handleImportSkill = async () => {
            try {
                const result = await backendStore.requestSkillsMgr("addSkillForWeb");

                if (result.success) {
                    notifyStore.showToast({
                        type: "success",
                        message: backendStore.translate(
                            "Import successful. The skill's md file and related configuration files have been imported.",
                        ),
                        duration: 1000,
                    });
                    await skillsStore.refreshSkills();
                } else if (result.error) {
                    notifyStore.showToast({
                        type: "error",
                        message: result.error,
                        duration: 3000,
                    });
                }
                // 用户取消时不显示任何提示
            } catch (error) {
                console.error("[SkillsPage] Failed to import skill", error);
                notifyStore.showToast({
                    type: "error",
                    message: backendStore.translate("Import failed. Please try again later."),
                    duration: 3000,
                });
            }
        };

        const handleToggleSkill = async (skillId: string, enabled: boolean) => {
            await skillsStore.toggleSkill(skillId, enabled).catch(() => undefined);
        };

        const handleOpenSkillDirectory = async (skillId: string) => {
            const targetSkill = skillsStore.skills.find((skill: SkillItem) => skill.name === skillId);

            if (!targetSkill || !targetSkill.path) {
                return;
            }

            await backendStore.requestSystem("openFile", targetSkill.path).catch((error) => {
                console.error("[SkillsPage] Failed to open skill directory", error);
                notifyStore.showToast({
                    type: "error",
                    message: backendStore.translate("Failed to open skill directory. Please try again later."),
                    duration: 3000,
                });
            });
        };

        const skillDirectoryAction: ToolManagementCustomAction = {
            icon: "icon_open_dir",
            tooltip: () => backendStore.translate("Open installation directory"),
            visible: (item) => item.editable,
            onClick: (item) => {
                void handleOpenSkillDirectory(item.id);
            },
        };

        const handleDeleteSkill = async (skillId: string) => {
            const targetSkill = skillsStore.skills.find((skill: SkillItem) => skill.name === skillId);

            if (!targetSkill) {
                return;
            }

            const result = await notifyStore.showDialog({
                title: backendStore.translate("Confirm deletion") + " " + targetSkill.name + "?",
                content: backendStore.translate(
                    "After deletion, this skill will be unavailable. Proceed with caution.",
                ),
                buttons: [
                    { key: "cancel", text: backendStore.translate("Cancel"), type: "default" },
                    { key: "confirm", text: backendStore.translate("Delete"), type: "danger" },
                ],
            });

            if (result.key !== "confirm") {
                return;
            }

            const success = await skillsStore.deleteSkill(skillId).catch(() => false);

            if (!success) {
                notifyStore.showToast({
                    type: "error",
                    message: backendStore.translate(
                        "Failed to delete skill. Only skills from uos-ai source can be deleted.",
                    ),
                    duration: 3000,
                });
            }
        };

        onMounted(() => {
            void loadPageData();
        });

        const titleText = computed(() => {
            return backendStore.translate("My Skills");
        });

        const refreshButtonText = computed(() => {
            return backendStore.translate("Refresh");
        });

        const importButtonText = computed(() => {
            return backendStore.translate("Import Skill");
        });

        const loadingText = computed(() => {
            return backendStore.translate("Loading Skills...");
        });

        const emptyText = computed(() => {
            return backendStore.translate("No Skills available.");
        });

        return {
            skillsStore,
            sortedToolItems,
            titleText,
            refreshButtonText,
            importButtonText,
            loadingText,
            emptyText,
            handleRefresh,
            handleImportSkill,
            handleToggleSkill,
            skillDirectoryAction,
            handleDeleteSkill,
        };
    },

    render() {
        return (
            <div class="skills-page">
                <div class="skills-page__header-container">
                    <div class="skills-page__container">
                        <div class="skills-page__header">
                            <div class="skills-page__header-left">
                                <div class="skills-page__header-content">
                                    <div class="skills-page__title">{this.titleText}</div>
                                </div>
                            </div>

                            <div class="skills-page__actions">
                                <button class="skills-page__action-button" onClick={this.handleRefresh} type="button">
                                    <span class="skills-page__action-button-text">{this.refreshButtonText}</span>
                                </button>
                                <button
                                    class="skills-page__action-button"
                                    onClick={this.handleImportSkill}
                                    type="button"
                                >
                                    <span class="skills-page__action-button-text">{this.importButtonText}</span>
                                </button>
                            </div>
                        </div>
                    </div>
                </div>

                <div class="skills-page__content">
                    <ScrollBar class="skills-page__scroll" edgeBounce momentum>
                        <div class="skills-page__content-container">
                            <div class="skills-page__container">
                                <section class="skills-page__section">
                                    <ToolManagementList
                                        isLoading={this.skillsStore.isLoading}
                                        items={this.sortedToolItems}
                                        customAction={this.skillDirectoryAction}
                                        showEditButton={false}
                                        loadingText={this.loadingText}
                                        emptyText={this.emptyText}
                                        onToggleItem={this.handleToggleSkill}
                                        onDeleteItem={this.handleDeleteSkill}
                                    />
                                </section>
                            </div>
                        </div>
                    </ScrollBar>
                </div>
            </div>
        );
    },
});
