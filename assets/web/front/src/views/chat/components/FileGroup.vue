<template>
  <div class="file-group-container">
    <!-- 单个图片文件:使用原来的image-container样式 -->
    <div
      v-if="isSingleImage"
      class="image-container"
      @click="handleFileClick(fileList[0])"
    >
      <img class="image-file" :src="getImageUrl(fileList[0])" alt="" />
    </div>

    <!-- 多个文件或包含文档文件:使用FileItem横向排列 -->
    <div v-else class="file-items-wrapper">
      <FileItem
        v-for="(file, index) in displayFiles"
        :key="index"
        :fileInfo="file"
        style="margin-right: 10px;"
        @click="handleFileClick(file)"
      />

      <!-- 超出3个文件时显示的收纳按钮 -->
      <div
        v-if="remainingCount > 0"
        ref="moreBtnRef"
        class="more-files-btn"
        @click.stop="togglePopover"
      >
        +{{ remainingCount }}

        <!-- 悬浮面板 -->
        <div v-if="showPopover" ref="popoverRef" class="files-popover" :class="{ 'advanced-features': store.IsEnableAdvancedCssFeatures && isWindowMode}" :style="popoverPositionStyle">
          <div
            v-for="(file, index) in hiddenFiles"
            :key="index"
            class="popover-file-item"
            @click="handlePopoverFileClick($event, file)"
          >
            <div class="popover-file-icon">
              <img :src="getFileIcon(file)" alt="file icon" />
            </div>
            <div class="popover-file-info">
              <el-tooltip popper-class="uos-tooltip document-parsing-tooltip" effect="light" :show-arrow="false" :enterable="false"
                :show-after="1000" :offset="4" :content="file.fileNameText"
                :disabled="!isTextOverflow(index)" placement="top">
                <div class="popover-file-name" ref="popoverFileNameRef">{{ file.fileNameText }}</div>
              </el-tooltip>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch, nextTick, onMounted, onBeforeUnmount } from 'vue'
import { ElTooltip } from 'element-plus'
import { useGlobalStore } from "@/store/global"
import FileItem from './FileItem.vue'
import { Qrequest } from "@/utils"

const { chatQWeb } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps({
  fileList: {
    type: Array,
    default: () => []
  },
  isDarkMode: {
    type: Boolean,
    default: false
  },
  isWindowMode: {
    type: Boolean,
    default: true
  }
})

const emit = defineEmits(['openImage'])

// popover中文件名的元素引用
const popoverFileNameRef = ref([])
const popoverTextOverflowFlags = ref([])

// 检查文本是否溢出
const isTextOverflow = (index) => {
  return popoverTextOverflowFlags.value[index] || false
}

// 检查popover文件名是否溢出
const checkPopoverTextOverflow = async () => {
  await nextTick()
  popoverTextOverflowFlags.value = []

  if (popoverFileNameRef.value) {
    popoverFileNameRef.value.forEach((element, index) => {
      if (element) {
        popoverTextOverflowFlags.value[index] = element.scrollWidth > element.clientWidth
      }
    })
  }
}

// 悬浮面板显示状态
const showPopover = ref(false)
// Popover的位置样式
const popoverPositionStyle = ref({})
// Popover元素的ref
const popoverRef = ref(null)
// 更多按钮的ref
const moreBtnRef = ref(null)

// 更新popover位置
const updatePopoverPosition = async () => {
  await nextTick()
  if (moreBtnRef.value) {
    const btnRect = moreBtnRef.value.getBoundingClientRect()
    popoverPositionStyle.value = {
      top: btnRect.bottom + 3 + 'px',
      right: (window.innerWidth - btnRect.right) + 'px'
    }
  }
}

// 监听窗口resize
const handleWindowResize = () => {
  if (showPopover.value) {
    updatePopoverPosition()
  }
}

// 判断是否为单个图片文件
const isSingleImage = computed(() => {
  return (
    props.fileList.length === 1 &&
    props.fileList[0].type === store.DocParsingFileType.Image
  )
})

