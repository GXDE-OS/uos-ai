<template>
    <div class="documentParsing" 
        v-show="true"
        @mouseover="handleMouseOver"
        @mouseleave="handleMouseLeave"
        @click="openFile"
        :style="{ maxWidth: '100%' }">
        <!-- 内容区域 -->
        <div class="documentParsingContent" ref="documentParsingContentRef">
            <div class="fileTitle" v-show="isShowTitle">{{ store.loadTranslations['Outline:'] }}</div> 
            <div class="fileIcon" ref="fileIconRef">
                <img :src="imageUrl" alt="" :imgBase64="imgBase64">
            </div>
            <el-tooltip popper-class="uos-tooltip document-parsing-tooltip" effect="light" :show-arrow="false" :enterable="false"
                :show-after="1000" :offset="2" :content="fileNameText">
                <div class="fileNamePrefixAndSuffix">
                    <div class="fileName-prefix">{{fileNamePrefix}}</div>
                    <div class="fileName-suffix" ref="fileNameSuffixRef" >{{fileNameSuffix}}</div>
                </div>
            </el-tooltip>
            <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                :show-after="1000" :offset="2" :content="parsingStatusText"  v-show="isShowParsingStatus">
                <div class="parsingStatus" v-show="isShowParsingStatus" ref="parsingStatusRef">
                    <img :src="isParsingStatusEnd?'icons/warning.svg':'icons/loading.svg'"  :class="{ 'rotating': !isParsingStatusEnd }"  alt=""  class="parsingStatusIcon"/>
                    <span class="parsingStatusText" :style="{ color: isParsingStatusEnd ? 'rgba(255, 87, 54, 1)' : 'var(--uosai-color-document-parsing-text)' }" v-text="parsingStatusText"></span>
                </div>
            </el-tooltip>
            
        </div>
        <!-- 删除按钮 -->
        <div  class="documentParsingDelete-btn" @click="handleDelete" v-show="isShowDelete">
            <img :src="isDarkMode? 'icons/file-delete-dark.svg':'icons/file-delete-light.svg'"  class="deleteIcon"/>
        </div>
    </div>
</template>

<script setup>
import { ref, watch, computed, onMounted, onBeforeUnmount, nextTick } from "vue";
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import _ from "lodash";

const { chatQWeb, updateActivityColor, updateTheme, updateFont } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps({
    fileInfo: Object,
    isWindowMode: Boolean,
    isDarkMode: Boolean
})
const emit = defineEmits(['setFileParam', 'sigDeleteFile'])

// const isEnabledMouthOver=ref(false)  //是否启用鼠标悬停事件
const isShowFile = ref(true)
const isShowDelete=ref(false);
const imageUrl = ref(''); // 用于存储图片的URL


//是否启用鼠标悬停事件 不知道这个用来干嘛的
const isEnabledMouthOver = computed(() => {
    return props.fileInfo.isEnabledMouthOver
})
const isShowParsingStatus = computed(() => {
    return props.fileInfo.isShowParsingStatus
})
const isParsingStatusEnd = computed(() => {
    return props.fileInfo.isParsingStatusEnd
})
const parsingStatusText = computed(() => {
    return props.fileInfo.parsingStatusText
})

const fileNameText = computed(() => {
    return props.fileInfo.fileNameText
})

const filePath = computed(() => {
    return props.fileInfo.filePath
})

const imgBase64 = computed(() => {
    setFileIcon(props.fileInfo.imgBase64)
})

const isWindowMode = computed(() => {  //是否为窗口模式
    return props.isWindowMode
})

const setFileIcon = (imgBase64_) =>{
    const blob = base64ToBlob(imgBase64_);
    imageUrl.value = createObjectURL(blob);
}

const isShowTitle = computed(() => {
    return props.fileInfo.fileCategory === store.DocFileCategory.FileOutline
})                          

// 将Base64字符串转换为Blob
function base64ToBlob(base64) {
    const byteCharacters = atob(base64);
    const byteNumbers = new Array(byteCharacters.length);
    for (let i = 0; i < byteCharacters.length; i++) {
        byteNumbers[i] = byteCharacters.charCodeAt(i);
    }
    const byteArray = new Uint8Array(byteNumbers);
    return new Blob([byteArray], { type: 'image/png' });
}

// 创建Object URL
function createObjectURL(blob) {
  return URL.createObjectURL(blob);
}

const handleMouseOver = (event) => {
  if(isEnabledMouthOver.value){
    isShowDelete.value = true
  }
}
const handleMouseLeave = (event) => {
  isShowDelete.value = false
  // 在这里可以执行你想要在鼠标离开时进行的操作
}

const handleDelete = async(event) => {
    event.stopPropagation();  //阻止事件冒泡
    isShowFile.value = false
    emit('sigDeleteFile', props.fileInfo.index)
    // await Qrequest(chatQWeb.onDocSummaryRemove, filePath.value)
}

