import { defineComponent, ref, computed, watch } from "vue";
import type { PropType } from "vue";
import type { Conversation, Message as MessageType } from "@/types/conversation";
import { useConversationManagerStore } from "@/stores/conversationmanager";
import type { Assistant } from "@/types/assistant";

import { UserType } from "@/types/conversation";
import Message from "./Message";

export default defineComponent({
    name: "MessageNavigator",
    props: {
        conversation: {
            type: Object as PropType<Conversation>,
            required: true,
        },
        isStreamingLastMessage: {
            type: Boolean,
            default: false,
        },
        isSessionRunning: {
            type: Boolean,
            default: false,
        },
        isOperationCanceled: {
            type: Boolean,
            default: false,
        },
        isFromHistory: {
            type: Boolean,
            default: false,
        },
        currentAssistant: {
            type: Object as PropType<Assistant>,
            default: () => null,
        },
        shouldDisableRetry: {
            type: Boolean,
            default: false,
        },
    },
    emits: {
        messageClick: (message: MessageType) => {
            return message && typeof message === "object";
        },
        // 重试消息事件
        retryMessage: (message: MessageType) => {
            return message && typeof message === "object";
        },
        // 切换到前一个兄弟消息
        switchToPrevious: (parentId: string, currentId: string) => {
            return parentId && currentId;
        },
        // 切换到后一个兄弟消息
        switchToNext: (parentId: string, currentId: string) => {
            return parentId && currentId;
        },
    },
    setup(props, { emit }) {
        // 当前选中的消息ID（用于处理一个问题的多个答案）
        const currentMessageId = ref<string>("");

        // 当前正在播放的消息ID（共享状态）
        const playingMessageId = ref<string | null>(null);

        // 获取消息链中的所有消息ID（按顺序）
        const messageChainIds = computed(() => {
            const ids: string[] = [];
            const conversation = props.conversation;
            if (!conversation) return ids;

            // 从root.cur_next开始遍历
            let currentId = conversation.root.cur_next;
            while (currentId && conversation.messages[currentId]) {
                ids.push(currentId);
                const message = conversation.messages[currentId];
                if (message) {
                    currentId = message.cur_next;
                } else {
                    break;
                }
            }
            return ids;
        });

        // 获取当前消息链中的正在被显示的消息（cur_next链路）
        const messageChain = computed(() => {
            const chain: MessageType[] = [];
            const conversation = props.conversation;
            if (!conversation) return chain;

            // 从root.cur_next开始遍历
            let currentId = conversation.root.cur_next;
            while (currentId && conversation.messages[currentId]) {
                const message = conversation.messages[currentId];
                if (message) {
                    chain.push(message);
                    currentId = message.cur_next;
                } else {
                    break;
                }
            }
            return chain;
        });

        // 判断是否已有用户消息
        const hasUserMessages = computed(() => {
            return messageChain.value.some((msg) => msg.role === UserType.USER);
        });

        // 判断当前是否正在流式输出最后一条消息
        const isStreamingLastMessage = computed(() => {
            if (!props.isStreamingLastMessage) return false;
            if (messageChain.value.length === 0) return false;
            const lastMessage = messageChain.value[messageChain.value.length - 1];
            return lastMessage && lastMessage.role === UserType.ASSISTANT;
        });

        // 获取某个消息的所有兄弟消息（同一个问题的多个答案）
        const getSiblingMessages = (messageId: string): MessageType[] => {
            const conversation = props.conversation;
            if (!conversation) return [];

            const message = conversation.messages[messageId];
            if (!message) return [];

            // 找到所有具有相同previous和cur_next的消息
            const siblings: MessageType[] = [];
            for (const id in conversation.messages) {
                const msg = conversation.messages[id];
                if (msg && msg.previous === message.previous && msg.cur_next === message.cur_next) {
                    siblings.push(msg);
                }
            }
            return siblings;
        };

        // 获取某个消息在兄弟消息中的索引
        const getSiblingIndex = (messageId: string): number => {
            const conversation = props.conversation;
            if (!conversation) return -1;
            const parentMessageId = conversation.messages[messageId]?.previous; // 父节点ID
            if (!parentMessageId) return -1;
            const currentIndex = findChildIds(parentMessageId).indexOf(messageId); // 当前索引
            return currentIndex;
        };

        // 根据父节点查找所有子节点ID
        const findChildIds = (parentId: string): string[] => {
            const conversation = props.conversation;
            if (!conversation) return [];
            const message = conversation.messages[parentId];
            if (!message) return [];
            // 给message.next去重
            return [...new Set(message.next)];
        };

        // 切换后更新父节点的curr_next
        // const updateParentMessageCurrNext = (parentId: string, newCurrNext: string) => {
        //     const conversation = props.conversation;
        //     if (!conversation) return;
        //     const parentMessage = conversation.messages[parentId];
        //     if (parentMessage) {
        //         parentMessage.cur_next = newCurrNext;
        //     }
        // };

        /**
         * 1.切换兄弟消息时，需要更新父节点的curr_next
         * 2.切换完成后，需要通知后端同步修改
         */

        // 切换到兄弟节点
        const switchToSibling = (targetMessageId: string, curNext: string, type: string) => {
            const siblings = findChildIds(targetMessageId);
            const currentIndex = siblings.indexOf(curNext);
            let nextMessageId: string = "";

            if (type === "previous") {
                // 找到前一个兄弟消息
                if (currentIndex > 0) {
                    nextMessageId = siblings[currentIndex - 1] as string;
                }
            } else if (type === "next") {
                // 找到下一个兄弟消息
                if (currentIndex < siblings.length - 1) {
                    nextMessageId = siblings[currentIndex + 1] as string;
                }
            }

            if (nextMessageId === "") {
                return;
            }

            useConversationManagerStore().updateCurrentMessageCurrNext(
                props.conversation.root.id,
                targetMessageId,
                nextMessageId,
            );
        };

        // 处理消息点击
        const handleMessageClick = (message: MessageType) => {
            emit("messageClick", message);
        };

        // 处理消息重试
        const handleRetryMessage = (message: MessageType) => {
            emit("retryMessage", message);
        };

        // 处理消息播放状态变更
        const handlePlayingMessageIdChange = (messageId: string | null) => {
            playingMessageId.value = messageId;
        };

        // 处理切换到前一个兄弟消息
        const handleSwitchToPrevious = (parentId: string, currentId: string) => {
            switchToSibling(parentId, currentId, "previous");
        };

        // 处理切换到后一个兄弟消息
        const handleSwitchToNext = (parentId: string, currentId: string) => {
            switchToSibling(parentId, currentId, "next");
        };

        // 监听conversation变化，重置currentMessageId
        watch(
            () => props.conversation,
            (newConversation) => {
                if (newConversation && newConversation.root.cur_next) {
                    currentMessageId.value = newConversation.root.cur_next;
                }
            },
            { immediate: true },
        );

        return {
            currentMessageId,
            messageChainIds,
            messageChain,
            hasUserMessages,
            isStreamingLastMessage,
            isSessionRunning: computed(() => props.isSessionRunning),
            isFromHistory: computed(() => props.isFromHistory), // 是否从历史记录中加载
            playingMessageId,
            handlePlayingMessageIdChange,
            getSiblingMessages,
            getSiblingIndex,
            switchToSibling,
            findChildIds,
            handleMessageClick,
            handleRetryMessage,
            handleSwitchToPrevious,
            handleSwitchToNext,
            currentAssistant: computed(() => props.currentAssistant),
        };
    },
    render() {
        if (!this.hasUserMessages) {
            return null;
        }

        return (
            <div class="message-navigator">
                {this.messageChain.map((message, index) => {
                    const siblings = this.findChildIds(message.previous || "");
                    const siblingIndex = this.getSiblingIndex(message.id || "");
                    const hasSiblings = siblings.length > 1;
                    const isLastMessage = index === this.messageChain.length - 1;

                    // 当当前消息是 ASSISTANT 时，获取上一个问题消息
                    const previousMessage =
                        message.role === UserType.ASSISTANT &&
                        index > 0 &&
                        this.messageChain[index - 1]?.role === UserType.USER &&
                        this.messageChain[index - 1]?.id === message.previous
                            ? this.messageChain[index - 1]
                            : undefined;
                    return (
                        <div key={message.frontendKey || message.id || index} class="message-navigator__item">
                            {/* 渲染消息 */}
                            <Message
                                message={message}
                                isStreamingLastMessage={this.isStreamingLastMessage && isLastMessage}
                                isLastMessage={isLastMessage}
                                siblingMessage={{
                                    currIndex: siblingIndex,
                                    total: siblings.length,
                                    parentId: message.previous || "",
                                    currentId: message.id || "",
                                }}
                                isSessionRunning={this.isSessionRunning}
                                isOperationCanceled={this.$props.isOperationCanceled}
                                previousMessage={previousMessage}
                                isFromHistory={this.isFromHistory}
                                playingMessageId={this.playingMessageId}
                                onMessageClick={this.handleMessageClick}
                                onRetryMessage={this.handleRetryMessage}
                                onPlayingMessageIdChange={this.handlePlayingMessageIdChange}
                                onSwitchToPrevious={this.handleSwitchToPrevious}
                                onSwitchToNext={this.handleSwitchToNext}
                                currentAssistant={this.currentAssistant || undefined}
                                shouldDisableRetry={this.shouldDisableRetry}
                            />
                        </div>
                    );
                })}
            </div>
        );
    },
});
