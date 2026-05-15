import { defineComponent } from "vue";
import listenVideo from "@/assets/video/listen.webm";
import listen1Image from "@/assets/images/listen1.png";
import listen2Image from "@/assets/images/listen2.png";
import "@/assets/styles/components/DigitalHuman.css";
import type { PropType } from "vue";
import { DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "ListenState",
    props: {
        // 缩放比例最底层（listen2.png）
        scale: {
            type: Number,
            default: 1,
        },
        // 缩放比例第二层（listen1.png）
        scale1: {
            type: Number,
            default: 1,
        },
        // 点击事件回调
        onClick: {
            type: Function as PropType<(state: DigitalHumanState) => void>,
            required: false,
        },
    },
    setup(props) {
        const handleClick = () => {
            if (props.onClick) {
                props.onClick(DigitalHumanState.Listen);
            }
        };

        return {
            handleClick,
        };
    },
    render() {
        return (
            <div
                class="listen-container"
                style={
                    {
                        "--scale": this.$props.scale,
                        "--scale1": this.$props.scale1,
                    } as any
                }
                onClick={this.handleClick}
            >
                {/* PNG 图片容器 - 位于下层 */}
                <div class="listen">
                    <div class="scale">
                        <img src={listen2Image} class="listen-img rotate" alt="listen2" />
                    </div>
                    <div class="scale1">
                        <img src={listen1Image} class="listen-img rotate" alt="listen1" />
                    </div>
                </div>

                {/* 视频容器 - 位于上层（z-index: 1） */}
                <video class="listen-video" autoplay loop muted playsinline>
                    <source src={listenVideo} type="video/webm" />
                </video>
            </div>
        );
    },
});
