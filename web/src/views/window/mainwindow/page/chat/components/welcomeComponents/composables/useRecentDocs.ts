import { ref } from "vue";
import { useBackendStore } from "@/stores";

/**
 * 最近创作的文档记录
 * 文档内容存储于数据库，通过 id 查询，不对应本地文件
 */
export interface RecentDoc {
    id: string; // 文档数据库 ID（article_id），用于查询和打开
    name: string; // 文档标题
    updated_at: string; // 格式化后的时间字符串，如 "昨天 14:30"
    conversation_id: string; // 所属 workspace 的 conversation_id
}

/** 开发模式下的 mock 测试数据 - 按 updated_at 降序排序 */
const DEV_MOCK_DOCS: RecentDoc[] = [
    { id: "mock-001", name: "UOS AI 产品需求文档", updated_at: "今天 10:25", conversation_id: "mock-conv-001" },
    { id: "mock-002", name: "2026 年度技术规划", updated_at: "今天 09:10", conversation_id: "mock-conv-001" },
    { id: "mock-003", name: "写作助手接口设计说明", updated_at: "昨天 17:42", conversation_id: "mock-conv-002" },
    { id: "mock-004", name: "深度学习模型调研报告", updated_at: "昨天 14:30", conversation_id: "mock-conv-002" },
    { id: "mock-005", name: "操作系统内核模块开发笔记", updated_at: "3月8日 11:05", conversation_id: "mock-conv-003" },
    { id: "mock-006", name: "Qt6 组件库迁移方案", updated_at: "3月7日 16:20", conversation_id: "mock-conv-003" },
];

/**
 * 最近创作文档 composable
 * 负责从后端加载最近编辑的 markdown 文档列表
 */
export function useRecentDocs() {
    const backend = useBackendStore();

    const docs = ref<RecentDoc[]>([]);
    const loading = ref(false);

    const load = async () => {
        loading.value = true;
        try {
            const result = await backend.requestAssistant("getRecentWritingDocs");
            const parsed: RecentDoc[] = result ? JSON.parse(result as string) : [];
            // 开发模式下，后端无数据时使用 mock 数据便于调试
            docs.value = parsed.length > 0 ? parsed : import.meta.env.DEV ? DEV_MOCK_DOCS : [];
        } catch (e) {
            console.error("[WritingAssistant] Failed to load recent docs:", e);
            docs.value = import.meta.env.DEV ? DEV_MOCK_DOCS : [];
        } finally {
            loading.value = false;
        }
    };

    return { docs, loading, load };
}
