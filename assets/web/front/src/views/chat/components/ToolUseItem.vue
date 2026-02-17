<template>
    <div class="tool-use-status" v-if="toolUseItem.chatType === store.ChatAction.ChatToolUse">
        <div class="tool-use-item">
            <div class="tool-header" @click="toggleExpanded">
                <div class="tool-name-wrapper">
                    <div class="expand-icon" :class="{ 'expanded': isExpanded }">
                        <SvgIcon icon="arrow_down"/>
                    </div>
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="toolUseItem.name" >
                        <div class="tool-name">{{ toolUseItem.name }}</div>
                    </el-tooltip>
                </div>
                <div class="tool-status">
                    <div class="tool-status-icon" >
                        <img :src="getStatusIcon(toolUseItem.status)" :class="{ 'rotating': toolUseItem.status === store.ToolUseStatus.Calling }"  alt="" />
                    </div>
                    <div class="tool-status-text">{{ getStatusText(toolUseItem.status) }}</div> 
                </div>
                <div class="spacer"></div>
                <el-tooltip  popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="store.loadTranslations['copy']">
                    <div class="tool-copy" v-show="toolUseItem.status != store.ToolUseStatus.Calling && toolUseItem.status != store.ToolUseStatus.Canceled" >
                        <SvgIcon icon="copy-bubble" @click.stop="copyToolUseItem(toolUseItem)" />
                    </div>
                </el-tooltip>
            </div>
            <div v-show="isExpanded" class="tool-content">
                <div class="tool-params">
                    <div class="tool-section-title">{{ store.loadTranslations["params"] }}</div>    
                    <pre>{{ toolUseItem.params }}</pre>
                </div>
                <div class="tool-result">
                    <div class="tool-section-title">{{ store.loadTranslations["result"] }}</div>
                    <pre>{{ toolUseItem.result }}</pre>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import SvgIcon from "@/components/svgIcon/svgIcon.vue";
import { useGlobalStore } from "@/store/global";
import { computed, ref, watch } from 'vue';
import { Qrequest } from "@/utils";

const { chatQWeb, updateActivityColor, updateTheme, updateFont, updateMainContentBackgroundColor} = useGlobalStore()
const emit = defineEmits(['copyToolUseItem'])

const store = useGlobalStore();
const isExpanded = ref(false);

const props = defineProps({
    toolUseItem: {
        type: Object,
        default: () => ({chatType: -1})
    },
    errCode: {
        type: Number,
        default: 0
    }
});

const toolUseItem = computed(() => {
    let toolUseItem = JSON.parse(JSON.stringify(props.toolUseItem))
    if (props.errCode == 298 && toolUseItem.status == store.ToolUseStatus.Calling) {
        toolUseItem.status = store.ToolUseStatus.Canceled
    }
    // 埋点
    if (props.errCode == 0 && toolUseItem.status == store.ToolUseStatus.Calling) {
        Qrequest(chatQWeb.mcpDataUpload, toolUseItem.name)
    }
    return toolUseItem;
});

const toggleExpanded = () => {
    isExpanded.value = !isExpanded.value;
};

const getStatusIcon = (status) => {
    switch (status) {
        case store.ToolUseStatus.Calling:
            return 'icons/mcp-loading.png';
        case store.ToolUseStatus.Completed:
            return 'icons/mcp-completed.png';
        case store.ToolUseStatus.Failed:
            return 'icons/mcp-warning.png';
        case store.ToolUseStatus.Canceled:
            return 'icons/mcp-cancal.png';
        default:
            return '';
    }
};

const getStatusText = (status) => {
    switch (status) {
        case store.ToolUseStatus.Calling:
            return store.loadTranslations["Calling"];
        case store.ToolUseStatus.Completed:
            return store.loadTranslations["Completed"];
        case store.ToolUseStatus.Failed:
            return store.loadTranslations["Call Failed"];
        case store.ToolUseStatus.Canceled:
            return store.loadTranslations["Cancelled"];
    }
};

