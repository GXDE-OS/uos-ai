import { defineComponent, PropType, ref } from "vue";
import SvgIcon from "@/components/SvgIcon";
import type { AppStoreCardData } from "@/types/conversation";
import { useBackendStore } from "@/stores";
import { useThemeIcon } from "@/utils/loadThemeIcon";
import CardBase from "./CardBase";
import "./CardBase.css";
import "./CommandCard.css";

/**
 * 应用商店卡片组件
 * 用于显示应用商店搜索结果，支持最多3个应用的展示
 */
export default defineComponent({
    name: "AppStoreCard",

    components: {
        SvgIcon,
        CardBase,
    },

    props: {
        data: {
            type: Object as PropType<AppStoreCardData>,
            required: true,
        },
    },

    emits: {
        cardClick: (data: AppStoreCardData) => !!data,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const hoveredIndex = ref(-1);

        // 使用 useThemeIcon 加载主题图标
        const storeIconName = ref("deepin-app-store");
        const storeIconUrl = useThemeIcon(storeIconName, { width: 16, height: 16 });

        const handleAppClick = async (appPackage: string) => {
            console.log("[AppStoreCard] App clicked:", appPackage);

            // 跳转到应用商店对应的应用下载页
            try {
                // 调用后端接口打开应用商店的特定应用页面
                await backendStore.requestSystem("openAppStore", appPackage);
            } catch (error) {
                console.error("Failed to open app store:", error);
            }
        };

        const handleCardClick = () => {
            emit("cardClick", props.data);
        };

        const handleMouseEnter = (index: number) => {
            hoveredIndex.value = index;
        };

        const handleMouseLeave = () => {
            hoveredIndex.value = -1;
        };

        return {
            handleAppClick,
            handleCardClick,
            handleMouseEnter,
            handleMouseLeave,
            hoveredIndex,
            backendStore,
            storeIconUrl,
        };
    },

    methods: {
        isIconUrl(icon: string): boolean {
            return (icon && (icon.startsWith('http://')) || icon.startsWith('https://') || icon.startsWith('data:'));
        },
        /**
         * 格式化下载量显示
         * @param count 下载次数
         * @returns 格式化后的字符串，如 "100k+ downloads"、"<10k downloads"
         */
        formatDownloadCount(count: number): string {
            // 转换逻辑：count < 10000 -> <10k downloads
            //          count < 100000 -> <100k downloads
            //          count >= 100000 -> 100k+ downloads
            if (count < 10000) {
                return this.backendStore.translate("<10k downloads");
            } else if (count < 100000) {
                return this.backendStore.translate("<100k downloads");
            } else {
                return this.backendStore.translate("100k+ downloads");
            }
        },
        /**
         * 渲染评分星星
         * @param score 评分（0-5）
         * @returns 5个星星的JSX数组
         */
        renderStars(score: number) {
            const rating = Math.round(score || 0);
            const stars = [];
            for (let i = 0; i < 5; i++) {
                stars.push(
                    <span
                        key={i}
                        class={[
                            "app-store-card__star",
                            i < rating ? "app-store-card__star--active" : ""
                        ]}
                    >
                        ★
                    </span>
                );
            }
            return stars;
        },
        /**
         * 格式化评分数字显示
         * @param score 评分（整数）
         * @returns 格式化后的字符串
         */
        formatRatingNumber(score: number): string {
            if (!score || score === 0) {
                return "0";
            }
            const rating = Math.round(score);
            return this.backendStore.translate("%1 stars").replace("%1", rating.toString());
        },
    },

    render() {
        const { data } = this.$props;
        const apps = data.apps || [];

        if (apps.length === 0) {
            return null;
        }

        return (
            <CardBase
                title="App Store"
                cardClassSuffix="app-store"
                settingsIcon="appstore"
                settingsText={this.backendStore.translate("App Store")}
                showActionIcon={false}
                dividerWidth="230px"
                onActionClick={this.handleCardClick}
            >
                {/* 自定义顶部插槽：使用主题图标 */}
                {{
                    top: () => (
                        <div class="app-store-card__top">
                            {this.storeIconUrl ? (
                                <img
                                    src={this.storeIconUrl}
                                    class="app-store-card__store-icon"
                                    alt="App Store"
                                />
                            ) : (
                                <SvgIcon icon="appstore" class="app-store-card__store-icon" size={[16, 16]} />
                            )}
                            <span class="app-store-card__store-text">{this.backendStore.translate("App Store")}</span>
                        </div>
                    ),
                    default: () => (
                        /* 下半部分：应用列表 */
                        <div class="app-store-card__bottom">
                            {apps.map((app: any, index: number) => (
                                <div
                                    key={index}
                                    class={[
                                        "app-store-card__app-item",
                                        this.hoveredIndex === index ? "app-store-card__app-item--hovered" : ""
                                    ]}
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        this.handleAppClick(app.package);
                                    }}
                                    title={this.backendStore.translate("Click to download")}
                                    onMouseenter={() => this.handleMouseEnter(index)}
                                    onMouseleave={() => this.handleMouseLeave()}
                                >
                                    {/* 应用图标和名称 */}
                                    <div class="app-store-card__app-header">
                                        <div class={["app-store-card__app-icon", this.isIconUrl(app.icon) ? "has-url" : ""]}>
                                            {this.isIconUrl(app.icon) ? (
                                                <img src={app.icon} alt={app.name} class="app-store-card__app-icon-img" />
                                            ) : (
                                                <SvgIcon icon={app.icon || "app_default"} size={[24, 24]} />
                                            )}
                                        </div>
                                        <div class="app-store-card__app-name">{app.name}</div>
                                    </div>

                                    {/* 应用信息 */}
                                    <div class="app-store-card__app-info">
                                        <div class="app-store-card__app-desc">{app.desc}</div>
                                        <div class="app-store-card__app-stats">
                                            <span class="app-store-card__downloads">
                                                {this.formatDownloadCount(app.downloads || 0)}
                                            </span>
                                            <span class="app-store-card__rating">
                                                {this.renderStars(app.rating || 0)}
                                                <span class="app-store-card__rating-number">
                                                    {this.formatRatingNumber(app.rating || 0)}
                                                </span>
                                            </span>
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )
                }}
            </CardBase>
        );
    },
});
