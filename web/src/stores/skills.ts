import { defineStore } from "pinia";

import { useBackendStore } from "./backend";
import type { SkillItem } from "@/types/skill";
import { convertSkillToToolItem, type ToolManagementItem } from "@/views/window/mainwindow/page/settings/common/types";

const getBackendStore = () => {
    const backend = useBackendStore();

    if (!backend.skillsMgr) {
        throw new Error("Skills manager is not available");
    }

    return backend;
};

export const useSkillsStore = defineStore("skills", {
    state: () => ({
        skills: [] as SkillItem[],
        isLoading: false,
        isLoaded: false,
    }),

    getters: {
        /** 获取可删除的自定义技能列表 */
        customSkills: (state) => {
            return state.skills.filter((skill) => skill.source === "uos-ai");
        },

        /** 获取转换后的工具管理列表项 */
        toolItems: (state): ToolManagementItem[] => {
            return state.skills.map(convertSkillToToolItem);
        },
    },

    actions: {
        /**
         * 从后端响应同步技能数据
         */
        syncSkillsData(skillsData: SkillItem[]) {
            this.skills = skillsData || [];
        },

        /**
         * 加载技能列表数据
         */
        async loadPageData() {
            if (this.isLoaded) {
                return;
            }

            this.isLoading = true;

            try {
                const backend = getBackendStore();
                const skillsData = await backend.requestSkillsMgr("skillsData");
                this.syncSkillsData(skillsData as SkillItem[]);
                this.isLoaded = true;
            } catch (error) {
                console.error("[skillsStore] Failed to load Skills page data", error);
                throw error;
            } finally {
                this.isLoading = false;
            }
        },

        /**
         * 刷新技能列表
         */
        async refreshSkills() {
            this.isLoading = true;

            try {
                const backend = getBackendStore();
                // 先调用后端重新加载
                await backend.requestSkillsMgr("reloadSkills");
                // 再获取最新数据
                const skillsData = await backend.requestSkillsMgr("skillsData");
                this.syncSkillsData(skillsData as SkillItem[]);
            } catch (error) {
                console.error("[skillsStore] Failed to refresh Skills", error);
                throw error;
            } finally {
                this.isLoading = false;
            }
        },

        /**
         * 切换技能启用状态
         * @param skillName 技能名称
         * @param enabled 启用状态
         */
        async toggleSkill(skillName: string, enabled: boolean) {
            try {
                const backend = getBackendStore();
                const success = await backend.requestSkillsMgr("setSkillEnabled", skillName, enabled);

                if (success) {
                    // 更新本地状态
                    const skill = this.skills.find((s) => s.name === skillName);
                    if (skill) {
                        skill.enabled = enabled;
                    }
                }
            } catch (error) {
                console.error(`[skillsStore] Failed to toggle skill "${skillName}"`, error);
                throw error;
            }
        },

        /**
         * 删除技能
         * 仅支持删除 source 为 uos-ai 的技能
         * @param skillName 技能名称
         */
        async deleteSkill(skillName: string) {
            try {
                const backend = getBackendStore();
                const success = await backend.requestSkillsMgr("removeSkill", skillName);

                if (success) {
                    // 从本地列表移除
                    this.skills = this.skills.filter((s) => s.name !== skillName);
                }

                return success;
            } catch (error) {
                console.error(`[skillsStore] Failed to delete skill "${skillName}"`, error);
                throw error;
            }
        },

        /**
         * 检查技能是否存在
         * @param skillName 技能名称
         */
        async hasSkill(skillName: string): Promise<boolean> {
            try {
                const backend = getBackendStore();
                return await backend.requestSkillsMgr("hasSkill", skillName);
            } catch (error) {
                console.error(`[skillsStore] Failed to check skill "${skillName}"`, error);
                return false;
            }
        },
    },
});
