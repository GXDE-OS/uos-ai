import { defineComponent, computed, type PropType } from "vue";
import SvgIcon from "@/components/SvgIcon";
import ScrollBar from "@/components/ScrollBar";
import type { ModelOption, ModelOptionGroup } from "@/types/combobox";
import { MODEL_NETWORK_ICON_MAP } from "@/types/model";
import "@/assets/styles/window/mainwindow/titlebar/components/ModelSelectorDropdown.css";
import { useBackendStore } from "@/stores/backend";

// 分组配置
const GROUP_CONFIG: Record<ModelOptionGroup, { labelKey: string; icon: string }> = {
    recommended: { labelKey: "Smart Recommendation", icon: "icon_model_intel" },
    online: { labelKey: "Online Model", icon: MODEL_NETWORK_ICON_MAP.online },
    local: { labelKey: "Local Model", icon: MODEL_NETWORK_ICON_MAP.local },
    private: { labelKey: "Private Model", icon: MODEL_NETWORK_ICON_MAP.private },
};

type GroupKey = keyof typeof GROUP_CONFIG;

export default defineComponent({
    name: "ModelSelectorDropdown",
    props: {
        isOpen: {
            type: Boolean,
            required: true,
        },
        options: {
            type: Array as PropType<ModelOption[]>,
            required: true,
        },
        selectedValue: {
            type: [String, Number] as PropType<string | number>,
            required: true,
        },
    },
    emits: {
        select: (option: ModelOption) => true,
        close: () => true,
        addModel: () => true,
    },
    setup(props, { emit }) {
        const backend = useBackendStore();
        const groupConfig = computed(() => ({
            recommended: {
                label: backend.translate(GROUP_CONFIG.recommended.labelKey),
                icon: GROUP_CONFIG.recommended.icon,
            },
            online: {
                label: backend.translate(GROUP_CONFIG.online.labelKey),
                icon: GROUP_CONFIG.online.icon,
            },
            local: {
                label: backend.translate(GROUP_CONFIG.local.labelKey),
                icon: GROUP_CONFIG.local.icon,
            },
            private: {
                label: backend.translate(GROUP_CONFIG.private.labelKey),
                icon: GROUP_CONFIG.private.icon,
            },
        }));

        // 将选项按分组分类
        const groupedOptions = computed(() => {
            const groups: Record<GroupKey, ModelOption[]> = {
                recommended: [],
                online: [],
                private: [],
                local: [],
            };

            props.options.forEach((option) => {
                const group = option.group || "online";
                if (group in groups) {
                    groups[group as GroupKey].push(option);
                } else {
                    groups.online.push(option);
                }
            });

            return groups;
        });

        // 获取有选项的分组列表
        const activeGroups = computed(() => {
            return Object.keys(GROUP_CONFIG).filter((key) => {
                const groupKey = key as GroupKey;
                return groupedOptions.value[groupKey].length > 0;
            }) as GroupKey[];
        });

        const handleItemClick = (option: ModelOption) => {
            emit("select", option);
        };

        const handleAddModel = () => {
            emit("addModel");
        };

        const addButtonText = computed(() => backend.translate("Add Model"));
        const defaultTitle = computed(() => backend.translate("Model List"));
        const officialText = computed(() => backend.translate("Official"));

        return {
            isEnableAdvancedCssFeatures: backend.$state.isEnableAdvancedCssFeatures,
            groupConfig,
            groupedOptions,
            activeGroups,
            addButtonText,
            defaultTitle,
            officialText,
            handleItemClick,
            handleAddModel,
        };
    },
    render() {
        if (!this.$props.isOpen) return null;
        return (
            <div
                class={[
                    "model-selector-dropdown",
                    this.isEnableAdvancedCssFeatures && "model-selector-dropdown--advanced-css",
                ]}
            >
                {/* 标题栏 */}
                <div class="model-selector-header">
                    <span class="model-selector-header__title">{this.defaultTitle}</span>
                    <button class="model-selector-header__add-btn" onClick={this.handleAddModel}>
                        <span class="model-selector-header__add-text">{this.addButtonText}</span>
                    </button>
                </div>

                {/* 分组列表 */}
                <ScrollBar class="model-selector-dropdown__scroll">
                    {this.activeGroups.map((groupKey) => {
                        const config = this.groupConfig[groupKey];
                        const groupOptions = this.groupedOptions[groupKey];

                        return (
                            <div key={groupKey} class="model-selector-group">
                                {/* 分组标题 */}
                                <div class="model-selector-group__header">
                                    <SvgIcon icon={config.icon} class="model-selector-group__icon" />
                                    <span class="model-selector-group__label">{config.label}</span>
                                </div>

                                {/* 分组选项 */}
                                {groupOptions.map((option) => {
                                    const isSelected = option.value === this.$props.selectedValue;

                                    return (
                                        <div
                                            key={String(option.value)}
                                            class={[
                                                "model-selector-item",
                                                isSelected && "model-selector-item--selected",
                                            ]}
                                            onClick={() => this.handleItemClick(option)}
                                        >
                                            {isSelected && (
                                                <SvgIcon icon="icon_selected" class="model-selector-item__check-icon" />
                                            )}
                                            <span class="model-selector-item__label">
                                                {option.label || String(option.value)}
                                            </span>
                                            {option.provider === "uos_free" && (
                                                <span class="model-selector-item__official-badge">
                                                    <span class="model-selector-item__official-badge-text">
                                                        {this.officialText}
                                                    </span>
                                                </span>
                                            )}
                                        </div>
                                    );
                                })}
                            </div>
                        );
                    })}
                </ScrollBar>
            </div>
        );
    },
});
