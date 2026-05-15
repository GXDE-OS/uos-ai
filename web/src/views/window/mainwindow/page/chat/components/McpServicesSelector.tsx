import { computed, defineComponent, onMounted, onUnmounted, ref, type PropType, type VNodeRef } from "vue";
import CheckButton from "@/components/CheckButton";
import SvgIcon from "@/components/SvgIcon";
import type { McpService } from "@/types/mcp-service";
import { useBackendStore } from "@/stores";

export default defineComponent({
    name: "McpServicesSelector",

    props: {
        services: {
            type: Array as PropType<McpService[]>,
            required: true,
        },
        selectedServiceIds: {
            type: Array as PropType<string[]>,
            default: () => [],
        },
        disabled: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        toggleService: (_serviceId: string, _checked: boolean) => true,
        toggleSelectAll: (_checked: boolean) => true,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const isOpen = ref(false);
        const rootRef = ref<HTMLElement | null>(null);

        const enabledCount = computed(() => props.services.length);
        const selectedServiceIdSet = computed(() => new Set(props.selectedServiceIds));
        const selectedCount = computed(() => {
            return props.services.filter((service) => selectedServiceIdSet.value.has(service.id)).length;
        });
        const countText = computed(() => `(${selectedCount.value}/${enabledCount.value})`);
        const allSelected = computed(() => {
            return enabledCount.value > 0 && selectedCount.value === enabledCount.value;
        });
        const partiallySelected = computed(() => {
            return selectedCount.value > 0 && selectedCount.value < enabledCount.value;
        });
        const selectAllAriaChecked = computed(() => {
            return partiallySelected.value ? "mixed" : allSelected.value;
        });

        const handleToggleDropdown = (event: MouseEvent) => {
            event.stopPropagation();

            if (props.disabled) {
                return;
            }

            isOpen.value = !isOpen.value;
        };

        const handleClickOutside = (event: MouseEvent) => {
            if (!isOpen.value) {
                return;
            }

            const target = event.target as Node;

            if (rootRef.value?.contains(target)) {
                return;
            }

            isOpen.value = false;
        };

        const handleToggleSelectAll = (checked?: boolean) => {
            if (props.disabled || enabledCount.value === 0) {
                return;
            }

            emit("toggleSelectAll", checked ?? !allSelected.value);
        };

        const handleToggleService = (serviceId: string, checked?: boolean) => {
            if (props.disabled) {
                return;
            }

            const nextChecked = checked ?? !props.selectedServiceIds.includes(serviceId);
            emit("toggleService", serviceId, nextChecked);
        };

        const setRootRef: VNodeRef = (element) => {
            rootRef.value = element as HTMLElement | null;
        };

        onMounted(() => {
            document.addEventListener("click", handleClickOutside);
        });

        onUnmounted(() => {
            document.removeEventListener("click", handleClickOutside);
        });

        const buttonText = backendStore.translate("MCP Servers");
        const selectAllText = backendStore.translate("Select All MCP Servers");

        return {
            isOpen,
            countText,
            allSelected,
            partiallySelected,
            selectAllAriaChecked,
            isEnableAdvancedCssFeatures: backendStore.isEnableAdvancedCssFeatures,
            handleToggleDropdown,
            handleToggleSelectAll,
            handleToggleService,
            setRootRef,
            buttonText,
            selectAllText,
        };
    },

    render() {
        return (
            <div ref={this.setRootRef} class="mcp-services-selector">
                <button
                    type="button"
                    class={[
                        "mcp-services-selector__trigger",
                        this.isOpen && "mcp-services-selector__trigger--open",
                        this.$props.disabled && "mcp-services-selector__trigger--disabled",
                    ]}
                    disabled={this.$props.disabled}
                    aria-haspopup="menu"
                    aria-expanded={this.isOpen}
                    onClick={this.handleToggleDropdown}
                >
                    <SvgIcon icon="icon_input_mcp" size={[16, 16]} class="mcp-services-selector__trigger-icon" />
                    <span class="mcp-services-selector__trigger-label">{this.buttonText}</span>
                    <span class="mcp-services-selector__trigger-count">{this.countText}</span>
                    <SvgIcon
                        icon="icon_arrow"
                        class={[
                            "mcp-services-selector__trigger-arrow",
                            this.isOpen && "mcp-services-selector__trigger-arrow--open",
                        ]}
                    />
                </button>

                {this.isOpen && (
                    <div
                        class={[
                            "mcp-services-selector__dropdown",
                            this.isEnableAdvancedCssFeatures && "mcp-services-selector__dropdown--advanced-css",
                        ]}
                        role="menu"
                    >
                        <div
                            class="mcp-services-selector__row mcp-services-selector__row--summary"
                            role="menuitemcheckbox"
                            aria-checked={this.selectAllAriaChecked}
                            onClick={this.handleToggleSelectAll}
                        >
                            <CheckButton
                                checked={this.allSelected}
                                indeterminate={this.partiallySelected}
                                onChange={this.handleToggleSelectAll}
                            />
                            <span class="mcp-services-selector__row-label">{this.selectAllText}</span>
                            <span class="mcp-services-selector__row-count">{this.countText}</span>
                        </div>

                        <div class="mcp-services-selector__divider" />

                        <div class="mcp-services-selector__list">
                            {this.$props.services.map((service) => {
                                const checked = this.$props.selectedServiceIds.includes(service.id);

                                return (
                                    <div
                                        key={service.id}
                                        class="mcp-services-selector__row"
                                        role="menuitemcheckbox"
                                        aria-checked={checked}
                                        onClick={() => this.handleToggleService(service.id)}
                                    >
                                        <CheckButton
                                            checked={checked}
                                            onChange={(nextChecked) =>
                                                this.handleToggleService(service.id, nextChecked)
                                            }
                                        />
                                        <span class="mcp-services-selector__service-name" title={service.name}>
                                            {service.name}
                                        </span>
                                    </div>
                                );
                            })}
                        </div>
                    </div>
                )}
            </div>
        );
    },
});
