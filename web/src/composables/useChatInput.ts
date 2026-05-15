import { inject } from 'vue';
import { CHAT_INPUT_KEY, type ChatInputContext } from '@/types/chat-input';

/**
 * 使用 ChatView 提供的输入框能力
 *
 * @throws {Error} 如果在 ChatView 之外调用会抛出错误
 * @example
 * ```typescript
 * const { fillInput, focusInput } = useChatInput();
 * fillInput("这是模板内容", "replace");
 * focusInput();  // 自动聚焦输入框
 * ```
 */
export function useChatInput(): ChatInputContext {
    const chatInput = inject(CHAT_INPUT_KEY);

    if (!chatInput) {
        throw new Error('[useChatInput] Must be used within ChatView component tree');
    }

    return chatInput;
}
