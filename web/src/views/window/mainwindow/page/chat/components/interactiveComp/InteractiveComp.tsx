import { defineComponent, PropType, watch } from "vue";
import type { BashApproveData, FileChangeApproveData, InteractiveCompType } from "@/types/conversation";
import { InteractiveCompStatus } from "@/types/conversation";
import { useBackendStore, useConversationManagerStore } from "@/stores";
import BashApprove from "./BashApprove";
import FileChangeApprove from "./FileChangeApprove";

export default defineComponent({
    name: "InteractiveComp",

    components: {
        BashApprove,
        FileChangeApprove,
    },

    props: {
        data: {
            type: Object as PropType<BashApproveData | FileChangeApproveData>,
            required: true,
        },
        sessionId: {
            type: String,
            required: true,
        },
        messageId: {
            type: String,
            required: true,
        },
        isLastMessage: {
            type: Boolean,
            default: false,
        },
        isSessionCanceled: {
            type: Boolean,
            default: false,
        },
    },

    setup(props) {
        const handleSubmit = (action: Record<string, unknown>) => {
            const approved = typeof action.approved === "boolean" ? action.approved : action.approve === true;
            const status = approved ? InteractiveCompStatus.APPROVED : InteractiveCompStatus.REJECTED;
            const conversationManagerStore = useConversationManagerStore();
            const conversationId = conversationManagerStore.currentConversationId;
            const conversationRecord = conversationManagerStore.getConversationById(conversationId);
            conversationRecord?.updateICompStatus(props.messageId, props.data.id, status);
            const alwaysApprove = action.always_approve === true;
            alwaysApprove && conversationRecord?.updateAlwaysApprove(alwaysApprove); // 只有用户主动设置 always_approve 才更新

            const backendStore = useBackendStore();
            backendStore.requestSession("invokeAction", props.sessionId, JSON.stringify(action));
        };

        watch(
            () => props.isSessionCanceled,
            (newVal, oldVal) => {
                if (!oldVal && newVal && props.isLastMessage) {
                    if (props.data.status === InteractiveCompStatus.PENDING) {
                        const conversationManagerStore = useConversationManagerStore();
                        const conversationId = conversationManagerStore.currentConversationId;
                        const conversationRecord = conversationManagerStore.getConversationById(conversationId);
                        conversationRecord?.updateICompStatus(
                            props.messageId,
                            props.data.id,
                            InteractiveCompStatus.CANCELED,
                        );
                    }
                }
            },
        );

        return {
            handleSubmit,
        };
    },

    render() {
        const { data } = this.$props;
        const icType = (data as any).ic_type as InteractiveCompType;

        if (icType === "bash_approve") {
            return <BashApprove data={data as BashApproveData} onSubmit={this.handleSubmit} />;
        } else if (icType === "file_change_approve") {
            return <FileChangeApprove data={data as FileChangeApproveData} onSubmit={this.handleSubmit} />;
        }

        return null;
    },
});