const copyToolUseItem = (toolUseItem) => {
    let copyContent = {
        name: toolUseItem.name,
        param: toolUseItem.params,
        result: toolUseItem.result
    }

    // 需要将copyContent转为JSON字符串
    copyContent = JSON.stringify(copyContent, null, 2);

    emit('copyToolUseItem', copyContent)
};

const formatParams = (params) => {
    try {
        return JSON.stringify(JSON.parse(params), null, 2);
    } catch (e) {
        return params;
    }
};
</script>

<style lang="scss" scoped>
.tool-use-status {
    // margin: 10px 0;
    width: calc(100% + 8px);
    margin-left: -4px;

    .tool-use-item {
        border-radius: 8px;
        margin-top: 6px;
        margin-bottom: 10px;
        background-color: var(--uosai-color-tool-use-bg);
        color: var(--uosai-color-tool-use-header);

        .tool-header {
            display: flex;
            align-items: center;
            margin-bottom: 6px;
            cursor: pointer;
            user-select: none;
            width: 100%;
            height: 30px;
            margin-left: 8.5px;
            

            .tool-name-wrapper {
                display: flex;
                align-items: center;
                height: 100%;
                flex: 0 1 auto; // 改为自适应宽度，不占用剩余空间
                min-width: 0;
                
                .expand-icon {
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    padding-top: 1px;  // Adjust this value to center the image vertically
                    flex-shrink: 0;
                    svg {
                        width: 8px;
                        height: 8px;
                        margin-right: 6px;
                        transition: transform 0.2s ease;
                        transform: rotate(-90deg);
                    }

                    &.expanded {
                        svg {
                            transform: rotate(0deg);
                        }
                    }
                }

                .tool-name {
                    font-size: 13px;
                    font-weight: 500;
                    display: flex;
                    align-items: center;
                    align-self: stretch;
                    margin-bottom: 2px;
                    overflow: hidden;
                    text-overflow: ellipsis;
                    white-space: nowrap;
                    flex: 1;
                    min-width: 0;
                }
            }

            .tool-status {
                display: flex;
                align-items: center;
                white-space: nowrap;
                flex-shrink: 0; // 防止被挤压

                .tool-status-icon {
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    width: 17px;
                    height: 17px;
                    margin-left: 8px;
                    margin-right: 2px;
                    padding-top: 1px;  // Adjust this value to center the image vertically
                    img {
                        width: 16px;
                        height: 16px;

                        &.rotating {
                            animation: spin 0.77s linear infinite;
                        }

                        @keyframes spin {
                            from {
                                transform: rotate(0deg);
                            }
                            to {
                                transform: rotate(360deg);
                            }
                        }
                    }
                }
                
                .tool-status-text {
                    font-size: 11px;
                    font-weight: 400;
                    display: flex;
                    align-items: center;
                    line-height: 1;
                    height: 16px;
                }
            }

            // 添加弹性空间，让复制按钮保持在右侧
            .spacer {
                flex: 1;
            }

            .tool-copy {
                display: flex;
                align-items: center;
                justify-content: center;
                padding-top: 3px;  // Adjust this value to center the image vertically
                cursor: pointer;
                width: 16px;
                height: 16px;
                flex-shrink: 0; // 防止被挤压
                margin-left: 10px; // tool-status和tool-copy之间保持10px间距
                margin-right: 20px;

                svg {
                    fill: var(--uosai-color-clear);
                    width: 13.1px;
                    height: 13.1px;
                    cursor: pointer;

                    &:not(.disabled):hover {
                        fill: var(--uosai-color-clear-hover);
                    }

                    &:not(.disabled):active {
                        fill: var(--activityColor);
                    }
                }

            }
        }

        .tool-content {
            font-size: 0.93rem;
            font-weight: 500;
            line-height: 1.2;
            padding-bottom: 10px;


            .tool-section-title {
                margin-bottom: 6px;
            }

            .tool-params, .tool-result {   
                margin-top: 6px;    
                padding-left: 21px;
                padding-right: 10px;         
                pre {
                    background-color: var(--uosai-color-tool-use-bg);
                    border-radius: 6px;
                    padding: 10px;
                    overflow-x: auto;
                    white-space: pre-wrap;
                    word-break: break-word;
                }
            }
        }
    }
}

</style> 