// 最多显示的文件数量：侧边栏模式下只显示1个，窗口模式下显示3个
const maxDisplayCount = computed(() => {
  return props.isWindowMode ? 3 : 1
})

// 最多显示的文件
const displayFiles = computed(() => {
  return props.fileList.slice(0, maxDisplayCount.value)
})

// 剩余文件数量
const remainingCount = computed(() => {
  return Math.max(0, props.fileList.length - maxDisplayCount.value)
})

// 被隐藏的文件列表
const hiddenFiles = computed(() => {
  return props.fileList.slice(maxDisplayCount.value)
})

// 图片URL缓存
const fileImageUrls = ref({})

// 检查文件是否存在并生成URL
const checkFileExist = async (file) => {
  if (!file || file.type !== store.DocParsingFileType.Image) return

  try {
    const chatQWeb = store.chatQWeb
    file.isExist = await Qrequest(chatQWeb.isFileExist, file.filePath)
    fileImageUrls.value[file.index] = file.isExist
      ? 'file://' + file.filePath
      : props.isDarkMode
      ? 'icons/icon-image-lost-dark.svg'
      : 'icons/icon-image-lost-light.svg'
  } catch (error) {
    file.isExist = false
    fileImageUrls.value[file.index] = props.isDarkMode
      ? 'icons/icon-image-lost-dark.svg'
      : 'icons/icon-image-lost-light.svg'
  }
}

// 获取图片URL
const getImageUrl = (file) => {
  return fileImageUrls.value[file.index] || ''
}

// 处理文件点击
const handleFileClick = (file) => {
  openFile(file)
}

const openFile = async (file) => {
    await Qrequest(chatQWeb.openFile, file.filePath)
}

// 切换悬浮面板显示状态
const togglePopover = async () => {
  showPopover.value = !showPopover.value
  if (showPopover.value) {
    await updatePopoverPosition()
    await checkPopoverTextOverflow()
  }
}

// 处理悬浮面板中的文件点击
const handlePopoverFileClick = (event, file) => {
  event.stopPropagation()  // 阻止事件冒泡,避免触发handleClickOutside
  showPopover.value = false  // 先关闭面板
  openFile(file)  // 再处理文件点击
}

// 点击外部关闭悬浮面板
const handleClickOutside = (event) => {
  if (showPopover.value) {
    const target = event.target
    const popover = popoverRef.value
    const button = moreBtnRef.value

    if (popover && !popover.contains(target) && button && !button.contains(target)) {
      showPopover.value = false
    }
  }
}

// 监听滚动事件隐藏 popover
const handleScroll = () => {
  if (showPopover.value) {
    showPopover.value = false
  }
}

// 监听点击事件
onMounted(() => {
  document.addEventListener('click', handleClickOutside)
  window.addEventListener('resize', handleWindowResize)
  window.addEventListener('scroll', handleScroll, true)
})

onBeforeUnmount(() => {
  document.removeEventListener('click', handleClickOutside)
  window.removeEventListener('resize', handleWindowResize)
  window.removeEventListener('scroll', handleScroll, true)
})

// 获取文件图标
const getFileIcon = (file) => {
  if (!file || !file.imgBase64) return ''

  // 将Base64转换为Blob URL
  try {
    const byteCharacters = atob(file.imgBase64)
    const byteNumbers = new Array(byteCharacters.length)
    for (let i = 0; i < byteCharacters.length; i++) {
      byteNumbers[i] = byteCharacters.charCodeAt(i)
    }
    const byteArray = new Uint8Array(byteNumbers)
    const blob = new Blob([byteArray], { type: 'image/png' })
    return URL.createObjectURL(blob)
  } catch (e) {
    console.error('Failed to convert base64 to blob:', e)
    return ''
  }
}

