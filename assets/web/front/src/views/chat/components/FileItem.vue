<template>
  <div class="file-item">
    <div class="file-icon">
      <img :src="imageUrl" alt="file icon" />
    </div>
    <div class="file-info">
      <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                    :show-after="1000" :offset="2" :content="fileNameText" :disabled="!isNameTruncated" placement="top">
        <div ref="fileNameRef" class="file-name">{{ fileNameText }}</div>
      </el-tooltip>
      <div class="file-type">{{ fileTypeName }}</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { ElTooltip } from 'element-plus'
import { useGlobalStore } from "@/store/global"
import { Qrequest } from "@/utils"

const { chatQWeb } = useGlobalStore()
const store = useGlobalStore()
const props = defineProps({
    fileInfo: Object
})

const imageUrl = ref('')
const fileNameRef = ref<HTMLElement>()

// 文件名是否被截断
const isNameTruncated = computed(() => {
  if (!fileNameRef.value) return false
  return fileNameRef.value.scrollWidth > fileNameRef.value.clientWidth
})

// 文件名
const fileNameText = computed(() => {
  return props.fileInfo.fileNameText || ''
})

// 文件类型(从文件名提取后缀)
const fileTypeName = computed(() => {
  const fileName = fileNameText.value
  if (!fileName) return ''

  const lastDotIndex = fileName.lastIndexOf('.')
  if (lastDotIndex === -1) return ''

  const ext = fileName.substring(lastDotIndex + 1).toLowerCase()

  // 根据文件扩展名返回对应的显示名称
  const typeMap: Record<string, string> = {
    'docx': 'Word',
    'doc': 'Word',
    'wps': 'WPS',
    'dps': 'WPS',
    'et': 'WPS',
    'pptx': 'PPT',
    'ppt': 'PPT',
    'xlsx': 'Excel',
    'xls': 'Excel',
    'csv': 'CSV',
    'pdf': 'PDF',
    'md': 'Markdown',
    'txt': store.loadTranslations['Text document'],   // 文本文档
    'rtf': store.loadTranslations['Text document']
  }

  // 如果在映射表中找到,返回对应的名称;否则直接返回小写的扩展名
  return typeMap[ext] || ext
})

// 将Base64字符串转换为Blob
function base64ToBlob(base64: string): Blob {
  const byteCharacters = atob(base64)
  const byteNumbers = new Array(byteCharacters.length)
  for (let i = 0; i < byteCharacters.length; i++) {
    byteNumbers[i] = byteCharacters.charCodeAt(i)
  }
  const byteArray = new Uint8Array(byteNumbers)
  return new Blob([byteArray], { type: 'image/png' })
}

// 创建Object URL
function createObjectURL(blob: Blob): string {
  return URL.createObjectURL(blob)
}

// 设置文件图标
const setFileIcon = (imgBase64: string) => {
  if (!imgBase64) return
  const blob = base64ToBlob(imgBase64)
  imageUrl.value = createObjectURL(blob)
}

// 监听imgBase64变化
watch(() => props.fileInfo.imgBase64, (newVal) => {
  if (newVal) {
    setFileIcon(newVal)
  }
}, { immediate: true })
</script>

<style scoped lang="scss">
.file-item {
  width: 240px;
  height: 48px;
  display: flex;
  align-items: center;
  gap: 8px;
  // padding: 6px 10px;
  border-radius: 8px;
  cursor: pointer;
  transition: background-color 0.2s;
  background-color: var(--uosai-color-file-item-bg);

  &:hover {
    background-color: var(--uosai-color-file-item-hover-bg);
  }

  &:active {
    background-color: var(--uosai-color-file-item-press-bg);
  }

  .file-icon {
    width: 24px;
    height: 24px;
    padding-left: 10px;
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

  .file-info {
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 0px;
    overflow: hidden;

    .file-name {
      font-size: 14px;
      font-weight: 400;
      font-style: normal;
      color: var(--uosai-color-file-item-name);
      line-height: 20px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      padding-right: 10px;
    }

    .file-type {
      font-size: 12px;
      font-weight: 400;
      font-style: normal;
      color: var(--uosai-color-file-item-type);
      line-height: 18px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
  }
}
</style>
