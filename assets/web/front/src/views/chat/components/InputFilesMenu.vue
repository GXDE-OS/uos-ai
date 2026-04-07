<template>
    <div class="input-files-menu" v-show="show" @click.stop ref="inputFilesMenuRef">
        <div class="menu-action" @click="handleSelectMaterials" style="margin-top: 8px">
            <div class="action-icon">
                <SvgIcon icon="local-materials" />
            </div>
            <span class="action-text">{{ store.loadTranslations["Local Materials"] }}</span>
        </div>
        <div class="menu-action" @click="handleSelectFileOutline">
            <div class="action-icon">
                <SvgIcon icon="file-outline" />
            </div>
            <span class="action-text">{{ store.loadTranslations["File Outline"] }}</span>
        </div>
    </div>
</template>

<script setup>
import { ref } from "vue";
import { useGlobalStore } from "@/store/global";
import SvgIcon from "@/components/svgIcon/svgIcon.vue";

const store = useGlobalStore();
const props = defineProps({
    show: {
        type: Boolean,
        default: false,
    },
});

const emit = defineEmits(["select"]);
const inputFilesMenuRef = ref(null);

const handleSelectMaterials = () => {
    emit("select", store.DocFileCategory.LocalMaterial);
};

const handleSelectFileOutline = () => {
    emit("select", store.DocFileCategory.FileOutline);
};
</script>

<style lang="scss" scoped>
.input-files-menu {
    position: absolute;
    right: 0;
    bottom: calc(100% + 8px);
    width: 162px;
    height: 84px;
    background-color: var(--uosai-color-conversion-mode-bg);
    border-radius: 18px;
    box-shadow:
        0 0 0 1px rgba(0, 0, 0, 0.05),
        0 6px 20px 0 rgba(0, 0, 0, 0.2);
    z-index: 1000;
    display: flex;
    flex-direction: column;
    user-select: none;
    overflow: hidden;

    .menu-action {
        display: flex;
        align-items: center;
        height: 34px;
        cursor: pointer;
        transition: background-color 0.2s ease;

        .action-icon {
            width: 20px;
            height: 20px;
            margin-left: 8px;
            margin-right: 4px;
            margin-top: 1px;
            display: flex;
            align-items: center;
            justify-content: center;

            svg {
                width: 12px;
                height: 14px;
                fill: var(--uosai-color-conversion-mode-icon);
            }
        }

        .action-text {
            font-size: 1rem;
            font-weight: 500;
            color: var(--uosai-color-conversion-mode-text);
            user-select: none;
        }

        &:hover {
            background-color: var(--activityColor);
            svg {
                fill: #fff;
            }
            .action-text {
                color: #fff;
            }
        }
    }
}

.dark {
    .input-files-menu {
        box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.3);
    }
}
</style>