// 监听isDarkMode变化,更新图片URL
watch(
  () => props.isDarkMode,
  () => {
    props.fileList.forEach((file) => {
      if (file.type === store.DocParsingFileType.Image) {
        checkFileExist(file)
      }
    })
  },
  { immediate: true }
)

// 监听fileList变化,检查图片文件
watch(
  () => props.fileList,
  (newList) => {
    newList.forEach((file) => {
      if (file.type === store.DocParsingFileType.Image) {
        checkFileExist(file)
      }
    })
  },
  { immediate: true, deep: true }
)

// 监听hiddenFiles变化,检查文本溢出
watch(
  () => hiddenFiles.value,
  async () => {
    if (showPopover.value) {
      await checkPopoverTextOverflow()
    }
  },
  { deep: true }
)
</script>

<style lang="scss" scoped>
.file-group-container {
  margin-top: 0px;
  margin-bottom: 20px;
  margin-left: auto;
  margin-right: 0px;
  width: fit-content;
  max-width: 100%;
}

.file-items-wrapper {
  display: flex;
  flex-direction: row;
  align-items: center;
  flex-wrap: nowrap;
}

.more-files-btn {
  width: 48px;
  height: 48px;
  min-width: 48px;
  min-height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  background-color: var(--uosai-color-file-item-bg);
  cursor: pointer;
  user-select: none;
  font-size: 14px;
  font-weight: 400;
  font-style: normal;
  color: var(--uosai-color-file-item-name);
  transition: background-color 0.2s;
  position: relative;

  &:hover {
    background-color: var(--uosai-color-file-item-hover-bg);
  }

  &:active {
    background-color: var(--uosai-color-file-item-press-bg);
  }
}

.files-popover {
  position: fixed;
  z-index: 1000;
  width: 330px;

  background-color: var(--uosai-color-file-list-bg);
  border-radius: 18px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
  padding: 10px;
  overflow-y: auto;

  &.advanced-features {
    background-color: var(--uosai-color-file-list-bg-qt6);
    backdrop-filter: blur(30px);
  }
}

.popover-file-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px;
  border-radius: 6px;
  cursor: pointer;
  transition: background-color 0.2s;

  &:hover {
    background-color: var(--uosai-color-file-list-item-hover-bg);
  }

  &:active {
    background-color: var(--uosai-color-file-list-item-press-bg);
  }

  &:not(:last-child) {
    margin-bottom: 4px;
  }

  .popover-file-icon {
    width: 16px;
    height: 16px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;

    img {
      width: 100%;
      height: 100%;
      object-fit: contain;
    }
  }

  .popover-file-info {
    flex: 1;
    display: flex;
    align-items: center;
    overflow: hidden;

    .popover-file-name {
      font-size: 14px;
      font-weight: 400;
      font-style: normal;
      color: var(--uosai-color-file-item-name);
      line-height: 20px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
  }
}

// 单个图片显示样式(保持原有样式)
.image-container {
  /* 基本布局属性 */
  margin-top: 0px;
  margin-bottom: 20px;
  margin-left: auto;
  margin-right: 0px;
  display: block;
  cursor: pointer;

  /* 视窗尺寸约束 - 固定容器大小范围 */
  min-width: 105px;
  max-width: 320px;
  min-height: 124px;
  max-height: 320px;

  /* 容器设置 */
  position: relative;
  overflow: hidden;
  border-radius: 8px;
  border: 1px solid var(--uosai-color-file-item-border);

  /* 让容器根据内容自适应，但受min/max约束 */
  width: max-content;
  height: max-content;
}

.image-file {
  /* 图片基本设置 */
  display: block;
  border-radius: 8px;

  /* 核心：图片适应策略 */
  object-fit: cover;
  object-position: top left;

  /* 图片尺寸设置 - 关键逻辑 */
  min-width: 105px;
  min-height: 124px;
  max-width: 320px;
  max-height: 320px;

  /* 让图片填充容器 */
  width: 100%;
  height: 100%;
}
</style>
