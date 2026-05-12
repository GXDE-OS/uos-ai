import { defineComponent } from "vue";
import "@/assets/styles/window/mainwindow/sidebar/components/GroupHeader.css";
import IconButton from "@/components/IconButton";
import { ButtonShape } from "@/types/button";

export default defineComponent({
    name: "GroupHeader",
    props: {
        groupName: {
            type: String,
            required: true,
        },
        groupId: {
            type: String,
            required: true,
        },
        headerDomId: {
            type: String,
            required: false,
        },
        hidden: {
            type: Boolean,
            required: false,
            default: false,
        },
        rightButtonIcon: {
            type: String,
            required: false,
        },
        rightButtonTooltip: {
            type: String,
            required: false,
        },
    },
    emits: ["rightButtonIconClick"],
    setup(props, { emit }) {
        const handleClick = () => {
            const element = document.getElementById(`group-${props.groupId}`);
            const scrollContainer = element?.closest<HTMLElement>(".scroll-bar-content");
            if (!element || !scrollContainer) {
                element?.scrollIntoView({ behavior: "smooth", block: "start" });
                return;
            }

            const style = window.getComputedStyle(element);
            const scrollMarginTop = Number.parseFloat(style.scrollMarginTop || "0") || 0;
            const nextScrollTop = Math.max(0, element.offsetTop - scrollMarginTop);

            scrollContainer.scrollTo({
                top: nextScrollTop,
                behavior: "smooth",
            });
        };

        const handleButtonIconClick = (event: MouseEvent) => {
            // 防止事件冒泡
            event.stopPropagation();
            emit("rightButtonIconClick", { groupId: props.groupId });
        };

        return {
            handleClick,
            handleButtonIconClick,
        };
    },
    render() {
        return (
            <div
                id={this.headerDomId || `group-header-${this.groupId}`}
                class={["group-header", this.hidden && "group-header--hidden"]}
                onClick={this.handleClick}
            >
                <span class="group-header__name">{this.groupName}</span>
                {this.rightButtonIcon && (
                    <IconButton
                        icon={this.rightButtonIcon}
                        tooltip={this.rightButtonTooltip}
                        iconSize={[16, 16]}
                        size={[24, 24]}
                        shape={ButtonShape.Rounded}
                        onClick={this.handleButtonIconClick}
                    />
                )}
            </div>
        );
    },
});
