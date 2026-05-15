import { defineComponent, ref, type PropType, type CSSProperties } from "vue";
import IconButton from "./IconButton";
import { ButtonShape } from "@/types/button";

export default defineComponent({
    name: "CopyButton",
    props: {
        // 按钮大小
        size: {
            type: Array as unknown as PropType<[number, number]>,
            default: () => [24, 24],
        },
        // 图标大小
        iconSize: {
            type: Array as unknown as PropType<[number, number]>,
            default: () => [16, 16],
        },
        // 按钮形状
        shape: {
            type: String as PropType<ButtonShape>,
            default: ButtonShape.Rounded,
        },
        // 提示文本
        tooltip: {
            type: String,
            default: "复制",
        },
        // 点击回调
        onClick: {
            type: Function as PropType<(event: MouseEvent) => void>,
            required: true,
        },
        // 自定义类名
        className: {
            type: String,
            default: "",
        },
        // 自定义样式
        style: {
            type: Object as PropType<CSSProperties>,
            default: () => ({}),
        },
    },
    setup(props) {
        // 跟踪当前图标状态：true 表示显示 checked 图标，false 表示显示 copy 图标
        const isCopied = ref(false);
        // 定时器引用
        let timer: number | null = null;

        // 处理点击事件
        const handleClick = (event: MouseEvent) => {
            // 执行父组件传入的回调
            props.onClick(event);

            // 切换到 checked 图标
            isCopied.value = true;

            // 清除之前的定时器（如果存在）
            if (timer !== null) {
                clearTimeout(timer);
            }

            // 2秒后恢复到 copy 图标
            timer = window.setTimeout(() => {
                isCopied.value = false;
                timer = null;
            }, 2000);
        };

        return {
            isCopied,
            handleClick,
        };
    },
    render() {
        return (
            <IconButton
                icon={this.isCopied ? "checked" : "copy"}
                size={this.$props.size}
                iconSize={this.$props.iconSize}
                shape={this.$props.shape}
                tooltip={this.isCopied ? "已复制" : this.$props.tooltip}
                onClick={this.handleClick}
                class={this.$props.className}
                style={this.$props.style}
            />
        );
    },
});
