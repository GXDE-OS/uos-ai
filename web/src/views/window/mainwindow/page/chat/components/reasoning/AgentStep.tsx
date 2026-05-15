import { defineComponent, ref, watch, nextTick } from "vue";
import type { PropType } from "vue";
import type { AgentStepData, AgentStepEntry } from "@/types/conversation";
import ToolUse from "@/views/window/mainwindow/page/chat/components/toolUse/ToolUse";
import SvgIcon from "@/components/SvgIcon";

export default defineComponent({
    name: "AgentStep",

    components: { ToolUse },

    props: {
        data: {
            type: Object as PropType<AgentStepData>,
            required: true,
        },
    },

    setup(props) {
        const isExpanded = ref(false);
        const contentRef = ref<HTMLElement | null>(null);

        const toggle = () => { isExpanded.value = !isExpanded.value; };

        // 强制滚动到底部
        const scrollToBottom = () => {
            const el = contentRef.value;
            if (el) el.scrollTop = el.scrollHeight;
        };

        watch(() => props.data, (data) => {
            if (data.status !== 0) {
                isExpanded.value = false;
            } else if ((data.entries && data.entries.length > 0) || !!data.content) {
                isExpanded.value = true;
            }
        }, { deep: true, immediate: true });

        // 监听 entries 变化：同时追踪数组长度和最后一条文本内容
        // text 细节是原地拼接，不改变 length，所以需要一起追踪其内容
        watch(
            () => {
                const entries = props.data.entries;
                if (!entries || entries.length === 0) return "";
                const last = entries[entries.length - 1];
                if (last?.kind === "text") {
                    return `${entries.length}:text:${last.content ?? ""}`;
                }
                return `${entries.length}:tool:${(last as any)?.data?.name ?? ""}:${(last as any)?.data?.status ?? ""}`;
            },
            () => {
                if (!isExpanded.value) return;
                nextTick(scrollToBottom);
            },
        );

        // 展开时立即滚动到底部
        watch(isExpanded, (val) => {
            if (val) nextTick(scrollToBottom);
        });

        return { isExpanded, toggle, contentRef };
    },

    render() {
        const { title, status, content, entries } = this.$props.data;
        // 如果没有细节内容，禁止展开
        const canExpand = !!(entries && entries.length > 0) || !!content;

        const renderEntry = (entry: AgentStepEntry, idx: number) => {
            if (entry.kind === "text") {
                return <div key={idx} class="agent-step__content">{entry.content}</div>;
            }
            // Debug: 打印数据结构
            console.log("[AgentStep] tool entry:", entry.data);
            return <ToolUse key={idx} data={entry.data} forceCollapsed={!this.isExpanded} />;
        };

        const renderStatusIcon = () => {
            switch (status) {
                case 1: // Completed
                    return (
                        <span class="agent-step__icon agent-step__icon--done">
                            <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                                <path d="M2.5 7L5.5 10L11.5 4" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
                            </svg>
                        </span>
                    );
                case 2: // Failed
                    return (
                        <span class="agent-step__icon agent-step__icon--failed">
                            <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                                <path d="M3.5 3.5L10.5 10.5M10.5 3.5L3.5 10.5" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"/>
                            </svg>
                        </span>
                    );
                case 3: // Canceled
                    return (
                        <span class="agent-step__icon agent-step__icon--canceled">
                            <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
                                <path d="M3.5 7H10.5" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"/>
                            </svg>
                        </span>
                    );
                default: // Running
                    return (
                        <span class="agent-step__icon agent-step__icon--running">
                            <span class="agent-step__pulse" />
                        </span>
                    );
            }
        };

        return (
            <div class={`agent-step ${this.isExpanded ? "agent-step--expanded" : ""}`}>
                <div class="agent-step__header" onClick={canExpand ? this.toggle : undefined}>
                    <div class="agent-step__header-left">
                        {renderStatusIcon()}
                        <span class="agent-step__title">{title}</span>
                    </div>
                    {canExpand && (
                        <span class="agent-step__chevron">
                            {this.isExpanded ? (
                                <SvgIcon icon="mdeditor-exit-fullscreen" size={[16, 16]} />
                            ) : (
                                <SvgIcon icon="mdeditor-fullscreen" size={[16, 16]} />
                            )}
                        </span>
                    )}
                </div>

                {canExpand && this.isExpanded && (
                    <div class="agent-step__body agent-step__body--expanded" ref="contentRef"
                        onWheel={(e: WheelEvent) => {
                            const el = e.currentTarget as HTMLElement;
                            const atTop = el.scrollTop <= 0;
                            const atBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 1;
                            if ((e.deltaY < 0 && atTop) || (e.deltaY > 0 && atBottom)) return;
                            e.stopPropagation();
                        }}>
                        {entries && entries.map(renderEntry)}
                        {content && (
                            <div class={`agent-step__content${entries && entries.length > 0 ? " agent-step__content--summary" : ""}`}>{content}</div>
                        )}
                    </div>
                )}
            </div>
        );
    },
});
