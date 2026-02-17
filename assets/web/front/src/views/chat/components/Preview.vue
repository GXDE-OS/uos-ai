<template>
    <div class="pre-view" v-if="preView">
        <el-tooltip popper-class="uos-tooltip pre-view-tooltip" effect="light" :show-arrow="false" :enterable="false"
        :show-after="1000" :offset="2" :content="filePathHover">
            <!-- <span class="linkText" @click="openPreView">{{ filePath }}</span> -->
            <div class="fileNamePrefixAndSuffix"  @click="openPreView">
                <div class="fileName-prefix">{{fileNamePrefix}}</div>
                <div class="fileName-suffix" ref="fileNameSuffixRef" >{{fileNameSuffix}}</div>
            </div>
        </el-tooltip>
    </div>
</template>

<script setup>
import { ref, watchEffect } from "vue";
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import _ from "lodash";
import { useRouter } from "vue-router";
import { useIntervalFn } from '@vueuse/core'
const { chatQWeb, updateActivityColor, updateTheme, updateFont } = useGlobalStore()

const filePath = computed ( () => {
    return handleFileName(props.preView.docPath)
})

const filePathHover = computed ( () => {
    return props.preView.docPath.substring(props.preView.docPath.lastIndexOf('/') + 1).replace(/\.md$/i, "")  //只删除md后缀
})

const fileNamePrefix = computed ( () => {
    const fileName = filePath.value
    if (!fileName) return ''
    // 获取文件名前缀（去掉后缀）
    const lastDotIndex = fileName.lastIndexOf('.')
    if (lastDotIndex === -1) return fileName
    
    return fileName.substring(0, lastDotIndex -1)
})

const fileNameSuffix = computed ( () => {
    const fileName = filePath.value
    if (!fileName) return ''
    
    // 获取文件后缀
    const lastDotIndex = fileName.lastIndexOf('.')
    if (lastDotIndex === -1) return ''
    return fileName.substring(lastDotIndex - 1)
})

const index = computed ( () => {
    return props.index + 1
})

//去除前缀和后缀显示文件名
const handleFileName = (docPath) => {
    return index.value + '. ' + docPath.substring(docPath.lastIndexOf('/') + 1)
}

const openPreView = async () => {
    /**
     * TODO:调用后端接口，用默认应用程序打开文件
     */
    // 如果docPath以http开头，直接网页打开打开
    if (props.preView.docPath.startsWith('http')) {
        await Qrequest(chatQWeb.openUrl, props.preView.docPath)
        return;
    }
     await Qrequest(chatQWeb.openFile, props.preView.docPath)
}


const props = defineProps(['preView','index'])
</script>

<style lang="scss" scoped>

.pre-view{
    margin-left: 6px;
    max-width: 100%;
    color: var(--uosai-color-PreView-font);
    height: 30px;
    font-size: 0.93rem;
    font-weight: 400;
    font-style: normal;
    cursor: pointer;
    border-radius:8px;
    display: flex;
    align-items: center;

    .fileNamePrefixAndSuffix{
        display: flex;
        height: 30px;
        align-items: center; /* 水平方向居中 */  
        padding: 0 6px;
        flex: 1; /* 占据剩余空间 */
        min-width: 0; /* 允许收缩 */

        .fileName-prefix {
            font-weight: 400;
            white-space: nowrap; /* 不换行 */
            text-overflow: ellipsis; /* 超出部分显示省略号 */
            overflow: hidden; /* 超出部分不显示 */
            min-width: 0; /* 允许flex子元素收缩到比内容更小 */
        }

        .fileName-suffix {
            font-weight: 400;
            white-space: nowrap; /* 不换行 */
            flex-shrink: 0; /* 不允许收缩 */
        }
    }
    &:hover{
        background-color: var(--uosai-color-PreView-hover-bg);
    }

    &:active{
        background-color: var(--uosai-color-PreView-hover-bg);
        color: var(--activityColor);
    }

    
 }
</style>