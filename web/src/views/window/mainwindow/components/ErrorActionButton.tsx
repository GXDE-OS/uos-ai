import { defineComponent, type PropType } from "vue";
import SvgIcon from "@/components/SvgIcon";

/**
 * 错误操作按钮组件
 */
export default defineComponent({
    name: "ErrorActionButton",

    props: {
        text: {
            type: String,
            required: true,
        },
        onClick: {
            type: Function as PropType<(event: MouseEvent) => void>,
            required: true,
        },
    },

    render() {
        const { text, onClick } = this.$props;

        return (
            <div class="error-action-button" onClick={onClick}>
                <span class="error-action-button__text">{text}</span>
                <SvgIcon icon="icon_arrow" size={[14, 14]} class="error-action-button__icon" />
            </div>
        );
    },
});
