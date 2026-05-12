import { ref } from "vue";
import { useBackendStore } from "@/stores";

/**
 * 模板分类枚举
 */
export enum TemplateCategory {
    ALL = "all",
    OFFICIAL = "official",
    PROMOTION = "promotion",
    GENERAL = "general",
    INDUSTRY = "industry",
    TECH = "tech",
}

/**
 * Tab 标签配置
 */
export interface TabConfig {
    key: string;
    label: string;
}

/** Tab 列表配置 */
export const TEMPLATE_TABS: TabConfig[] = [
    { key: TemplateCategory.ALL, label: "全部" },
    { key: TemplateCategory.OFFICIAL, label: "办公公文" },
    { key: TemplateCategory.PROMOTION, label: "宣传推广" },
    { key: TemplateCategory.GENERAL, label: "通用辅助" },
    { key: TemplateCategory.INDUSTRY, label: "行业专项" },
    { key: TemplateCategory.TECH, label: "技术文档" },
];

/**
 * 写作模板记录
 * 从后端 API 获取，点击后填充内容到输入框
 */
export interface WritingTemplate {
    id: string;
    name: string;
    description: string;
    icon: string;
    category: string;
    content: string;
}

/** 开发模式下的 mock 测试数据 */
const DEV_MOCK_TEMPLATES: WritingTemplate[] = [
    {
        id: "tpl-001",
        name: "通知公告",
        description: "适用于发布官方通知、公告等",
        icon: "document",
        category: TemplateCategory.OFFICIAL,
        content: "<uos-ai-prompt-template>请帮我生成[[本周|本月]]的{{技术周报}}</uos-ai-prompt-template>",
    },
    {
        id: "tpl-002",
        name: "会议纪要",
        description: "记录会议核心内容与决议事项",
        icon: "document",
        category: TemplateCategory.OFFICIAL,
        content: "<uos-ai-prompt-template>[[本周|本月|本年度|本年度|本年度|本年度]]的{{财务报告}}</uos-ai-prompt-template>",
    },
    {
        id: "tpl-003",
        name: "产品宣传",
        description: "产品推广活动文案模板",
        icon: "star",
        category: TemplateCategory.PROMOTION,
        content: "<uos-ai-prompt-template>[[本周|本月|本年度]]的{{财务报告}},并提交到。</uos-ai-prompt-template>",
    },
    {
        id: "tpl-004",
        name: "活动策划",
        description: "线下活动策划方案框架",
        icon: "calendar",
        category: TemplateCategory.PROMOTION,
        content: "# 活动策划书\n\n## 活动概述\n活动时间：\n活动地点：\n参与人数：",
    },
    {
        id: "tpl-005",
        name: "工作周报",
        description: "周度工作总结与计划模板",
        icon: "writing",
        category: TemplateCategory.GENERAL,
        content: "# 工作周报\n\n## 本周工作\n1. \n2. \n\n## 下周计划\n1. \n2. ",
    },
    {
        id: "tpl-006",
        name: "项目管理",
        description: "项目进度跟踪与风险管理",
        icon: "project",
        category: TemplateCategory.INDUSTRY,
        content: "# 项目管理报告\n\n## 项目状态\n\n## 风险与问题\n\n## 下一步行动",
    },
    {
        id: "tpl-007",
        name: "技术文档",
        description: "API/系统技术文档模板",
        icon: "code",
        category: TemplateCategory.TECH,
        content: "# 技术文档\n\n## 概述\n\n## 接口说明\n\n## 示例代码\n\n```javascript\n```",
    },
    {
        id: "tpl-008",
        name: "需求分析",
        description: "产品需求文档 PRD 模板",
        icon: "edit",
        category: TemplateCategory.TECH,
        content: "# 需求文档 (PRD)\n\n## 背景与目标\n\n## 功能需求\n\n## 非功能需求",
    },
];

/**
 * 写作模板 composable
 * 负责从后端加载写作模板列表
 */
export function useWritingTemplates() {
    const backend = useBackendStore();

    const templates = ref<WritingTemplate[]>([]);
    const loading = ref(false);

    const load = async () => {
        loading.value = true;
        try {
            const result = await backend.requestAssistant("getWritingTemplates");
            const parsed: WritingTemplate[] = result ? JSON.parse(result as string) : [];
            // 开发模式下，后端无数据时使用 mock 数据便于调试
            templates.value = parsed.length > 0 ? parsed : import.meta.env.DEV ? DEV_MOCK_TEMPLATES : [];
        } catch (e) {
            console.error("[WritingAssistant] Failed to load writing templates:", e);
            templates.value = import.meta.env.DEV ? DEV_MOCK_TEMPLATES : [];
        } finally {
            loading.value = false;
        }
    };

    /** 根据分类过滤模板 */
    const filterByCategory = (category: string): WritingTemplate[] => {
        if (category === TemplateCategory.ALL) {
            return templates.value;
        }
        return templates.value.filter((t) => t.category === category);
    };

    return { templates, loading, load, filterByCategory };
}
