import type { Message, RenderMessageItem, Root } from "@/types/conversation";
import { InteractiveCompStatus } from "@/types/conversation";
import { ContentType } from "@/types/message";

type ApprovalCandidateData = {
    ic_type?: unknown;
    status?: unknown;
};

const isApprovalComponentType = (icType: unknown): boolean =>
    icType === "bash_approve" || icType === "file_change_approve";

export const isPendingApprovalRenderItem = (item: RenderMessageItem): boolean => {
    if (item.type !== ContentType.CntIComps) {
        return false;
    }

    const data = item.data as ApprovalCandidateData;
    return isApprovalComponentType(data.ic_type) && data.status === InteractiveCompStatus.PENDING;
};

const hasPendingApprovalInMessage = (message: Pick<Message, "render_message">): boolean =>
    message.render_message.some(isPendingApprovalRenderItem);

export const hasPendingApprovalInConversation = (
    root: Pick<Root, "cur_next"> | null | undefined,
    messages: Record<string, Pick<Message, "cur_next" | "render_message">>,
): boolean => {
    const visited = new Set<string>();
    let currentId = root?.cur_next ?? "";

    while (currentId && !visited.has(currentId)) {
        const message = messages[currentId];
        if (!message) {
            return false;
        }

        if (hasPendingApprovalInMessage(message)) {
            return true;
        }

        visited.add(currentId);
        currentId = message.cur_next;
    }

    return false;
};
