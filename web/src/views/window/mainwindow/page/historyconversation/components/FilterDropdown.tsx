import { defineComponent, computed } from "vue";
import type { AssistantFilterTag } from "@/types/historyconversation";
import type { ComboboxOption } from "@/types/combobox";
import { useHistoryConversationStore } from "@/stores";
import ComboBox from "@/components/combobox/ComboBox";
import "@/assets/styles/window/mainwindow/page/historyconversation/components/FilterDropdown.css";

export default defineComponent({
    name: "FilterDropdown",
    setup() {
        const historyConversationStore = useHistoryConversationStore();

        const tags = computed(() => historyConversationStore.assistantFilterTags);

        const comboboxOptions = computed<ComboboxOption[]>(() => {
            return [
                ...tags.value.map((tag: AssistantFilterTag) => ({
                    value: tag.id,
                    label: tag.name,
                })),
            ];
        });

        // 从 store 中获取当前选中的值
        const selectedValue = computed(() => {
            const selectedAssistants = historyConversationStore.filterCondition.selectedAssistants;
            // 如果选中了"all"或者选中了所有标签，显示"all"
            if (selectedAssistants.includes("all")) {
                return "all";
            }
            // 否则返回第一个选中的非"all"标签
            return selectedAssistants.find((id) => id !== "all") || "all";
        });

        const handleSelect = (option: ComboboxOption) => {
            historyConversationStore.toggleAssistantTag(String(option.value));
        };

        return {
            comboboxOptions,
            selectedValue,
            handleSelect,
        };
    },
    render() {
        return (
            <ComboBox
                options={this.comboboxOptions}
                value={this.selectedValue}
                customClass="filter-dropdown"
                paddingX={8}
                onClickOption={this.handleSelect}
            />
        );
    },
});
