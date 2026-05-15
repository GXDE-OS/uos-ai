/**
 * Skill 项接口
 * 与后端 SkillsManager::skillsData() 返回的 JSON 结构对应
 */
export interface SkillItem {
    /** 技能名称 */
    name: string;
    /** 技能描述 */
    description: string;
    /** 技能所在目录路径 */
    path: string;
    /** 技能来源（builtin/uos-ai/claude/vscode/...） */
    source: string;
    /** 是否启用 */
    enabled: boolean;
}
