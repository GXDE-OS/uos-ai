import { defineComponent } from "vue";
import silenceVideo from "@/assets/video/silence.webm";
import type { PropType } from "vue";
import { DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "SilenceState",
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
                props.onClick(DigitalHumanState.Silence);
            }
        };

        return {
            handleClick,
        };
    },
    render() {
        return (
            <video class="silence-video" autoplay loop muted playsinline onClick={this.handleClick}>
                <source src={silenceVideo} type="video/webm" />
            </video>
        );
    },
});
