import { defineComponent, computed, onMounted, ref } from "vue";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";
import { useMainWindowStore, useBackendStore, useAssistantInfosStore, useMcpServicesStore } from "@/stores";
import McpServiceAgreementDialog from "@/views/window/mainwindow/page/settings/mcpservices/components/McpServiceAgreementDialog";
import { useChatInput } from "@/composables/useChatInput";

/**
 * FAQ 卡片数据结构
 */
interface FAQItem {
    Question: string;
}

/**
 * MCP & Skills 助手 (UOS_AI_MCP_AND_SKILL) 的欢迎页个性化组件
 * 显示 MCP 服务和 Skills 设置入口按钮以及推荐列表
 */
export default defineComponent({
    name: "McpAndSkillsAssistant",

    components: {
        McpServiceAgreementDialog,
    },

    setup() {
        const mainWindowStore = useMainWindowStore();
        const backendStore = useBackendStore();
        const assistantInfosStore = useAssistantInfosStore();
        const mcpServicesStore = useMcpServicesStore();
        const agreementDialogVisible = ref(false);
        const navigateToMcpServicesAfterAgreement = ref(false);
        const faqList = ref<FAQItem[]>([]);

        // 获取 Input 填充能力
        const { fillInput, focusInput } = useChatInput();

        // 从后端加载 FAQ 数据
        const loadFAQData = async () => {
            try {
                const response = await backendStore.requestAssistant("getClawFAQ");
                if (response) {
                    const parsed = JSON.parse(String(response));
                    if (Array.isArray(parsed)) {
                        faqList.value = parsed;
                    }
                }
            } catch (error) {
                console.error("[McpAndSkillsAssistant] Failed to load FAQ data:", error);
            }
        };

        const handleMCPServicesClick = () => {
            mainWindowStore.openMcpServicesPage();
        };

        const handleSkillsClick = () => {
            mainWindowStore.openSkillsPage();
        };

        const openAgreementDialog = (navigateAfterAgreement = false) => {
            navigateToMcpServicesAfterAgreement.value = navigateAfterAgreement;
            agreementDialogVisible.value = true;
        };

        const isDisabled = computed(() => {
            const currentAssistant = assistantInfosStore.getCurrentAssistant;
            return currentAssistant?.envExists === false;
        });

        const ensureThirdPartyAgreementLoaded = async () => {
            if (!mcpServicesStore.hasLoadedThirdPartyAgreement) {
                await mcpServicesStore.loadThirdPartyAgreement().catch(() => undefined);
            }

            return mcpServicesStore.thirdPartyAgreementAccepted;
        };

        const handleMCPButtonClick = async () => {
            if (isDisabled.value) {
                return;
            }

            const agreed = await ensureThirdPartyAgreementLoaded();

            if (!agreed) {
                openAgreementDialog(true);
                return;
            }

            handleMCPServicesClick();
        };

        const handleSkillsButtonClick = () => {
            if (isDisabled.value) {
                return;
            }
            handleSkillsClick();
        };

        const handleAgreementDialogCancel = () => {
            agreementDialogVisible.value = false;
            navigateToMcpServicesAfterAgreement.value = false;
        };

        const handleAgreementDialogConfirm = async () => {
            const shouldNavigateToMcpServices = navigateToMcpServicesAfterAgreement.value;

            try {
                await mcpServicesStore.acceptThirdPartyAgreement();
            } catch (_error) {
                return;
            }

            agreementDialogVisible.value = false;
            navigateToMcpServicesAfterAgreement.value = false;

            if (shouldNavigateToMcpServices) {
                handleMCPServicesClick();
            }
        };

        // 卡片点击：填充提示词到输入框
        const handleCardClick = (faq: FAQItem) => {
            fillInput(faq.Question, "replace");
            focusInput();
        };

        onMounted(() => {
            loadFAQData();

            if (isDisabled.value) {
                return;
            }

            void (async () => {
                const agreed = await ensureThirdPartyAgreementLoaded();

                if (!agreed) {
                    openAgreementDialog(false);
                }
            })();
        });

        const mcpButtonTitleText = backendStore.translate("My MCP Server");
        const mcpButtonDescText = backendStore.translate("You can add and manage MCP servers");
        const skillsButtonTitleText = backendStore.translate("My Skills");
        const skillsButtonDescText = backendStore.translate("You can add and manage Skills");

        return {
            handleMCPServicesClick,
            handleSkillsClick,
            handleMCPButtonClick,
            handleSkillsButtonClick,
            mcpButtonTitleText,
            mcpButtonDescText,
            skillsButtonTitleText,
            skillsButtonDescText,
            isDisabled,
            agreementDialogVisible,
            handleAgreementDialogCancel,
            handleAgreementDialogConfirm,
            faqList,
            handleCardClick,
        };
    },

    render() {
        return (
            <div class="mcp-skills-assistant">
                {/* MCP 服务入口按钮 */}
                <Tooltip
                    content={`${this.mcpButtonTitleText}<br/>${this.mcpButtonDescText}`}
                    placement="top"
                    showAfter={500}
                    isRawContent={true}
                    disabled={this.isDisabled}
                >
                    <div
                        class={[
                            "mcp-skills-assistant__button",
                            this.isDisabled && "mcp-skills-assistant__button--disabled",
                        ]}
                        onClick={this.handleMCPButtonClick}
                    >
                        <div class="mcp-skills-assistant__icon">
                            <SvgIcon icon="icon_my_mcp" size={[32, 32]} />
                        </div>
                        <div class="mcp-skills-assistant__content">
                            <span class="mcp-skills-assistant__title">{this.mcpButtonTitleText}</span>
                            <span class="mcp-skills-assistant__desc">{this.mcpButtonDescText}</span>
                        </div>
                        <div class="mcp-skills-assistant__arrow">
                            <SvgIcon icon="icon_arrow" size={[16, 16]} />
                        </div>
                    </div>
                </Tooltip>

                {/* Skills 入口按钮 */}
                <Tooltip
                    content={`${this.skillsButtonTitleText}<br/>${this.skillsButtonDescText}`}
                    placement="top"
                    showAfter={500}
                    isRawContent={true}
                    disabled={this.isDisabled}
                >
                    <div
                        class={[
                            "mcp-skills-assistant__button",
                            this.isDisabled && "mcp-skills-assistant__button--disabled",
                        ]}
                        onClick={this.handleSkillsButtonClick}
                    >
                        <div class="mcp-skills-assistant__icon">
                            <SvgIcon icon="icon_my_skills" size={[32, 32]} />
                        </div>
                        <div class="mcp-skills-assistant__content">
                            <span class="mcp-skills-assistant__title">{this.skillsButtonTitleText}</span>
                            <span class="mcp-skills-assistant__desc">{this.skillsButtonDescText}</span>
                        </div>
                        <div class="mcp-skills-assistant__arrow">
                            <SvgIcon icon="icon_arrow" size={[16, 16]} />
                        </div>
                    </div>
                </Tooltip>

                {/* FAQ 推荐列表 */}
                {this.faqList.length > 0 && (
                    <div class="mcp-skills-assistant__faq">
                        <div class="mcp-skills-assistant__faq-container">
                            {this.faqList.map((faq, index) => (
                                <div
                                    key={index}
                                    class="mcp-skills-assistant__faq-card"
                                    onClick={() => this.handleCardClick(faq)}
                                >
                                    <span class="mcp-skills-assistant__faq-text">{faq.Question}</span>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                <McpServiceAgreementDialog
                    visible={this.agreementDialogVisible}
                    onCancel={this.handleAgreementDialogCancel}
                    onConfirm={this.handleAgreementDialogConfirm}
                />
            </div>
        );
    },
});
