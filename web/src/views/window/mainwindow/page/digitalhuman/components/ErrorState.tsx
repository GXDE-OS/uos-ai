import { defineComponent, ref } from "vue";
import errorVideo from "@/assets/video/error.webm";
import type { PropType } from "vue";
import { DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "ErrorState",
    props: {
        // 点击事件回调
        onClick: {
            type: Function as PropType<(state: DigitalHumanState) => void>,
            required: false,
        },
    },
    setup(props) {
        const hasPlayed = ref(false);
        const videoRef = ref<HTMLVideoElement | null>(null);

        const handleClick = () => {
            if (props.onClick) {
                props.onClick(DigitalHumanState.Error);
            }
        };

        const handleVideoEnded = () => {
            hasPlayed.value = true;
        };

        // 当组件挂载时，如果视频还没有播放过，就播放一次
        const playVideoOnce = () => {
            if (!hasPlayed.value && videoRef.value) {
                videoRef.value.play().catch(error => {
                    console.error("Error playing video:", error);
                });
            }
        };

        return {
            handleClick,
            handleVideoEnded,
            videoRef,
            playVideoOnce,
            hasPlayed,
        };
    },
    mounted() {
        this.playVideoOnce();
    },
    render() {
        return (
            <video
                ref="videoRef"
                class="error-video"
                muted
                playsinline
                onClick={this.handleClick}
                onEnded={this.handleVideoEnded}
            >
                <source src={errorVideo} type="video/webm" />
            </video>
        );
    },
});
