import { defineComponent, ref } from "vue";
import ConversationItem from "./ConversationItem";
import type { GroupedConversations } from "@/types/historyconversation";
import "@/assets/styles/window/mainwindow/page/historyconversation/components/TimeGroup.css";

export default defineComponent({
    name: "TimeGroup",
    props: {
        group: {
            type: Object as () => GroupedConversations,
            required: true,
        },
        isBatchMode: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        delete: (conversationId: string) => true,
    },
    setup() {
        const groupTitleRef = ref<HTMLElement | null>(null);

        return {
            groupTitleRef,
        };
    },
    render() {
        return (
            <div class="time-group">
                <div ref="groupTitleRef" class="time-group__title">
                    {this.group.label}
                </div>
                <div class="time-group__list">
                    {this.group.list.map((conversation) => (
                        <ConversationItem
                            key={conversation.id}
                            conversation={conversation}
                            isBatchMode={this.isBatchMode}
                            onDelete={(conversationId: string) => this.$emit("delete", conversationId)}
                        />
                    ))}
                </div>
            </div>
        );
    },
});