const openFile = async () => {
    await Qrequest(chatQWeb.openFile, filePath.value)
}

const fileNamePrefix = computed(() => {
    const fileName = props.fileInfo.fileNameText
    if (!fileName) return ''
        
    // 获取文件名前缀（去掉后缀）
    const lastDotIndex = fileName.lastIndexOf('.')
    if (lastDotIndex === -1) return fileName
    
    return fileName.substring(0, lastDotIndex -1)
})

const fileNameSuffix = computed(() => {
    const fileName = props.fileInfo.fileNameText
    if (!fileName) return ''
    
    // 获取文件后缀
    const lastDotIndex = fileName.lastIndexOf('.')
    if (lastDotIndex === -1) return ''
    return fileName.substring(lastDotIndex - 1)
})

const fileNameContainer = ref(null)
const fileIconRef = ref(null)
const parsingStatusRef = ref(null)
const fileNameSuffixRef = ref(null)
const documentParsingContentRef = ref(null)

// 计算各元素的宽度
const calculateElementWidths = () => {
    if (!documentParsingContentRef.value) return { fileIcon: 0, suffix: 0, parsingStatus: 0 }
    
    let fileIconWidth = 0
    let suffixWidth = 0
    let parsingStatusWidth = 0
    
    // 计算fileIcon宽度 (16px + margin: 7px left + 7px right)
    fileIconWidth = 30
    
    // 计算后缀宽度
    if (fileNameSuffixRef.value) {
        suffixWidth = fileNameSuffixRef.value.offsetWidth
    } else {
        // 如果没有ref，用字体大小估算
        const suffix = fileNameSuffix.value
        suffixWidth = suffix.length * sysFontPixelSize.value * 0.6 // 估算字符宽度
    }
    
    // 计算解析状态宽度
    if (isShowParsingStatus.value && parsingStatusRef.value) {
        parsingStatusWidth = parsingStatusRef.value.offsetWidth
    }
    
    return { fileIcon: fileIconWidth, suffix: suffixWidth, parsingStatus: parsingStatusWidth }
}

// 重新计算ileNamePrefixMaxWidth
const recalculateLayout = () => {
    nextTick(() => {
        if (!fileNameContainer.value || !fileNameText.value || !documentParsingContentRef.value) {
            return
        }
        
        const containerWidth = fileNameContainer.value.offsetWidth
        const contentWidth = documentParsingContentRef.value.offsetWidth
        const fileName = fileNameText.value
        
        // 使用canvas测量文本宽度，更准确
        const getTextWidth = (text, fontSize = sysFontPixelSize.value) => {
            const canvas = document.createElement('canvas')
            const context = canvas.getContext('2d')
            context.font = `${fontSize}px var(--font-family, 'PingFangSC-Regular')`
            return context.measureText(text).width
        }
        
        // 计算完整文件名的宽度
        const fullNameWidth = getTextWidth(fileName)
        
        // 计算其他元素的宽度
        const { fileIcon, suffix, parsingStatus } = calculateElementWidths()
        
        // 计算margins和padding
        const fileNameMarginRight = 11
        const otherMargins = 20
        const totalOtherWidth = fileIcon + parsingStatus + fileNameMarginRight + otherMargins
        
        // 计算可用的总宽度（根据parsingStatus的显示状态）
        let availableContainerWidth = contentWidth
        if (isShowParsingStatus.value) {
            // 如果显示解析状态，需要减去parsingStatus的宽度
            availableContainerWidth = contentWidth - parsingStatus - 10 // 10px为parsingStatus的margin
        }
    })
}

watch(fileNameText, () => {
    if (fileNameText.value) {
        recalculateLayout()
    }
}, { immediate: true })

watch(fileNameContainer, () => {
    if (fileNameContainer.value && fileNameText.value) {
        recalculateLayout()
    }
}, { immediate: true })

// 监听isShowParsingStatus变化，重新计算布局
watch(isShowParsingStatus, () => {
    recalculateLayout()
}, { immediate: true })

// 监听窗口大小变化
const handleResize = () => {
    recalculateLayout()
}

//等待渲染完成
onMounted(async () => {
    // 添加窗口大小变化监听
    window.addEventListener('resize', handleResize)
    // isWindowMode.value = await Qrequest(chatQWeb.isWindowMode);
    
})

onBeforeUnmount(() => {
    // 移除窗口大小变化监听
    window.removeEventListener('resize', handleResize)
});
</script>

