import { defineComponent, ref, computed, watch, onMounted, onUnmounted, PropType } from "vue";
import SvgIcon from "@/components/SvgIcon";
import ScrollBar from "@/components/ScrollBar";
import CommonButton from "@/components/CommonButton";
import TextButton from "@/components/TextButton";
import type { BashApproveData } from "@/types/conversation";
import { InteractiveCompStatus } from "@/types/conversation";
import { useBackendStore } from "@/stores/backend";

type OptionType = "allow" | "allow_session" | "reject";

export default defineComponent({
    name: "BashApprove",

    props: {
        data: {
            type: Object as PropType<BashApproveData>,
            required: true,
        },
        onSubmit: {
            type: Function as PropType<(action: Record<string, unknown>) => void>,
            required: true,
        },
    },

    setup(props) {
        const backendStore = useBackendStore();
        const selectedIndex = ref<OptionType>("allow");
        const rejectMsg = ref("");
        const commandScrollBar = ref<InstanceType<typeof ScrollBar> | null>(null);

        const isDisabled = computed(() => props.data.status !== InteractiveCompStatus.PENDING);

        const headerTitle = computed(() => backendStore.translate("Awaiting Approval"));
        const skipText = computed(() => backendStore.translate("Skip"));
        const submitText = computed(() => backendStore.translate("Submit"));
        const options = computed<Array<{ key: OptionType; label: string }>>(() => [
            {
                key: "allow",
                label: backendStore.translate("Allow Once - This command only"),
            },
            {
                key: "allow_session",
                label: backendStore.translate("Allow Chat - For this chat"),
            },
            {
                key: "reject",
                label: backendStore.translate("Reject & Revise - Tell UOS AI what to change"),
            },
        ]);

        const rejectInputRef = ref<HTMLInputElement | null>(null);

        const handleRejectFocus = () => {
            selectedIndex.value = "reject";
        };

        watch(selectedIndex, (val) => {
            if (val !== "reject") {
                rejectInputRef.value?.blur();
            }
        });

        const handleKeyDown = (e: KeyboardEvent) => {
            if (isDisabled.value) return;

            const target = e.target as HTMLElement;

            if (e.key === "Tab") {
                e.preventDefault();
                const idx = options.value.findIndex((o) => o.key === selectedIndex.value);
                const nextIdx = e.shiftKey
                    ? (idx - 1 + options.value.length) % options.value.length
                    : (idx + 1) % options.value.length;
                selectedIndex.value = options.value[nextIdx].key;
                if (selectedIndex.value === "reject") rejectInputRef.value?.focus();
            } else if (e.key === "ArrowDown") {
                e.preventDefault();
                const idx = options.value.findIndex((o) => o.key === selectedIndex.value);
                const nextIdx = (idx + 1) % options.value.length;
                selectedIndex.value = options.value[nextIdx].key;
                if (selectedIndex.value === "reject") rejectInputRef.value?.focus();
            } else if (e.key === "ArrowUp") {
                e.preventDefault();
                const idx = options.value.findIndex((o) => o.key === selectedIndex.value);
                const prevIdx = (idx - 1 + options.value.length) % options.value.length;
                selectedIndex.value = options.value[prevIdx].key;
                if (selectedIndex.value === "reject") rejectInputRef.value?.focus();
            } else if (e.key === "Enter") {
                e.preventDefault();
                e.stopPropagation();
                handleSubmit();
            } else if (e.key >= "1" && e.key <= "9") {
                if (selectedIndex.value === "reject") {
                    return;
                }
                const numIdx = parseInt(e.key) - 1;
                if (numIdx < options.value.length) {
                    selectedIndex.value = options.value[numIdx].key;
                    if (selectedIndex.value === "reject") {
                        e.preventDefault();
                        rejectInputRef.value?.focus();
                    }
                }
            }
        };

        const handleOptionClick = (key: OptionType) => {
            if (isDisabled.value) return;
            selectedIndex.value = key;
        };

        const handleSkip = () => {
            if (isDisabled.value) return;
            props.onSubmit({
                request_id: props.data.id,
                type: props.data.ic_type,
                approved: false,
                always_approve: false,
                reject_msg: "",
            });
        };

        const handleSubmit = () => {
            if (isDisabled.value) return;
            if (selectedIndex.value === "reject") {
                props.onSubmit({
                    request_id: props.data.id,
                    type: props.data.ic_type,
                    approved: false,
                    always_approve: false,
                    reject_msg: rejectMsg.value,
                });
            } else if (selectedIndex.value === "allow_session") {
                props.onSubmit({
                    request_id: props.data.id,
                    type: props.data.ic_type,
                    approved: true,
                    always_approve: true,
                    reject_msg: "",
                });
            } else {
                props.onSubmit({
                    request_id: props.data.id,
                    type: props.data.ic_type,
                    approved: true,
                    always_approve: false,
                    reject_msg: "",
                });
            }
        };

        const handleCommandWheel = (e: WheelEvent) => {
            const container = commandScrollBar.value?.scrollContainerRef;
            if (!container) return;

            const { scrollTop, scrollHeight, clientHeight } = container;
            const isAtTop = scrollTop <= 0;
            const isAtBottom = scrollTop + clientHeight >= scrollHeight - 1;

            if ((e.deltaY > 0 && !isAtBottom) || (e.deltaY < 0 && !isAtTop)) {
                e.stopPropagation();
            }
        };

        onMounted(() => {
            window.addEventListener("keydown", handleKeyDown, { capture: true });
        });

        onUnmounted(() => {
            window.removeEventListener("keydown", handleKeyDown, true);
        });

        return {
            selectedIndex,
            rejectMsg,
            options,
            isDisabled,
            headerTitle,
            skipText,
            submitText,
            handleOptionClick,
            rejectInputRef,
            handleSkip,
            handleSubmit,
            handleRejectFocus,
            commandScrollBar,
            handleCommandWheel,
        };
    },

    render() {
        const { data } = this.$props;

        return this.isDisabled ? null : (
            <div class="bash-approve">
                <header class="bash-approve__header">
                    <div class="bash-approve__header-left">
                        <SvgIcon icon="bash-wait" size={[16, 16]} />
                        <span class="bash-approve__title">{this.headerTitle}</span>
                    </div>
                </header>
                <div class="bash-approve__command_wapper">
                    <div class="bash-approve__command-title">{data.title}</div>
                    <div class="bash-approve__command" onWheel={this.handleCommandWheel}>
                        <ScrollBar ref="commandScrollBar">
                            <div class="bash-approve__command-text">{data.command}</div>
                        </ScrollBar>
                    </div>
                </div>
                <div class="bash-approve__options">
                    {this.options.map((option, idx) => (
                        <div
                            key={option.key}
                            class={[
                                "bash-approve__option",
                                option.key === "reject" ? "bash-approve__option--reject" : "",
                                this.selectedIndex === option.key ? "bash-approve__option--selected" : "",
                            ]}
                            onClick={() => this.handleOptionClick(option.key)}
                        >
                            <span class="bash-approve__option-index">{idx + 1}.</span>
                            {option.key === "reject" ? (
                                <input
                                    ref="rejectInputRef"
                                    type="text"
                                    class="bash-approve__reject-textarea"
                                    placeholder={option.label}
                                    value={this.rejectMsg}
                                    onClick={(e: Event) => e.stopPropagation()}
                                    onFocus={this.handleRejectFocus}
                                    onInput={(e: Event) => {
                                        this.rejectMsg = (e.target as HTMLInputElement).value;
                                    }}
                                />
                            ) : (
                                <span class="bash-approve__option-label">{option.label}</span>
                            )}
                        </div>
                    ))}
                </div>
                <div class="bash-approve__actions">
                    <TextButton text={this.skipText} disabled={this.isDisabled} onClick={this.handleSkip} />
                    <CommonButton
                        class="bash-approve__submit-btn"
                        text={this.submitText}
                        variant="default"
                        disabled={this.isDisabled}
                        onClick={this.handleSubmit}
                    />
                </div>
            </div>
        );
    },
});
