import { computed, type ComputedRef } from "vue";
import type { ConversationIndex } from "@/types/conversation";
import type { GroupedConversations, TimeGroupType } from "@/types/historyconversation";
import { isToday, isYesterday, isPast7Days, isPast30Days, isThisYear, isPast5Years } from "@/utils/date";
import { useBackendStore } from "@/stores";

/**
 * 时间分组组合式函数
 * @param conversations 原始历史会话列表（响应式）
 * @returns 按时间分组后的会话列表
 */
export function useTimeGroup(conversations: ComputedRef<ConversationIndex[]>) {
    const groupedConversations = computed((): GroupedConversations[] => {
        const groups: Map<TimeGroupType, ConversationIndex[]> = new Map();

        // 初始化所有分组
        groups.set("today", []);
        groups.set("yesterday", []);
        groups.set("past7", []);
        groups.set("past30", []);
        // 细分月份为1-11月（12月会落入yesterday/past7/past30）
        for (let i = 0; i < 11; i++) {
            groups.set(`month${i + 1}` as TimeGroupType, []);
        }
        groups.set("year", []);
        groups.set("earlier", []);

        // 遍历会话列表，按时间分组
        conversations.value.forEach((conversation) => {
            const timestamp = conversation.updated_at;
            const date = new Date(timestamp);

            if (isToday(timestamp)) {
                groups.get("today")!.push(conversation);
            } else if (isYesterday(timestamp)) {
                groups.get("yesterday")!.push(conversation);
            } else if (isPast7Days(timestamp)) {
                groups.get("past7")!.push(conversation);
            } else if (isPast30Days(timestamp)) {
                groups.get("past30")!.push(conversation);
            } else if (isThisYear(timestamp)) {
                // 按月份细分1-11月
                const month = date.getMonth(); // 0-11 (1-12月)
                if (month >= 0 && month <= 10) {
                    groups.get(`month${month + 1}` as TimeGroupType)!.push(conversation);
                }
            } else if (isPast5Years(timestamp)) {
                groups.get("year")!.push(conversation);
            } else {
                groups.get("earlier")!.push(conversation);
            }
        });

        // 将分组转换为数组并按时间从新到旧排序
        const result: GroupedConversations[] = [];
        const baseGroupOrder: TimeGroupType[] = ["today", "yesterday", "past7", "past30"];

        baseGroupOrder.forEach((type) => {
            const list = groups.get(type)!;
            if (list.length > 0) {
                list.sort((a, b) => b.updated_at - a.updated_at);
                const label = getGroupLabel(type, list);
                result.push({ type, label, list });
            }
        });

        // 添加月份分组 (1-11月，按月份从新到旧，即11月到1月)
        for (let i = 10; i >= 0; i--) {
            const monthGroup = `month${i + 1}` as TimeGroupType;
            const list = groups.get(monthGroup)!;
            if (list.length > 0) {
                list.sort((a, b) => b.updated_at - a.updated_at);
                const label = getGroupLabel(monthGroup, list);
                result.push({ type: monthGroup, label, list });
            }
        }

        // 添加年份和更早的分组
        ["year", "earlier"].forEach((type) => {
            const monthGroup = type as TimeGroupType;
            const list = groups.get(monthGroup)!;
            if (list.length > 0) {
                list.sort((a, b) => b.updated_at - a.updated_at);
                const label = getGroupLabel(monthGroup, list);
                result.push({ type: monthGroup, label, list });
            }
        });

        return result;
    });

    return {
        groupedConversations,
    };
}

/**
 * 获取分组标签
 * @param type 分组类型
 * @param list 该分组的会话列表
 * @returns 分组标签
 */
function getGroupLabel(type: TimeGroupType, list: ConversationIndex[]): string {
    const backendStore = useBackendStore();

    switch (type) {
        case "today":
            return backendStore.translate("Today");
        case "yesterday":
            return backendStore.translate("Yesterday");
        case "past7":
            return backendStore.translate("Last 7 Days");
        case "past30":
            return backendStore.translate("Last 30 Days"); // 显示最近30天
        case "month1":
        case "month2":
        case "month3":
        case "month4":
        case "month5":
        case "month6":
        case "month7":
        case "month8":
        case "month9":
        case "month10":
        case "month11": {
            const monthNum = parseInt(type.replace("month", ""));
            const monthNames = [
                "January",
                "February",
                "March",
                "April",
                "May",
                "June",
                "July",
                "August",
                "September",
                "October",
                "November",
            ];
            return backendStore.translate(monthNames[monthNum - 1] as string); // 显示1月-11月的月份名称
        }
        case "year":
            // 获取该分组中最新会话的年份 (%1年)
            if (list.length > 0) {
                const latest = list[0];
                if (latest) {
                    const date = new Date(latest.updated_at);
                    const year = date.getFullYear();
                    return year + backendStore.translate("year").replace("year", ""); // 英文只显示年份，中文显示年份+年
                }
            }
        case "earlier":
            return backendStore.translate("Earlier");
        default:
            return "";
    }
}
