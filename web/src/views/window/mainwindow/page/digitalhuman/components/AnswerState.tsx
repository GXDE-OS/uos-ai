import { defineComponent } from "vue";
import playVideo from "@/assets/video/play.webm";
import type { PropType } from "vue";
import { DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "AnswerState",
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
                props.onClick(DigitalHumanState.Answer);
            }
        };

        return {
            handleClick,
        };
    },
    render() {
        return (
            <video class="answer-video" autoplay loop muted playsinline onClick={this.handleClick}>
                <source src={playVideo} type="video/webm" />
            </video>
        );
    },
});
