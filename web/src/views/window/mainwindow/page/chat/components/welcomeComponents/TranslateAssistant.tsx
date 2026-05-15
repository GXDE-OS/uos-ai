import { defineComponent, ref, onMounted } from "vue";
import { useBackendStore } from "@/stores";
import { useChatInput } from "@/composables/useChatInput";

/**
 * 翻译 FAQ 卡片数据结构
 */
interface TranslationFAQ {
    Question: string;
}

/**
 * 翻译助手 (UOS_AI_TRANSLATION) 的欢迎页个性化组件
 * 居中流式展示提示词卡片，点击后填充到输入框
 */
export default defineComponent({
    name: "TranslateAssistant",

    setup() {
        const backend = useBackendStore();
        const faqList = ref<TranslationFAQ[]>([]);

        // 获取 Input 填充能力
        const { fillInput, focusInput } = useChatInput();

        // 从后端加载翻译 FAQ 数据
        const loadFAQData = async () => {
            try {
                // AssistantType::AI_TRANSLATION = 0x0007
                const response = await backend.requestAssistant("getTranslationFAQ");
                if (response) {
                    const parsed = JSON.parse(String(response));
                    if (Array.isArray(parsed)) {
                        faqList.value = parsed;
                    }
                }
            } catch (error) {
                console.error("[TranslateAssistant] Failed to load FAQ data:", error);
            }
        };

        // 卡片点击：填充提示词到输入框
        const handleCardClick = (faq: TranslationFAQ) => {
            fillInput(faq.Question, "replace");
            focusInput();
        };

        onMounted(() => {
            loadFAQData();
        });

        return {
            faqList,
            handleCardClick,
        };
    },

    render() {
        // 无数据时不渲染
        if (!this.faqList.length) return null;

        return (
            <div class="translate-assistant">
                <div class="translate-assistant__card-container">
                    {this.faqList.map((faq, index) => (
                        <div
                            key={index}
                            class="translate-assistant__card"
                            onClick={() => this.handleCardClick(faq)}
                        >
                            <span class="translate-assistant__card-text">{faq.Question}</span>
                        </div>
                    ))}
                </div>
            </div>
        );
    },
});