<style lang="scss" scoped>
    .documentParsing{
        position: relative; /* 为按钮定位创建一个相对定位的上下文 */
        left: 10px;
        top: 8px;
        width: fit-content;
        height: 30px;
        background-color: var(--uosai-color-document-parsing-bg);
        align-items: center; /* 水平方向居中 */  
        border-radius: 8px;  /* 圆角 */
        cursor: pointer;
        user-select: none;

        /* 添加伪元素用于扩大hover区域 */
        &::before {
            content: '';
            position: absolute;
            top: -4px;
            left: -4px;
            right: -4px;
            bottom: -4px;
            z-index: 0;
            pointer-events: auto;
        }

        /* 确保内容在伪元素之上 */
        .documentParsingContent {
            position: relative;
            z-index: 1;
            display: flex;
            align-items: center; /* 水平方向居中 */  
            height: 30px;
            max-width: 100%;
            min-width: none;
            text-overflow: ellipsis;/* 超出部分显示省略号 */
            overflow: hidden;/* 超出部分不显示 */
            
            .fileTitle{
                display: flex;
                height: 30px;
                align-items: center; /* 水平方向居中 */  
                font-size: 1rem;
                font-family: var(--font-family);
                font-weight: 400;
                white-space: nowrap; /* 不换行 */
                text-overflow: ellipsis;/* 超出部分显示省略号 */
                overflow: hidden;/* 超出部分不显示 */
                margin-left: 7px;
                color: var(--uosai-color-document-file-name-text);
            }
            
            .fileIcon{
                display: grid;
                width: 16px;
                height: 16px;
                flex-shrink: 0; /* 防止被flex布局挤压 */
                align-items: center; /* 水平方向居中 */  
                justify-content: center; /* 垂直方向居中 */
                margin-left: 7px;
                margin-right: 7px;
            }

            .fileIcon img {
                max-width: 100%;
                max-height: 100%;
                object-fit: cover; /* 根据需要选择 none | contain | cover | fill */
            }

            .fileName{
                display: flex;
                height: 30px;
                align-items: center; /* 水平方向居中 */  
                font-size: 1rem;
                font-family: var(--font-family);
                font-weight: 400;
                white-space: nowrap; /* 不换行 */
                text-overflow: ellipsis;/* 超出部分显示省略号 */
                overflow: hidden;/* 超出部分不显示 */
                margin-right: 11px;
                flex: 1; /* 占据剩余空间 */
                min-width: 0; /* 允许收缩 */
                color: var(--uosai-color-document-file-name-text);

                .fileNameText{
                    white-space: nowrap; /* 不换行 */
                    text-overflow: ellipsis;/* 超出部分显示省略号 */
                    overflow: hidden;/* 超出部分不显示 */
                }
            }

            .fileNamePrefixAndSuffix{
                display: flex;
                height: 30px;
                align-items: center; /* 水平方向居中 */  
                font-size: 1rem;
                margin-right: 11px;
                flex: 1; /* 占据剩余空间 */
                min-width: 0; /* 允许收缩 */
                color: var(--uosai-color-document-file-name-text);
                
                .fileName-prefix {
                    font-weight: 400;
                    white-space: nowrap; /* 不换行 */
                    text-overflow: ellipsis; /* 超出部分显示省略号 */
                    overflow: hidden; /* 超出部分不显示 */
                    flex: 1; /* 占据剩余空间 */
                    min-width: 0; /* 允许flex子元素收缩到比内容更小 */
                }
                
                .fileName-suffix {
                    font-weight: 400;
                    white-space: nowrap; /* 不换行 */
                    flex-shrink: 0; /* 不允许收缩 */
                }
            }

            .parsingStatus{
                display: flex;
                height: 30px;
                align-items: center; /* 水平方向居中 */  
                margin-right: 10px;
                margin-left: auto;
                flex-shrink: 0; /* 不允许收缩 */

                font-size: 1rem;
                font-family: var(--font-family);
                font-weight: 400;
                white-space: nowrap; /* 不换行 */
                overflow: hidden;/* 超出部分不显示 */
                text-overflow: ellipsis;/* 超出部分显示省略号 */

                .parsingStatusIcon{
                    min-width: 16px;
                    min-height: 16px; 
                }

                img {
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

                .parsingStatusText{
                    margin-left: 7px;
                    font-size: 1rem;
                    font-family: var(--font-family);
                    font-weight: 400;
                    white-space: nowrap; /* 不换行 */
                    overflow: hidden;/* 超出部分不显示 */
                    text-overflow: ellipsis;/* 超出部分显示省略号 */
                }
            }
        }

        .documentParsingDelete-btn{
            display: flex;
            position: absolute; /* 绝对定位按钮 */
            top: 0; /* 将按钮顶部对齐到容器的顶部 */
            right: 0; /* 将按钮右侧对齐到容器的右侧 */
            cursor: pointer; /* 鼠标悬停时显示指针 */
            transform: translate(50%, -50%); /* 使用transform将子div的中心点移动到右上角 */
            padding: 3px; /* 添加2px的缓冲区域，防止误触 */

        }
    }
</style>