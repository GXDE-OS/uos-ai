import { defineComponent } from "vue";
import thinkingVideo from "@/assets/video/thinking.webm";
import type { PropType } from "vue";
import { DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "ThinkingState",
    props: {
        // 点击事件回调
        onClick: {
            type: Function as PropType<(state: DigitalHumanState) => void>,
            required: false,
        },
    },
    setup(props) {
        const handleClick = () => {
            if (props.onClick) {
                props.onClick(DigitalHumanState.Thinking);
            }
        };

        return {
            handleClick,
        };
    },
    render() {
        return (
            <video class="thinking-video" autoplay loop muted playsinline onClick={this.handleClick}>
                <source src={thinkingVideo} type="video/webm" />
            </video>
        );
    },
});
