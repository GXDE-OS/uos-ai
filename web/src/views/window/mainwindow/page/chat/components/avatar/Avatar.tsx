import { defineComponent, computed, ref, watch } from "vue";
import SvgIcon from "@/components/SvgIcon";
import { AssistantID } from "@/types/assistant";
import { createId } from "@/utils/date";

export default defineComponent({
    name: "Avatar",

    props: {
        // 可选的头像图标名称
        icon: {
            type: String,
            default: "",
        },
        assistantId: {
            type: String,
            required: true,
            default: "",
        },
        loading: {
            type: Boolean,
            default: false,
        },
        gradient_colors: {
            // 流光动效渐变颜色数组
            type: Array,
            default: () => [],
        },
    },

    setup(props) {
        const gradientId = `ring-grad-${createId()}`;
        const dynamicKeyframeName = `color-flow-${gradientId}`;

        const hasCustomGradient = computed(() => {
            return (props.gradient_colors as string[]).length >= 3;
        });

        const dynamicStyle = computed(() => {
            const colors = (props.gradient_colors as string[]).slice(0, 3);
            return `@keyframes ${dynamicKeyframeName} { 0% { stop-color: ${colors[0]}; } 50% { stop-color: ${colors[1]}; } 100% { stop-color: ${colors[2]}; } }`;
        });

        // 仅在 loading 从 true→false 时才播放 loaded 动画，避免切换历史聊天时无意义触发
        const shouldAnimate = ref(false);

        watch(
            () => props.loading,
            (newVal, oldVal) => {
                if (newVal === true) {
                    shouldAnimate.value = false;
                } else if (oldVal === true && newVal === false) {
                    shouldAnimate.value = true;
                }
            },
            { flush: "pre" },
        );

        const animationType = computed(() => {
            if (props.assistantId === AssistantID.UOS_AI_TRANSLATION) {
                return "square";
            } else if (props.assistantId === AssistantID.UOS_AI) {
                return "hexagon";
            } else {
                return "circle";
            }
        });

        return {
            animationType,
            gradientId,
            hasCustomGradient,
            dynamicKeyframeName,
            dynamicStyle,
            shouldAnimate,
        };
    },

    render() {
        const isSquare = this.animationType === "square";
        const isHexagon = this.animationType === "hexagon";
        const isCircle = this.animationType === "circle";
        const isLoading = this.loading;

        const orbitClass = ["avatar__orbit", isLoading && "avatar__orbit--active"].filter(Boolean).join(" ");

        const renderOrbit = () => {
            const stopStyle = this.hasCustomGradient
                ? { animation: `${this.dynamicKeyframeName} 4s linear infinite alternate` }
                : undefined;

            return (
                <svg class={orbitClass} viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                    <defs>
                        <linearGradient id={this.gradientId}>
                            <stop class="glow-stop" offset="0%" style={stopStyle} />
                            <stop class="glow-stop" offset="100%" style={stopStyle} />
                        </linearGradient>
                    </defs>
                    <circle cx="12" cy="12" r="11" class="orbit-line" stroke={`url(#${this.gradientId})`} />
                    <circle
                        cx="12"
                        cy="12"
                        r="11"
                        class="orbit-line orbit-line-2"
                        stroke={`url(#${this.gradientId})`}
                    />
                </svg>
            );
        };

        const renderWithAnimation = (type: "square" | "circle" | "hexagon", content: JSX.Element) => {
            let animClass = "";
            if (isLoading) {
                animClass =
                    type === "square"
                        ? "avatar__icon__inner--loading"
                        : type === "hexagon"
                          ? "avatar__icon__inner--hex-loading"
                          : "avatar__icon__inner--circle-loading";
            } else if (this.shouldAnimate) {
                animClass =
                    type === "square"
                        ? "avatar__icon__inner--loaded"
                        : type === "hexagon"
                          ? "avatar__icon__inner--hex-loaded"
                          : "avatar__icon__inner--circle-loaded";
            }

            const innerClass = ["avatar__icon__inner", animClass].filter(Boolean).join(" ");

            return (
                <div class={`avatar__icon avatar__icon--${type}`}>
                    {renderOrbit()}
                    <div class={innerClass}>{content}</div>
                </div>
            );
        };

        const renderSvgIcon = (icon: string) => {
            if (isSquare) {
                return renderWithAnimation("square", <SvgIcon icon={icon} size={[24, 24]} />);
            }
            if (isHexagon) {
                return renderWithAnimation("hexagon", <SvgIcon icon={icon} size={[24, 24]} />);
            }
            if (isCircle) {
                return renderWithAnimation("circle", <SvgIcon icon={icon} size={[24, 24]} />);
            }
            return (
                <div class="avatar__icon">
                    <SvgIcon icon={icon} size={[24, 24]} />
                </div>
            );
        };

        const renderIcon = () => {
            if (this.icon && this.icon !== "") {
                if (this.icon.startsWith("file://")) {
                    return renderWithAnimation(
                        "circle",
                        <div class="avatar__icon_img avatar__icon">
                            <img src={this.icon} alt="" />
                        </div>,
                    );
                }
                return renderSvgIcon(this.icon);
            }
            return renderSvgIcon("lost-assistant-avatar");
        };

        return (
            <div class="avatar">
                {this.hasCustomGradient && <style>{this.dynamicStyle}</style>}
                {renderIcon()}
            </div>
        );
    },
});
