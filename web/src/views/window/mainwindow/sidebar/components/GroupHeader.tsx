import { computed, defineComponent } from "vue";
import "@/assets/styles/window/mainwindow/sidebar/components/GroupHeader.css";
import IconButton from "@/components/IconButton";
import Tooltip from "@/components/Tooltip";
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
        tooltip: {
            type: String,
            required: false,
        },
        collapsible: {
            type: Boolean,
            required: false,
            default: false,
        },
        collapsed: {
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
    emits: ["rightButtonIconClick", "collapse", "click"],
    setup(props, { emit }) {
        const handleClick = () => {
            emit("click", { groupId: props.groupId });
        };

        const handleButtonIconClick = (event: MouseEvent) => {
            // 防止事件冒泡
            event.stopPropagation();
            if (props.collapsible) {
                emit("collapse");
            } else {
                emit("rightButtonIconClick", { groupId: props.groupId });
            }
        };

        const collapseBtnClass = computed(() =>
            props.collapsible
                ? ["group-header__collapse-btn", props.collapsed && "group-header__collapse-btn--collapsed"]
                : undefined,
        );

        return {
            handleClick,
            handleButtonIconClick,
            collapseBtnClass,
        };
    },
    render() {
        const buttonIcon = this.collapsible ? "icon_arrow" : this.rightButtonIcon;
        const tooltipContent = this.tooltip || "";

        return (
            <Tooltip
                content={tooltipContent}
                disabled={!this.tooltip || this.hidden}
                showAfter={1000}
                placement="top-start"
                offset={4}
            >
                <div
                    id={this.headerDomId || `group-header-${this.groupId}`}
                    class={["group-header", this.hidden && "group-header--hidden"]}
                    onClick={this.handleClick}
                >
                    <span class="group-header__name">{this.groupName}</span>
                    {buttonIcon && (
                        <IconButton
                            icon={buttonIcon}
                            tooltip={this.collapsible ? undefined : this.rightButtonTooltip}
                            iconSize={[12, 12]}
                            size={[16, 16]}
                            shape={ButtonShape.Circle}
                            onClick={this.handleButtonIconClick}
                            class={this.collapseBtnClass}
                        />
                    )}
                </div>
            </Tooltip>
        );
    },
});
