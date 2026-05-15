import { defineComponent, computed } from "vue";
import "@/assets/styles/window/mainwindow/sidebar/components/ToggleExpand.css";
import SvgIcon from "@/components/SvgIcon";
import { useBackendStore } from "@/stores";

export default defineComponent({
    name: "ToggleExpand",
    props: {
        expanded: {
            type: Boolean,
            default: false,
        },
        hiddenCount: {
            type: Number,
            required: true,
        },
    },
    emits: ["toggle"],
    setup(props, { emit }) {
        const backendStore = useBackendStore();

        const handleClick = () => {
            emit("toggle");
        };

        const moreText = computed(() => backendStore.translate("More"));
        const collapseText = computed(() => backendStore.translate("Collapse"));
        return {
            handleClick,
            moreText,
            collapseText,
        };
    },
    render() {
        return (
            <div class="toggle-expand" onClick={this.handleClick}>
                <div class="toggle-expand__left">
                    <SvgIcon icon="icon_sidebar_more" size={[16, 16]}></SvgIcon>
                    <span class="toggle-expand__text">{this.expanded ? this.collapseText : this.moreText}</span>
                </div>
                <SvgIcon
                    icon="icon_arrow"
                    size={[12, 12]}
                    class={["toggle-expand__arrow", this.expanded ? "toggle-expand__arrow--expanded" : ""]}
                ></SvgIcon>
            </div>
        );
    },
});
