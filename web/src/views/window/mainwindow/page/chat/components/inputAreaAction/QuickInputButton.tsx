import { defineComponent } from "vue";
import IconTextButton from "@/components/IconTextButton";

export default defineComponent({
    name: "QuickInputButton",

    props: {
        icon: {
            type: String,
            required: true,
        },
        text: {
            type: String,
            required: true,
        },
        // 附加 CSS 类名，用于覆盖默认样式
        styleClass: {
            type: String,
            default: "",
        },
    },

    emits: {
        click: null,
    },

    setup(props, { emit }) {
        const handleClick = () => {
            emit("click");
        };

        return {
            handleClick,
        };
    },

    render() {
        return (
            <IconTextButton
                class={["quick-input-button", this.$props.styleClass]}
                icon={this.$props.icon}
                iconSize={[16, 16]}
                text={this.$props.text}
                onClick={this.handleClick}
            />
        );
    },
});
