import { defineComponent, computed, ref, onMounted, onUnmounted, watch } from "vue";
import SvgIcon from "@/components/SvgIcon";
import { getWelcomeComponent } from "./welcomeComponents";
import { useCurrentConversationPresentation } from "@/composables/useCurrentConversationPresentation";
import { useTitleBarState } from "@/composables/useTitleBarState";

export default defineComponent({
    name: "WelcomeView",

    props: {
        // 无需 props，数据直接从 store 获取
    },

    emits: {
        // 无需要自定义事件
    },

    setup() {
        const { currentConversationPresentation } = useCurrentConversationPresentation();
        const assistantInfo = computed(() => currentConversationPresentation.value.header);
        const welcomeConfig = computed(() => currentConversationPresentation.value.welcome);
        const titleText = computed(() =>
            welcomeConfig.value.titleSource === "slogan" ? assistantInfo.value.slogan || "" : assistantInfo.value.name || "",
        );
        const shouldShowAvatar = computed(() => welcomeConfig.value.showIcon);
        const shouldShowSlogan = computed(() => welcomeConfig.value.showSlogan && !!assistantInfo.value.slogan);

        // 根据助手 ID 动态获取对应的个性化组件
        const getComponent = () => getWelcomeComponent(currentConversationPresentation.value.assistantId || "");

        // 标题栏状态——监测 welcome-view__title-row 的可见比例，驱动标题栏透明度
        const { titleBarOpacity, assistantName } = useTitleBarState();
        const titleRowRef = ref<HTMLElement | null>(null);
        let intersectionObserver: IntersectionObserver | null = null;
        // 生成 0.00, 0.01, 0.02 ... 1.00 共 101 个阈值，实现逐像素平滑跟踪
        const THRESHOLDS = Array.from({ length: 101 }, (_, i) => i / 100);

        // 保持标题栏助手名称与当前助手同步
        watch(
            () => assistantInfo.value.name,
            (name) => {
                assistantName.value = name || "";
            },
            { immediate: true },
        );

        onMounted(() => {
            if (!titleRowRef.value) return;
            // WelcomeView 在 ScrollBar 自定义容器（.scroll-bar-content）内滚动，
            // 必须将其作为 IntersectionObserver 的 root，否则检测的是 viewport，永远不触发
            const scrollRoot = titleRowRef.value.closest<HTMLElement>(".scroll-bar-content") ?? null;
            intersectionObserver = new IntersectionObserver(
                ([entry]) => {
                    if (!entry) return;
                    // 助手标题遮挡比例 = 1 - 可见比例，作为标题栏透明度
                    titleBarOpacity.value = 1 - entry.intersectionRatio;
                },
                { root: scrollRoot, threshold: THRESHOLDS },
            );
            intersectionObserver.observe(titleRowRef.value);
        });

        onUnmounted(() => {
            intersectionObserver?.disconnect();
            intersectionObserver = null;
            // 卸载时重置状态，避免残留显示
            titleBarOpacity.value = 0;
        });

        return {
            assistantInfo,
            titleText,
            shouldShowAvatar,
            shouldShowSlogan,
            getComponent,
            currentConversationPresentation,
            titleRowRef,
        };
    },

    render() {
        const WelcomeComponent = this.getComponent();
        // 判断是否是三方助手
        const isThirdPartyAssistant = this.assistantInfo.icon?.startsWith("file://");
        return (
            <div class="welcome-view">
                {/* 上半部：第一排图标+名称，第二排标语 */}
                <div class="welcome-view__header">
                    <div class="welcome-view__title-row" ref="titleRowRef">
                        {this.shouldShowAvatar &&
                            (!isThirdPartyAssistant ? (
                                <div class="welcome-view__avatar">
                                    <SvgIcon
                                        icon={this.currentConversationPresentation.header.icon || ""}
                                        size={this.currentConversationPresentation.header.iconSize}
                                    />
                                </div>
                            ) : (
                                <img
                                    src={this.currentConversationPresentation.header.icon || ""}
                                    class="welcome-view__avatar"
                                />
                            ))}
                        <div
                            class={[
                                "welcome-view__name",
                                {
                                    "welcome-view__name--without-avatar": !this.shouldShowAvatar,
                                },
                            ]}
                        >
                            {this.titleText}
                        </div>
                    </div>
                    {this.shouldShowSlogan && (
                        <div class="welcome-view__slogan">
                            {this.currentConversationPresentation.header.slogan || ""}
                        </div>
                    )}
                </div>

                {/* 下半部：根据助手 ID 自动加载对应的个性化组件 */}
                {this.currentConversationPresentation.welcome.showContent && (
                    <div class="welcome-view__content">{WelcomeComponent ? <WelcomeComponent /> : null}</div>
                )}
            </div>
        );
    },
});
