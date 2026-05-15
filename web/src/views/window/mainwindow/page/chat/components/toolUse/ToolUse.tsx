import { computed, defineComponent, ref, watch } from "vue";
import type { PropType } from "vue";

import type { ToolUseData, ToolUseValue } from "@/types/conversation";
import { ToolUseStatus } from "@/types/conversation";
import { ButtonShape } from "@/types/button";
import { CopyDataType } from "@/types/message";

import CopyButton from "@/components/CopyButton";
import IconButton from "@/components/IconButton";
import SvgIcon from "@/components/SvgIcon";

import { useBackendStore } from "@/stores";

interface NormalizedToolUseData {
    name: string;
    status: ToolUseStatus;
    params?: ToolUseValue;
    result?: ToolUseValue;
    hasParams: boolean;
    hasResult: boolean;
}

const isPlainObject = (value: unknown): value is Record<string, unknown> => {
    return Object.prototype.toString.call(value) === "[object Object]";
};

const isEmptyValue = (value: unknown): boolean => {
    if (value === undefined || value === null || value === "") {
        return true;
    }

    if (Array.isArray(value)) {
        return value.length === 0;
    }

    if (isPlainObject(value)) {
        return Object.keys(value).length === 0;
    }

    return false;
};

const normalizeStatus = (status: unknown): ToolUseStatus => {
    if (typeof status === "number" && status >= ToolUseStatus.Calling && status <= ToolUseStatus.Canceled) {
        return status;
    }

    if (typeof status === "string") {
        const normalized = status.trim().toLowerCase();
        if (normalized === "completed" || normalized === "complete" || normalized === "success") {
            return ToolUseStatus.Completed;
        }
        if (normalized === "failed" || normalized === "error") {
            return ToolUseStatus.Failed;
        }
        if (normalized === "canceled" || normalized === "cancelled") {
            return ToolUseStatus.Canceled;
        }
    }

    return ToolUseStatus.Calling;
};

const formatToolUseValue = (value: unknown): string => {
    if (isEmptyValue(value)) {
        return "";
    }

    if (typeof value === "string") {
        const trimmed = value.trim();
        if (!trimmed) {
            return "";
        }

        try {
            return JSON.stringify(JSON.parse(trimmed), null, 2);
        } catch {
            return value;
        }
    }

    try {
        return JSON.stringify(value, null, 2);
    } catch {
        return String(value);
    }
};

const normalizeToolUseData = (data: ToolUseData): NormalizedToolUseData => {
    const legacyDisplayContent = isPlainObject(data.display_content) ? data.display_content : undefined;
    const params = legacyDisplayContent?.params ?? data.params;
    const result = legacyDisplayContent?.result ?? data.result;

    return {
        name: legacyDisplayContent?.name ?? data.name ?? useBackendStore().translate("Tool Use"),
        status: normalizeStatus(legacyDisplayContent?.status ?? data.status),
        params,
        result,
        hasParams: !isEmptyValue(params),
        hasResult: !isEmptyValue(result),
    };
};

const getStatusText = (status: ToolUseStatus): string => {
    switch (status) {
        case ToolUseStatus.Completed:
            return useBackendStore().translate("Completed");
        case ToolUseStatus.Failed:
            return useBackendStore().translate("Failed");
        case ToolUseStatus.Canceled:
            return useBackendStore().translate("Canceled");
        case ToolUseStatus.Calling:
        default:
            return useBackendStore().translate("Calling");
    }
};

const getStatusClassName = (status: ToolUseStatus): string => {
    switch (status) {
        case ToolUseStatus.Completed:
            return "completed";
        case ToolUseStatus.Failed:
            return "failed";
        case ToolUseStatus.Canceled:
            return "canceled";
        case ToolUseStatus.Calling:
        default:
            return "calling";
    }
};

const getStatusIconName = (status: ToolUseStatus): string => {
    switch (status) {
        case ToolUseStatus.Completed:
            return "completed";
        case ToolUseStatus.Failed:
            return "warning-red";
        case ToolUseStatus.Canceled:
            return "cancal";
        case ToolUseStatus.Calling:
        default:
            return "loading";
    }
};

