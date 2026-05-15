import { defineComponent, PropType, ref } from "vue";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";
import { useBackendStore } from "@/stores";
import { useThemeIcon } from "@/utils/loadThemeIcon";

/**
 * 基础卡片组件
 * 用于复用卡片的通用上半部分和分割线
 */
export default defineComponent({
    name: "CardBase",

    components: {
        SvgIcon,
        Tooltip,
    },

    props: {
        /** 卡片标题 */
        title: {
            type: String,
            required: true,
        },
        /** 卡片类名后缀，用于生成类名如 'card-base__top' */
        cardClassSuffix: {
            type: String,
            default: "base",
        },
        /** 设置图标，默认为控制中心图标 */
        settingsIcon: {
            type: String,
            default: "controlcenter",
        },
        /** 设置文本，默认为 'System Settings' */
        settingsText: {
            type: String,
            default: () => useBackendStore().translate("System Settings"),
        },
        /** 显示右上角跳转图标 */
        showActionIcon: {
            type: Boolean,
            default: true,
        },
        /** 分割线宽度 */
        dividerWidth: {
            type: String,
            default: "250px",
        },
    },

    emits: {
        actionClick: () => true,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const handleActionClick = (e: MouseEvent) => {
            e.stopPropagation();
            emit("actionClick");
        };

        // 使用 useThemeIcon 加载主题图标
        const settingsIconName = ref("preferences-system");
        const settingsIconUrl = useThemeIcon(settingsIconName, { width: 16, height: 16 });

        return {
            handleActionClick,
            backendStore,
            settingsIconUrl,
        };
    },

    render() {
        const suffix = this.$props.cardClassSuffix;

        return (
            <div class={[`card-base card-base--${suffix}`, `card-base--${suffix}`]}>
                {/* 右上角跳转图标 */}
                {this.$props.showActionIcon && (
                    <Tooltip
                        content={this.backendStore.translate("Click to go to app")}
                        placement="top"
                    >
                        <div
                            class={`card-base__action-icon card-base__action-icon--${suffix}`}
                            onClick={this.handleActionClick}
                        >
                            <SvgIcon icon="gotoapp" size={[16, 16]} />
                        </div>
                    </Tooltip>
                )}

                {/* 上半部分：系统设置 */}
                {this.$slots.top ? (
                    this.$slots.top()
                ) : (
                    <div class={`card-base__top card-base__top--${suffix}`}>
                        {this.settingsIconUrl ? (
                            <img
                                src={this.settingsIconUrl}
                                class={`card-base__settings-icon card-base__settings-icon--${suffix}`}
                                alt="settings"
                            />
                        ) : (
                            <SvgIcon
                                icon={this.$props.settingsIcon}
                                class={`card-base__settings-icon card-base__settings-icon--${suffix}`}
                                size={[16, 16]}
                            />
                        )}
                        <span class={`card-base__settings-text card-base__settings-text--${suffix}`}>
                            {this.$props.settingsText}
                        </span>
                    </div>
                )}

                {/* 分割线 */}
                <div
                    class={`card-base__divider card-base__divider--${suffix}`}
                    style={{
                        width: this.$props.dividerWidth,
                    }}
                />

                {/* 下半部分：插槽或默认内容 */}
                {this.$slots.default ? (
                    <div class={`card-base__bottom card-base__bottom--${suffix}`}>{this.$slots.default()}</div>
                ) : null}
            </div>
        );
    },
});
