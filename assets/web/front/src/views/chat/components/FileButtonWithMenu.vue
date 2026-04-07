<template>
    <div class="file-button-wrapper">
        <el-tooltip
            popper-class="uos-tooltip"
            effect="light"
            :show-arrow="false"
            :enterable="false"
            :show-after="1000"
            :offset="2"
            :content="tooltipContent"
            :class="{ disabled: !isDisabled }"
        >
            <div
                class="file-btn btn"
                ref="btnRef"
                :class="{ disabled: isDisabled }"
                :style="{ 'margin-right': isWindowMode ? '15px' : '10px' }"
                @mousedown="handleMouseDown"
                @click="handleClick"
            >
                <SvgIcon icon="file" />
            </div>
        </el-tooltip>
        <InputFilesMenu ref="inputFilesMenuRef" :show="showMenu" @select="handleSelect" />
    </div>
</template>

<script setup>
import { ref, computed } from "vue";
import { useGlobalStore } from "@/store/global";
import SvgIcon from "@/components/svgIcon/svgIcon.vue";
import InputFilesMenu from "./InputFilesMenu.vue";

const store = useGlobalStore();

const props = defineProps({
    disabled: {
        type: Boolean,
        default: false,
    },
    isWindowMode: {
        type: Boolean,
        default: true,
    },
    inputFileListLength: {
        type: Number,
        default: 0,
    },
    isShowGenContentBtn: {
        type: Boolean,
        default: false,
    },
    currentAssistant: {
        type: Object,
        default: () => ({}),
    },
    materialFilesLength: {
        type: Number,
        default: 0,
    },
    outlineFilesLength: {
        type: Number,
        default: 0,
    },
});

const emit = defineEmits(["selectFile", "selectFiles"]);

const btnRef = ref(null);
const inputFilesMenuRef = ref(null);
const showMenu = ref(false);
const isClickingFileBtn = ref(false);

// 计算按钮是否禁用
const isDisabled = computed(() => {
    // 写作助手：检查本地素材和文件大纲的限制
    if (props.currentAssistant.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        // 如果显示生成内容按钮，禁用
        if (props.isShowGenContentBtn) {
            return true;
        }
        // 如果本地素材已满（10个）或文件大纲已满（1个），禁用
        if (props.materialFilesLength >= 10 && props.outlineFilesLength >= 1) {
            return true;
        }
        return false;
    }

    // 其他助手类型
    return (
        props.disabled ||
        props.inputFileListLength === 3 ||
        (props.currentAssistant.type == store.AssistantType.PLUGIN_ASSISTANT &&
            props.currentAssistant.id != "PPT Assistant")
    );
});

// 计算提示文案
const tooltipContent = computed(() => {
    if (props.currentAssistant.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        if (props.materialFilesLength >= 10 && props.outlineFilesLength >= 1) {
            return store.loadTranslations["Only supports uploading 1 outline file and up to 10 local materials"];
        }
        if (props.materialFilesLength >= 10) {
            return store.loadTranslations["Supports uploading up to 10 local materials"];
        }
        if (props.outlineFilesLength >= 1) {
            return store.loadTranslations["Only supports uploading 1 outline file"];
        }
        return store.loadTranslations["Upload Files"];
    }

    if (props.inputFileListLength === 3) {
        return store.loadTranslations["You can upload up to 3 files or image"];
    }
    return store.loadTranslations["Upload Files"];
});

const handleMouseDown = () => {
    isClickingFileBtn.value = true;
};

const handleClick = () => {
    if (isDisabled.value) return;

    // 如果当前是writing assistant，显示菜单
    if (props.currentAssistant.type === store.AssistantType.AI_WRITING_ASSISTANT) {
        showMenu.value = !showMenu.value;
        isClickingFileBtn.value = false;
        return;
    }

    // 其他情况，触发文件选择
    emit("selectFile");
};

const handleSelect = (fileCategory) => {
    showMenu.value = false;
    emit("selectFiles", fileCategory);
};

// 暴露给父组件的方法
defineExpose({
    btnRef,
    inputFilesMenuRef,
    closeMenu: () => {
        showMenu.value = false;
    },
    isClickingFileBtn,
});
</script>

<style lang="scss" scoped>
.file-button-wrapper {
    position: relative;
    display: inline-flex;
    align-items: center;
}

.file-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 30px;
    height: 30px;
    border-radius: 50%;
    background-color: var(--uosai-color-voicebtn-bg);
    cursor: pointer;
    transition: background-color 0.2s ease;
    margin-right: 10px;

    &:not(.disabled):hover {
        background-color: var(--uosai-color-voicebtn-bg-hover);
    }

    &:not(.disabled):active {
        background-color: var(--uosai-color-voicebtn-bg-active);

        svg {
            fill: var(--uosai-color-voicebtn-bg-active-color);
        }
    }

    svg {
        height: 20px;
        width: 20px;
        fill: var(--uosai-color-flat-btn-icon);
    }
}

/* 修改 InputFilesMenu 的定位方式，使用 absolute 替代 fixed */
:deep(.input-files-menu) {
    position: absolute;
    right: 0;
    bottom: calc(100% + 8px);
    left: auto;
    top: auto;
}
</style>