export default defineComponent({
    name: "ToolUse",

    components: {
        CopyButton,
        IconButton,
        SvgIcon,
    },

    emits: {
        expandChange: (expanded: boolean) => {
            return typeof expanded === "boolean";
        },
    },

    props: {
        data: {
            type: Object as PropType<ToolUseData>,
            required: true,
        },
        className: {
            type: String,
            default: "",
        },
        defaultExpanded: {
            type: Boolean,
            default: false,
        },
        forceCollapsed: {
            type: Boolean,
            default: false,
        },
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const isExpanded = ref(props.defaultExpanded);

        const normalizedData = computed(() => normalizeToolUseData(props.data));
        const hasDetails = computed(() => normalizedData.value.hasParams || normalizedData.value.hasResult);
        const formattedParams = computed(() => formatToolUseValue(normalizedData.value.params));
        const formattedResult = computed(() => formatToolUseValue(normalizedData.value.result));
        const formattedDetails = computed(() => {
            const parts: string[] = [];

            if (normalizedData.value.hasParams) {
                parts.push(`# ${useBackendStore().translate("Params")}\n${formattedParams.value}`);
            }
            if (normalizedData.value.hasResult) {
                parts.push(`# ${useBackendStore().translate("Result")}\n${formattedResult.value}`);
            }

            return parts.join("\n\n");
        });
        const canCopy = computed(() => normalizedData.value.status !== ToolUseStatus.Calling);

        watch(() => props.forceCollapsed, (val) => {
            if (val && isExpanded.value) {
                isExpanded.value = false;
                emit("expandChange", false);
            }
        });

        const toggleExpand = (event?: Event) => {
            event?.stopPropagation();
            if (!hasDetails.value) {
                return;
            }
            isExpanded.value = !isExpanded.value;
            emit("expandChange", isExpanded.value);
        };

        const handleCopy = (event: MouseEvent) => {
            event.stopPropagation();

            const payload = JSON.stringify(
                {
                    name: normalizedData.value.name,
                    param: formattedParams.value,
                    result: formattedResult.value,
                },
                null,
                2,
            );

            backendStore.requestSystem("copyToClipboard", payload, CopyDataType.CopyText);
        };

        return {
            isExpanded,
            normalizedData,
            hasDetails,
            formattedParams,
            formattedResult,
            formattedDetails,
            canCopy,
            toggleExpand,
            handleCopy,
            getStatusText,
            getStatusClassName,
            ToolUseStatus,
        };
    },

    render() {
        const statusClassName = this.getStatusClassName(this.normalizedData.status);

        return (
            <div class={["tool-use", this.$props.className].filter(Boolean).join(" ")}>
                <div
                    class={["tool-use__header", this.hasDetails && "tool-use__header--interactive"]
                        .filter(Boolean)
                        .join(" ")}
                    onClick={this.hasDetails ? this.toggleExpand : undefined}
                >
                    <div class="tool-use__header-main">
                        {this.hasDetails ? (
                            <IconButton
                                class={["tool-use__expand-btn", this.isExpanded && "is-expanded"]
                                    .filter(Boolean)
                                    .join(" ")}
                                icon="icon_arrow"
                                size={[24, 24]}
                                iconSize={[12, 12]}
                                shape={ButtonShape.Rounded}
                                onClick={this.toggleExpand}
                            />
                        ) : (
                            <span class="tool-use__expand-placeholder" />
                        )}
                        <span class="tool-use__name" title={this.normalizedData.name}>
                            {this.normalizedData.name}
                        </span>
                        <SvgIcon
                            class={[
                                "tool-use__status-icon",
                                `tool-use__status-icon--${statusClassName}`,
                                this.normalizedData.status === this.ToolUseStatus.Calling && "is-spinning",
                            ]
                                .filter(Boolean)
                                .join(" ")}
                            icon={getStatusIconName(this.normalizedData.status)}
                            size={[16, 16]}
                        />
                        <span class="tool-use__status-text">{this.getStatusText(this.normalizedData.status)}</span>
                    </div>

                    {this.canCopy && (
                        <CopyButton
                            size={[24, 24]}
                            iconSize={[16, 16]}
                            shape={ButtonShape.Rounded}
                            onClick={this.handleCopy}
                            className="tool-use__copy"
                        />
                    )}
                </div>

                {this.hasDetails && this.isExpanded && (
                    <div class="tool-use__content">
                        <pre class="tool-use__code">{this.formattedDetails}</pre>
                    </div>
                )}
            </div>
        );
    },
});
