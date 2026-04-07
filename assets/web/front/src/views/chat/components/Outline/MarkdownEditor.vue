<template>
    <div class="markdown-editor">
        <div class="top">
            <!-- 左组：导航和状态 -->
            <div class="toolbar-group toolbar-left">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Return to conversation']">
                    <div class="toolbar-btn" :class="props.isWindowMode ? 'with-text' : 'icon-only'" @click="closeMarkdownEditor">
                        <SvgIcon class="toolbar-icon back-icon" icon="arrow_left" color="currentColor" />
                        <span class="toolbar-text" v-if="props.isWindowMode">{{ store.loadTranslations['Return to conversation'] || 'Back to Chat' }}</span>
                    </div>
                </el-tooltip>
            </div>

            <!-- 中组：编辑功能 -->
            <div class="toolbar-group toolbar-center" :class="{ 'force-center': props.isWindowMode }">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Undo']">
                    <div class="toolbar-btn icon-only"
                        :class="{ disabled: !canUndo }"
                        @click="handleUndo">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-undo" color="currentColor" />
                    </div>
                </el-tooltip>
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Redo']">
                    <div class="toolbar-btn icon-only"
                        :class="{ disabled: !canRedo }"
                        @click="handleRedo">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-redo" color="currentColor" />
                    </div>
                </el-tooltip>
                <div class="toolbar-divider"></div>
                <div class="heading-btn-wrapper" style="position: relative;">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                        :enterable="false" :show-after="1000" :offset="2" :content="currentHeadingMenuLabel">
                        <div class="toolbar-btn icon-only"
                             @mouseenter="toggleHeadingMenu"
                             @mouseleave="handleHeadingBtnMouseLeave"
                             :class="{ checked: showHeadingMenu }">
                            <SvgIcon class="toolbar-icon" :icon="currentHeadingIcon" color="currentColor" />
                            <SvgIcon class="h-downarrow" icon="mdeditor-h-downarrow" color="currentColor" />
                        </div>
                    </el-tooltip>
                    <Menu
                        :visible="showHeadingMenu"
                        :items="headingMenuItems"
                        position="bottom-left"
                        @select="handleHeadingSelect"
                        @mouseenter="cancelHeadingMenuClose"
                        @mouseleave="handleHeadingMenuMouseLeave"
                    />
                </div>

                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Bold']">
                    <div class="toolbar-btn icon-only"
                        v-if="props.isWindowMode"
                        :class="{ checked: isBold }"
                        @click.prevent="handleBold">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-bold" color="currentColor" />
                    </div>
                </el-tooltip>
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Unordered list'] || 'Unordered List'">
                    <div class="toolbar-btn icon-only"
                        v-if="props.isWindowMode"
                         :class="{ checked: isUnorderedList }"
                         @click.prevent="handleUnorderedList">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-unordered" color="currentColor" />
                    </div>
                </el-tooltip>
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Ordered list'] || 'Ordered List'">
                    <div class="toolbar-btn icon-only"
                        v-if="props.isWindowMode"
                         :class="{ checked: isOrderedList }"
                         @click.prevent="handleOrderedList">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-ordered" color="currentColor" />
                    </div>
                </el-tooltip>

                <div class="more-btn-wrapper" style="position: relative;" ref="moreBtnWrapper" v-if="!props.isWindowMode">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                        :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['More'] || 'More'">
                        <div class="toolbar-btn icon-only"
                            :class="{ checked: showMoreMenu }"
                            @click="toggleShowMoreMenu">
                            <SvgIcon class="toolbar-icon" icon="mdeditor-more" color="currentColor" />
                        </div>
                    </el-tooltip>
                    <Menu
                        ref="moreMenuRef"
                        :visible="showMoreMenu"
                        :items="moreMenuItems"
                        position="bottom-left"
                        :offsetX="moreMenuOffsetX"
                        @select="handleMoreSelect"
                    />
                </div>
            </div>

            <!-- 右组：导出功能 -->
            <div class="toolbar-group toolbar-right">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Copy']">
                    <div class="toolbar-btn"
                        :class="[{ disabled: !hasContent }, props.isWindowMode ? 'with-text' : 'icon-only']"
                        @click="handleCopy">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-copy" color="currentColor" />
                        <span class="toolbar-text" v-if="props.isWindowMode">{{ store.loadTranslations['Copy'] || 'Copy' }}</span>
                    </div>
                </el-tooltip>
                <div class="download-btn-wrapper" style="position: relative;">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Save As']">
                        <div class="toolbar-btn"
                            :class="[{ disabled: !hasContent }, props.isWindowMode ? 'with-text' : 'icon-only']"
                            @click="handleDownload">
                            <SvgIcon class="toolbar-icon" icon="mdeditor-save-as" color="currentColor" />
                            <span class="toolbar-text" v-if="props.isWindowMode">{{ store.loadTranslations['Save As'] || 'Save as' }}</span>
                        </div>
                    </el-tooltip>
                    <DownloadCard v-if="showDownloadCard" 
                        class="download-card-align-right" 
                        @close="showDownloadCard = false" 
                        @selectMode="handleSelectDownloadMode" 
                        :style="{
                            position: 'absolute',
                            top: '40px',
                            left: '0',
                        }"/>
                </div>
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="store.loadTranslations['Print']">
                    <div class="toolbar-btn"
                        :class="[{ disabled: !hasContent }, props.isWindowMode ? 'with-text' : 'icon-only']"
                        @click="handlePrint">
                        <SvgIcon class="toolbar-icon" icon="mdeditor-print" color="currentColor" />
                        <span class="toolbar-text" v-if="props.isWindowMode">{{ store.loadTranslations['Print'] || 'Print' }}</span>
                    </div>
                </el-tooltip>
            </div>
        </div>
        <div class="editor">
            <custom-scrollbar class="vditor-scrollbar" id="vditor-scrollbar" ref="vditorScrollbarRef"
                :autoHideDelay="2000" :thumbWidth="6"
                :wrapperStyle="{height: '100%'}" :style="{ width: '100%', height: '100%'}" >
                <div ref="editorRef" class="vditor-container" :class="{ 'window-mode': props.isWindowMode, 'sidebar-mode': !props.isWindowMode }"></div>
            </custom-scrollbar>
        </div>
        <!-- 提示信息 -->
        <div class="bottom-tip">
           <div class="tip">
               {{ store.loadTranslations[`The content generated by AI is for reference only, please pay attention to the accuracy of the information.`] }}
           </div>
           <ShortcutTip v-show="props.isWindowMode && props.currentShortcutList.length > 0" :currentShortcutList="props.currentShortcutList"/>
        </div>
        <div class="handle-tip normal-tip" :class="{ 'shifted-up': newVisibleTip === 'save' && showTopTip }">
            <div class="tip-item-msg" v-show="showTopTip" :class="{'advanced-features': store.IsEnableAdvancedCssFeatures}">
                <div class="status-icon" >
                    <img :src="getStatusIcon(status)" :class="{ 'rotating': status === Status.Saving }"  alt="" />
                </div>
                <div class="tip-item-msg-text">
                    {{ topTipMsg }}
                </div>
            </div>
        </div>
        <div class="handle-tip save-tip" :class="{ 'shifted-up': newVisibleTip === 'normal' && showSaveTip }">
            <div class="tip-item-msg" v-show="showSaveTip" :class="{'advanced-features': store.IsEnableAdvancedCssFeatures}">
                <div class="status-icon" >
                    <img :src="getStatusIcon(statusSave)" :class="{ 'rotating': statusSave === Status.Saving }"  alt="" />
                </div>
                <div class="tip-item-msg-text">
                    {{ saveTipMsg  }}
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, onUnmounted, watch, nextTick } from 'vue'
import { ElTooltip } from 'element-plus'
import Vditor from 'vditor'
import 'vditor/dist/index.css'
import DownloadCard from './DownloadCard.vue'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import { useGlobalStore } from '@/store/global'
import { Qrequest } from '@/utils'
import CustomScrollbar from 'custom-vue-scrollbar';
import ShortcutTip from "../ShortcutTip.vue";
import Menu from '@/components/Menu.vue'

const store = useGlobalStore()
const { chatQWeb } = store

// 保存系统事件连接对象，用于清理
let systemEventConnection = null

const editorRef = ref(null)
let vditor = null

// 追踪 undo/redo 的可用状态
const canUndo = ref(false)
const canRedo = ref(false)

// 追踪加粗按钮的激活状态
const isBold = ref(false)

// 追踪无序列表按钮的激活状态
const isUnorderedList = ref(false)

// 追踪有序列表按钮的激活状态
const isOrderedList = ref(false)

// 追踪编辑器是否有内容
const hasContent = ref(false)

// 控制下载卡片显示
const showDownloadCard = ref(false)

// 控制标题菜单显示
const showHeadingMenu = ref(false)
// 标题菜单关闭定时器
let headingMenuCloseTimer = null

// 当前标题层级（0=正文, 1-6=H1-H6）
const currentHeadingLevel = ref(0)

// 当前标题图标
const currentHeadingIcon = ref('mdeditor-h1')

// 当前标题菜单项
const currentHeadingMenuLabel = ref("")

// 标题菜单项（响应式）
const headingMenuItems = ref([])

// 更多菜单项
const showMoreMenu = ref(false)
const moreMenuItems = ref([])

// 更多菜单的水平偏移量
const moreMenuOffsetX = ref(null)
// 更多按钮包装器引用
const moreBtnWrapper = ref(null)
// 更多菜单引用
const moreMenuRef = ref(null)
// markdown-editor 引用
const markdownEditorRef = ref(null)

// 保存相关
const saveTimer = ref(null)
const saveInterval = 1000 // 1秒自动保存
const hasUnsavedChanges = ref(false) // 是否有未保存的更改
const pendingManualSave = ref(false) // 是否有待执行的手动保存（显示提示）

// 更加粗按钮状态（通过监听 vditor 原生按钮的激活状态）
let boldButtonObserver = null
// 无序列表按钮状态观察者
let unorderedListButtonObserver = null
// 有序列表按钮状态观察者
let orderedListButtonObserver = null
// 标题按钮状态观察者
let headingButtonObserver = null
// 图片变化观察者
let imageMutationObserver = null
// 设置图片加载失败处理
const setupImageErrorHandling = (isThemeChange = false) => {
    if (!vditor || !vditor.vditor) return

    const editorElement = vditor.vditor.wysiwyg.element
    if (!editorElement) return

    // 如果已经存在观察者，先断开
    if (imageMutationObserver) {
        imageMutationObserver.disconnect()
        imageMutationObserver = null
    }

    // 创建 MutationObserver 监听新增的图片
    imageMutationObserver = new MutationObserver((mutations) => {
        mutations.forEach((mutation) => {
            mutation.addedNodes.forEach((node) => {
                if (node.nodeType === Node.ELEMENT_NODE) {
                    // 处理新增元素中的图片
                    const images = node.tagName === 'IMG' ? [node] : node.querySelectorAll('img')
                    images.forEach(img => {
                        if (!img.dataset.errorHandled) {
                            processImage(img)
                        }
                    })
                }
            })
        })
    })

    // 开始观察编辑器元素的变化
    imageMutationObserver.observe(editorElement, {
        childList: true,
        subtree: true
    })

    // 处理已存在的图片
    const images = editorElement.querySelectorAll('img')
    images.forEach(img => processImage(img, isThemeChange))
}

// 处理单个图片
const processImage = (img, isThemeChange = false) => {
    // 跳过已经处理过的图片
    if (img.dataset.errorHandled) return

    // 判断是否为网络图片
    const isNetworkImage = img.src.startsWith('http://') || img.src.startsWith('https://')

    if (img.complete) {
        if (img.naturalWidth === 0) {
            // 图片已经加载失败
            handleImageError(img, isThemeChange)
        }
    } else {
        // 图片还在加载中，添加错误监听
        img.addEventListener('error', () => handleImageError(img, isThemeChange))
        img.addEventListener('load', () => {
            // 图片加载完成，检查是否真的加载成功
            if (img.naturalWidth === 0) {
                handleImageError(img, isThemeChange)
            }
        })
        // 对于网络图片，添加超时保护
        if (isNetworkImage) {
            setTimeout(() => {
                if (img.complete && (!img.naturalWidth || img.naturalWidth === 0)) {
                    handleImageError(img, isThemeChange)
                }
            }, 5000) // 5秒超时
        }
    }
}

// 处理图片错误
const handleImageError = async (img, isThemeChange = false) => {
    if (!img) return

    // 如果不是主题变化，且已经处理过，则跳过
    if (!isThemeChange && img.dataset.errorHandled) return

    const originalSrc = img.src
    
    // 如果已经是默认图片，不需要处理
    if (originalSrc.includes('icon-image-lost-')) return
    
    console.log('Image failed to load:', originalSrc)

    // 判断是否为网络图片
    const isNetworkImage = originalSrc.startsWith('http://') || originalSrc.startsWith('https://')
    
    if (isNetworkImage) {
        // 网络图片加载失败，直接设置为默认图片
        console.log('Network image failed to load, setting default image')
        const defaultImage = props.isDarkMode
            ? 'icons/icon-image-lost-dark.svg'
            : 'icons/icon-image-lost-light.svg'
        
        requestAnimationFrame(() => {
            img.src = defaultImage
            img.alt = '网络图片无法访问'
            // 标记已经处理过
            img.dataset.errorHandled = 'true'
        })
        return
    }

    // 本地文件处理逻辑
    try {
        // 提取文件路径（移除 URL 参数）
        const filePath = decodeURIComponent(originalSrc.replace(/^file:\/\//, '').split('?')[0])
        console.log('Checking file existence:', filePath)

        // 检查文件是否存在
        const isExist = await Qrequest(chatQWeb.isFileExist, filePath)
        console.log('File existence check result:', isExist)

        // 根据主题和文件存在性设置默认图片
        const defaultImage = props.isDarkMode
            ? 'icons/icon-image-lost-dark.svg'
            : 'icons/icon-image-lost-light.svg'

        if (!isExist) {
            console.log('Setting default image for missing file')
            // 使用 requestAnimationFrame 确保 DOM 更新完成
            requestAnimationFrame(() => {
                img.src = defaultImage
                img.alt = '图片已丢失或无法访问'
                // 标记已经处理过
                img.dataset.errorHandled = 'true'
            })
        }
    } catch (error) {
        console.error('Error handling image load failure:', error)
        // 如果检查过程出错，直接使用默认图片
        const defaultImage = props.isDarkMode
            ? 'icons/icon-image-lost-dark.svg'
            : 'icons/icon-image-lost-light.svg'
        requestAnimationFrame(() => {
            img.src = defaultImage
            img.alt = '图片加载失败'
            // 标记已经处理过
            img.dataset.errorHandled = 'true'
        })
    }
}

// 设置 MutationObserver 监听 bold 按钮的 class 变化
const setupBoldButtonObserver = () => {
    if (!vditor || !vditor.vditor || !vditor.vditor.toolbar) return

    const boldBtn = vditor.vditor.toolbar.elements?.bold?.children?.[0]
    if (!boldBtn) return

    // 先断开之前的观察者（如果有）
    if (boldButtonObserver) {
        boldButtonObserver.disconnect()
    }

    // 创建新的观察者
    boldButtonObserver = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            if (mutation.attributeName === 'class') {
                // 按钮的 class 变化了，立即更新状态
                isBold.value = boldBtn.classList.contains('vditor-menu--current')
                moreMenuItems.value.find(i => i.data === 'bold').checked = isBold.value
                break
            }
        }
    })

    // 开始观察按钮的 class 属性变化
    boldButtonObserver.observe(boldBtn, { attributes: true, attributeFilter: ['class'] })
}

// 设置 MutationObserver 监听无序列表按钮的 class 变化
const setupUnorderedListButtonObserver = () => {
    if (!vditor || !vditor.vditor || !vditor.vditor.toolbar) return

    const unorderedListBtn = vditor.vditor.toolbar.elements?.list?.children?.[0]
    if (!unorderedListBtn) return

    // 先断开之前的观察者（如果有）
    if (unorderedListButtonObserver) {
        unorderedListButtonObserver.disconnect()
    }

    // 创建新的观察者
    unorderedListButtonObserver = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            if (mutation.attributeName === 'class') {
                // 按钮的 class 变化了，立即更新状态
                isUnorderedList.value = unorderedListBtn.classList.contains('vditor-menu--current')
                moreMenuItems.value.find(i => i.data === 'unordered').checked = isUnorderedList.value
                break
            }
        }
    })

    // 开始观察按钮的 class 属性变化
    unorderedListButtonObserver.observe(unorderedListBtn, { attributes: true, attributeFilter: ['class'] })
}

// 设置 MutationObserver 监听有序列表按钮的 class 变化
const setupOrderedListButtonObserver = () => {
    if (!vditor || !vditor.vditor || !vditor.vditor.toolbar) return

    const orderedListBtn = vditor.vditor.toolbar.elements?.['ordered-list']?.children?.[0]
    if (!orderedListBtn) return

    // 先断开之前的观察者（如果有）
    if (orderedListButtonObserver) {
        orderedListButtonObserver.disconnect()
    }

    // 创建新的观察者
    orderedListButtonObserver = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            if (mutation.attributeName === 'class') {
                // 按钮的 class 变化了，立即更新状态
                isOrderedList.value = orderedListBtn.classList.contains('vditor-menu--current')
                moreMenuItems.value.find(i => i.data === 'ordered').checked = isOrderedList.value
                break
            }
        }
    })

    // 开始观察按钮的 class 属性变化
    orderedListButtonObserver.observe(orderedListBtn, { attributes: true, attributeFilter: ['class'] })
}

// 获取当前光标所在段落的标题级别
const getCurrentParagraphHeadingLevel = () => {
    if (!vditor || !vditor.vditor || !vditor.vditor.wysiwyg) return 0

    const selection = window.getSelection()
    if (!selection || selection.rangeCount === 0) return 0

    const range = selection.getRangeAt(0)
    let node = range.startContainer

    // 如果选中的是文本节点，向上查找父元素
    if (node.nodeType === Node.TEXT_NODE) {
        node = node.parentElement
    }

    // 向上查找最近的块级元素（h1-h6 或 p）
    while (node && node !== vditor.vditor.wysiwyg.element) {
        const tagName = node.tagName?.toLowerCase()
        if (tagName === 'p') {
            return 0
        } else if (tagName && tagName.startsWith('h')) {
            const level = parseInt(tagName.substring(1))
            if (level >= 1 && level <= 6) {
                return level
            }
        }
        node = node.parentElement
    }

    return 0
}

// 更新当前标题级别
const updateCurrentHeadingLevel = () => {
    const level = getCurrentParagraphHeadingLevel()
    currentHeadingLevel.value = level

    // 更新 headingMenuItems 的 checked 状态
    headingMenuItems.value.forEach((item) => {
        item.checked = (item.data === level)
    })

    // 更新当前标题图标
    const currentItem = headingMenuItems.value.find((item) => item.data === level)
    if (currentItem) {
        currentHeadingIcon.value = currentItem.icon
        currentHeadingMenuLabel.value = currentItem.label
    }
}

// 设置监听器，监听光标位置变化和编辑器内容变化
const setupHeadingButtonObserver = () => {
    if (!vditor || !vditor.vditor || !vditor.vditor.wysiwyg) return

    const editorElement = vditor.vditor.wysiwyg.element
    if (!editorElement) return

    // 先断开之前的观察者（如果有）
    if (headingButtonObserver) {
        headingButtonObserver.disconnect()
    }

    // 创建新的观察者，监听编辑器内容变化
    headingButtonObserver = new MutationObserver((mutations) => {
        // 当编辑器内容变化时，更新当前标题级别
        updateCurrentHeadingLevel()
    })

    // 开始观察编辑器元素的变化
    headingButtonObserver.observe(editorElement, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ['class', 'style']
    })

    // 监听选区变化（光标移动）
    const handleSelectionChange = () => {
        updateCurrentHeadingLevel()
    }

    // 监听鼠标点击和键盘事件
    editorElement.addEventListener('click', handleSelectionChange)
    editorElement.addEventListener('keyup', handleSelectionChange)
    document.addEventListener('selectionchange', handleSelectionChange)

    // 保存事件监听器引用，以便清理
    headingButtonObserver.selectionChangeHandler = handleSelectionChange
    headingButtonObserver.clickHandler = handleSelectionChange
    headingButtonObserver.keyupHandler = handleSelectionChange

    // 初始化当前标题级别
    updateCurrentHeadingLevel()
}

// 更新 undo/redo 状态（通过监听原生工具栏按钮状态，不依赖私有API）
const updateUndoRedoState = () => {
    if (vditor && vditor.vditor && vditor.vditor.toolbar) {
        try {
            // 查找 vditor 原生的 undo/redo 按钮元素
            const toolbarElement = vditor.vditor.toolbar.element
            if (!toolbarElement) {
                canUndo.value = false
                canRedo.value = false
                return
            }

            // vditor 工具栏按钮结构：toolbar.elements['undo'] 或通过 DOM 查找
            const undoBtn = vditor.vditor.toolbar.elements?.undo?.children[0]
            const redoBtn = vditor.vditor.toolbar.elements?.redo?.children[0]

            // 根据按钮的 disabled 类名判断状态（vditor 使用 'vditor-menu--disabled' 类）
            canUndo.value = undoBtn && !undoBtn.classList.contains('vditor-menu--disabled')
            canRedo.value = redoBtn && !redoBtn.classList.contains('vditor-menu--disabled')

            console.log('Undo/Redo state updated (via DOM):', {
                canUndo: canUndo.value,
                canRedo: canRedo.value
            })
        } catch (error) {
            console.warn('Failed to update undo/redo state:', error)
            canUndo.value = false
            canRedo.value = false
        }
    }
}

// 定义组件属性
const props = defineProps({
    modelValue: {
        type: Object,
        default: () => ({})
    },
    isWindowMode: {
        type: Boolean,
        default: true
    },
    isDarkMode: {
        type: Boolean,
        default: false
    },
    currentShortcutList: {
        type: Array,
        default: () => []
    }
})

// 定义事件
const emit = defineEmits(['update:modelValue', 'closeMarkdownEditor', 'save'])

const closeMarkdownEditor = () => {
    // 关闭前检查是否有未保存的更改
    if (hasUnsavedChanges.value) {
        triggerSave()
    }
    emit('closeMarkdownEditor')
}

// 触发保存
const triggerSave = (showTip = false) => {
    if (!vditor) return

    if (props.modelValue.id == '') {
        console.warn('Cannot save: modelValue.id is undefined')
        return
    }

    // 手动保存时显示提示
    if (showTip) {
        handleShowTip(store.loadTranslations['Saving...'] || 'Saving...', Status.Saving)
    }

    const content = vditor.getValue()
    const saveData = {
        id: props.modelValue.id,
        title: props.modelValue.title || 'document',
        content: content
    }

    emit('save', saveData)
    hasUnsavedChanges.value = false

    // 手动保存时短暂延迟后显示保存成功提示
    if (showTip) {
        setTimeout(() => {
            handleShowTip(store.loadTranslations['Saved successfully!'] || 'Saved successfully', Status.Successed)
        }, 300)
    }
    console.log('Save success:', saveData.title)
}

// 重置保存定时器（防抖）
const resetSaveTimer = (showTip = false) => {
    // 如果是手动保存请求，记录标记
    if (showTip) {
        pendingManualSave.value = true
    }

    // 清除之前的定时器
    if (saveTimer.value) {
        clearTimeout(saveTimer.value)
        saveTimer.value = null
    }

    // 设置新的定时器，如果有待执行的手动保存，则保持显示提示
    saveTimer.value = setTimeout(() => {
        const shouldShowTip = showTip || pendingManualSave.value
        if (hasUnsavedChanges.value || shouldShowTip) {
            triggerSave(shouldShowTip)
        }
        // 保存完成后清除手动保存标记
        pendingManualSave.value = false
    }, saveInterval)
}

// 快捷键处理
// 注意：Qt WebEngine 环境下 event.key 返回控制字符，需要使用 event.code 判断
const handleKeyDown = (event) => {
    const ctrlOrCmd = event.ctrlKey || event.metaKey
    if (!ctrlOrCmd) return
    
    // Ctrl+S: 保存
    if (event.code === 'KeyS') {
        event.preventDefault()
        event.stopImmediatePropagation()
        console.log('Ctrl+S detected: triggering save')
        resetSaveTimer(true) // 手动保存，防抖并显示提示
        return
    }
    
    // Ctrl+Z: 撤销
    if (event.code === 'KeyZ' && !event.shiftKey) {
        event.preventDefault()
        event.stopImmediatePropagation()
        console.log('Ctrl+Z detected: triggering undo')
        handleUndo()
        return
    }
    
    // Ctrl+Y / Ctrl+Shift+Z: 重做
    if (event.code === 'KeyY' || (event.shiftKey && event.code === 'KeyZ')) {
        event.preventDefault()
        event.stopImmediatePropagation()
        console.log('Ctrl+Y or Ctrl+Shift+Z detected: triggering redo')
        handleRedo()
        return
    }
    
    // Ctrl+B: 加粗
    if (event.code === 'KeyB') {
        event.preventDefault()
        event.stopImmediatePropagation()
        console.log('Ctrl+B detected: triggering bold')
        handleBold()
        return
    }
}

// 处理撤销操作
const handleUndo = () => {
    if (!canUndo.value) return
    if (vditor?.vditor?.undo) {
        vditor.vditor.undo.undo(vditor.vditor)
        setTimeout(updateUndoRedoState, 50)
    }
}

// 处理重做操作
const handleRedo = () => {
    if (!canRedo.value) return
    if (vditor?.vditor?.undo) {
        vditor.vditor.undo.redo(vditor.vditor)
        setTimeout(updateUndoRedoState, 50)
    }
}

// 处理加粗操作
const handleBold = () => {
    const boldBtn = vditor?.vditor?.toolbar?.elements?.bold?.children?.[0]
    if (boldBtn) {
        boldBtn.click()
    }
}

// 处理无序列表操作
const handleUnorderedList = () => {
    const unorderedListBtn = vditor?.vditor?.toolbar?.elements?.list?.children?.[0]
    if (unorderedListBtn) {
        unorderedListBtn.click()
    }
}

// 处理有序列表操作
const handleOrderedList = () => {
    const orderedListBtn = vditor?.vditor?.toolbar?.elements?.['ordered-list']?.children?.[0]
    if (orderedListBtn) {
        orderedListBtn.click()
    }
}

// 切换标题菜单显示
const toggleHeadingMenu = () => {
    showHeadingMenu.value = !showHeadingMenu.value
    closeOtherMenus('heading')
}

// 处理标题按钮鼠标离开
const handleHeadingBtnMouseLeave = () => {
    // 延迟关闭菜单，给鼠标移动到菜单的时间
    headingMenuCloseTimer = setTimeout(() => {
        showHeadingMenu.value = false
    }, 200)
}

// 取消标题菜单关闭
const cancelHeadingMenuClose = () => {
    if (headingMenuCloseTimer) {
        clearTimeout(headingMenuCloseTimer)
        headingMenuCloseTimer = null
    }
}

// 处理标题菜单鼠标离开
const handleHeadingMenuMouseLeave = () => {
    // 延迟关闭菜单
    headingMenuCloseTimer = setTimeout(() => {
        showHeadingMenu.value = false
    }, 200)
}

// 处理更多菜单选择
const toggleShowMoreMenu = () => {
    showMoreMenu.value = !showMoreMenu.value
    closeOtherMenus('more')
}

// 计算更多菜单位置
const calculateMoreMenuPosition = () => {
    if (!moreBtnWrapper.value) return
    
    const buttonRect = moreBtnWrapper.value.getBoundingClientRect()
    const markdownEditor = document.querySelector('.markdown-editor')

    
    if (!markdownEditor) return
    
    const editorRect = markdownEditor.getBoundingClientRect()
    
    // 动态获取菜单宽度
    let menuWidth = 164 // 默认最小宽度
    if (moreMenuRef.value && moreMenuRef.value.menuContainer) {
        const menuRect = moreMenuRef.value.menuContainer.getBoundingClientRect()
        menuWidth = menuRect.width
    }
    
    // 计算菜单右边界位置
    const menuRight = buttonRect.left + menuWidth
    const editorRight = editorRect.right
    
    // 预留间隔（可手动调整）
    const gap = 10
    
    // 如果菜单会超出编辑器右边界，则向左调整
    if (menuRight > editorRight - gap) {
        const overflow = menuRight - (editorRight - gap)
        moreMenuOffsetX.value = -overflow
    } else {
        moreMenuOffsetX.value = null
    }
}

// 处理标题选择
const handleHeadingSelect = (item) => {
    if (!vditor) return
    console.log('Heading menu item selected:', item)

    if (item.data === 0) {
        // TODO 正文
        // const headings = vditor?.vditor?.toolbar?.elements?.headings
        const headings = vditor?.vditor?.toolbar?.elements?.['headings']?.children?.[0]
        if (headings && currentHeadingLevel.value !== 0) {
            headings.click()
        }
    } else {
        // 获取 vditor 原生的标题菜单容器
        const headingsMenu = vditor?.vditor?.toolbar?.elements?.headings?.children?.[1]
        if (headingsMenu) {
            // 根据 item.data 找到对应的按钮并触发点击
            // item.data: 0=正文, 1=H1, 2=H2, 3=H3, 4=H4, 5=H5, 6=H6
            let targetTag = ''
            if (item.data === 0) {
                targetTag = 'p' // 正文对应 p 标签
            } else {
                targetTag = `h${item.data}` // 1-6 对应 h1-h6
            }

            // 查找具有对应 data-tag 属性的按钮
            const targetButton = headingsMenu.querySelector(`[data-tag="${targetTag}"]`)
            if (targetButton) {
                console.log(`Triggering click on button with data-tag="${targetTag}"`)
                targetButton.click()
            } else {
                console.warn(`Button with data-tag="${targetTag}" not found`)
            }
        }
    }

    // 取消其他标题的选中状态
    headingMenuItems.value.forEach((i) => i.checked = false)
    // 设置当前标题的选中状态
    headingMenuItems.value.find((i) => i.data === item.data).checked = true
    // 更新当前标题图标
    currentHeadingIcon.value = item.icon
    // 更新当前标题菜单标签
    currentHeadingMenuLabel.value = item.label
    // 更新当前标题级别
    currentHeadingLevel.value = item.data
    // 触发标题操作
    showHeadingMenu.value = false
    // 清除可能存在的关闭定时器
    if (headingMenuCloseTimer) {
        clearTimeout(headingMenuCloseTimer)
        headingMenuCloseTimer = null
    }
}

const handleMoreSelect = (item) => {
    if (!vditor) return

    console.log('More menu item selected:', item)
    const action = item.data
    // 取消其他标题的选中状态
    moreMenuItems.value.forEach((i) => i.checked = false)
    // 设置当前标题的选中状态
    moreMenuItems.value.find((i) => i.data === action).checked = true
    switch (action) {
        case 'bold':
            console.log('Triggering bold action')
            handleBold()
            break
        case 'unordered':
            console.log('Triggering unordered list action')
            handleUnorderedList()
            break
        case 'ordered':
            console.log('Triggering ordered list action')
            handleOrderedList()
            break
        default:
            return
    }

    showMoreMenu.value = false
}

// 关闭其他菜单
const closeOtherMenus = (currentMenu) => {
    if (currentMenu !== 'heading') {
        showHeadingMenu.value = false
    }
    if (currentMenu !== 'download') {
        showDownloadCard.value = false
    }
    if (currentMenu !== 'more') {
        showMoreMenu.value = false
    }
}

// 处理复制操作
const handleCopy = async () => {
    if (!hasContent.value || !vditor) return
    const content = vditor.getValue()
    copy(content)
}

const copy = async (content) => {
    Qrequest(chatQWeb.copyReplyText, content);
    handleShowTip(store.loadTranslations['Copied successfully'], Status.Successed)
}

// 处理下载操作 - 显示下载选项卡片
const handleDownload = () => {
    if (!hasContent.value) return
    
    if (store.IsEnableMcp) {
        showDownloadCard.value = !showDownloadCard.value
        closeOtherMenus('download')
    } else {
        handleSelectDownloadMode('md')
    }
}

// 处理选择的下载格式
const handleSelectDownloadMode = async (mode) => {
    if (!vditor) return
    
    const content = vditor.getValue()
    const title = props.modelValue.title || 'document'
    const id = props.modelValue.id || ''
    
    // 调用后端统一的下载接口，与 ContentCard 复用相同逻辑
    const ret = await Qrequest(chatQWeb.downloadFile, id, title, content, mode)
    if (ret) {
        handleShowSaveTip(store.loadTranslations['Saving As...'] || 'Saving As...', Status.Saving)
    }
}

// 点击外部关闭下载卡片
const handleClickOutside = (event) => {
    // 关闭下载卡片
    if (showDownloadCard.value &&
        !event.target.closest('.download-card-popup') &&
        !event.target.closest('.download-btn-wrapper')) {
        showDownloadCard.value = false
    }
    // 关闭标题菜单
    if (showHeadingMenu.value &&
        !event.target.closest('.menu-container') &&
        !event.target.closest('.heading-btn-wrapper')) {
        showHeadingMenu.value = false
    }
    // 关闭更多菜单
    if (showMoreMenu.value &&
        !event.target.closest('.menu-container') &&
        !event.target.closest('.more-btn-wrapper')) {
        showMoreMenu.value = false
    }
}

// 处理打印操作
const handlePrint = () => {
    if (!hasContent.value || !vditor) return

    // 获取富文本 HTML 内容
    const html = vditor.getHTML()

    // 添加样式，确保图片在打印时宽度为100%
    const styledHtml = `
        <style>
            img {
                max-width: 100%;
                height: auto;
            }
        </style>
        ${html}
    `

    const title = props.modelValue.title || 'document'

    // 调用后端打印接口
    Qrequest(chatQWeb.printDocument, styledHtml, title)
}

// 编辑器配置
const editorConfig = {
    mode: 'wysiwyg', // 即时渲染模式，更接近Typora的体验
    // 快捷键处理回调
    keydown: (event) => {
        handleKeyDown(event)
    },
    preview: {
        mode: 'editor',
        delay: 0, // 即时预览
        math: {
            engine: 'KaTeX', // 使用 KaTeX 渲染数学公式
            inlineDigit: false // 禁止行内公式以数字开头，避免与普通文本冲突
        }
    },
    toolbarConfig: {
        hide: false, // 完全隐藏工具栏区域
        pin: true
    },
    cache: {
        enable: false
    },
    // 自定义图片渲染处理
    image: {
        // 图片加载失败时的回调
        error: (img) => {
            handleImageError(img)
        }
    },
    // 添加 wysiwyg 模式配置
    customWysiwygToolbar: (vditor) => {
        // 返回自定义工具栏配置，防止报错
        return []
    },
    input: (value) => {
        // 编辑器内容变化时触发事件
        // emit('update:modelValue', { content: value })

        // 更新 undo/redo 状态
        setTimeout(updateUndoRedoState, 50)

        // 更新内容状态
        hasContent.value = value && value.trim().length > 0

        // 标记有未保存的更改
        hasUnsavedChanges.value = true
        // 重置保存定时器（防抖）
        resetSaveTimer()

        // 延迟处理新添加的图片
        setTimeout(() => {
            setupImageErrorHandling()
        }, 100)
    },
    after: () => {
        vditor.clearStack()
        canUndo.value = false
        canRedo.value = false
        isBold.value = false
        isUnorderedList.value = false
        isOrderedList.value = false
        currentHeadingLevel.value = 0

        // 设置 MutationObserver 监听 bold 按钮的 class 变化（最直接的方式）
        setTimeout(setupBoldButtonObserver, 100)
        // 设置 MutationObserver 监听无序列表按钮的 class 变化
        setTimeout(setupUnorderedListButtonObserver, 100)
        // 设置 MutationObserver 监听有序列表按钮的 class 变化
        setTimeout(setupOrderedListButtonObserver, 100)
        // 设置 MutationObserver 监听标题按钮的 class 变化
        setTimeout(setupHeadingButtonObserver, 100)
        
        // 禁用图片双击预览功能
        const editorElement = vditor.vditor.wysiwyg.element
        if (editorElement) {
            editorElement.addEventListener('dblclick', (e) => {
                if (e.target.tagName === 'IMG') {
                    e.stopPropagation()
                    e.preventDefault()
                    console.log('Disabled image double-click preview')
                }
            }, true)
        }

        // 添加图片加载失败处理
        setupImageErrorHandling()
    },
}


// 状态枚举定义（必须在使用前定义）
const Status = {
    None: 0,
    Saving: 1,
    Successed: 2,
    Error: 3,
}

// 提示信息
const topTipMsg = ref('')
const showTopTip = ref(false)
const topTipTimer = ref(null)
const status = ref(Status.None)

// 另存提示信息
const saveTipMsg = ref('')
const showSaveTip = ref(false)
const statusSave = ref(Status.None)

// 追踪哪个提示先显示
const newVisibleTip = ref('') // 'normal' 或 'save'

const handleShowTip = (msg, statusNow = Status.None) => {
    // 清除之前的定时器
    if (topTipTimer.value) {
        clearTimeout(topTipTimer.value)
        topTipTimer.value = null
    }

    // 先隐藏之前的 tip
    showTopTip.value = false

    // 短暂延迟后显示新的 tip，确保过渡平滑
    setTimeout(() => {
        topTipMsg.value = msg
        showTopTip.value = true
        status.value = statusNow

        // 更新先显示的提示
        newVisibleTip.value = 'normal'

        // 设置新的定时器
        topTipTimer.value = setTimeout(() => {
            showTopTip.value = false
            topTipTimer.value = null
            status.value = Status.None
            newVisibleTip.value = ''
        }, 3000)
    }, 50)
}

const handleShowSaveTip = (msg, statusNow = Status.None) => {
    saveTipMsg.value = msg
    showSaveTip.value = true
    statusSave.value = statusNow

    // 更新先显示的提示
    newVisibleTip.value = 'save'

    if (statusNow !== Status.Saving) {
        setTimeout(() => {
            showSaveTip.value = false
            newVisibleTip.value = ''
        }, 3000)
    }
}

const getStatusIcon = (status) => {
    switch (status) {
        case Status.Saving:
            return 'icons/mcp-loading.png';
        case Status.Successed:
            return 'icons/mcp-completed.png';
        case Status.Error:
            return 'icons/mcp-warning.png';
        default:
            return '';
    }
};

const vditorScrollbarRef = ref(null)

// 监听系统事件（关机、重启、锁屏、关闭应用）
const setupSystemEventListener = () => {
    // 通过 QWebChannel 监听 Qt 层发送的系统事件
    if (chatQWeb?.sigSystemEvent) {
        systemEventConnection = chatQWeb.sigSystemEvent.connect((eventType) => {
            console.log('System event received:', eventType)

            // 如果有未保存的更改，触发保存（不显示提示，静默保存）
            if (hasUnsavedChanges.value) {
                console.log('Triggering save before system event:', eventType)
                triggerSave(false)

                // 使用 nextTick 确保保存操作完成后再通知
                nextTick(() => {
                    // 等待一小段时间确保异步保存完成
                    setTimeout(() => {
                        if (chatQWeb?.confirmDataSaved) {
                            console.log('Confirming data saved to system')
                            chatQWeb.confirmDataSaved()
                        }
                    }, 100)
                })
            } else {
                // 没有未保存的更改，直接确认
                if (chatQWeb?.confirmDataSaved) {
                    console.log('No unsaved changes, confirming immediately')
                    chatQWeb.confirmDataSaved()
                }
            }
        })
    }
}

const sigDownloadFileFinished = (id, res) => {
    const contentId = props.modelValue.id || ''
    if (id === contentId) {
        if (res) {
            handleShowSaveTip(store.loadTranslations['Save As successful!'] || 'Save As successful!', Status.Successed)
        } else {
            handleShowSaveTip(store.loadTranslations['Save As failed. Please retry.'] || 'Save As failed. Please retry.', Status.Error)
        }
    }
}

const responseAIFunObj = {
    sigDownloadFileFinished,
}

onMounted(async () => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].connect(responseAIFunObj[key]);
        }
    }
    // 初始化翻译
    useGlobalStore().loadTranslations = await Qrequest(chatQWeb.loadTranslations)

    // 初始化标题菜单
    headingMenuItems.value = [
        { icon: 'mdeditor-text', label: store.loadTranslations['Body text'] , data: 0, checked: true },
        { icon: 'mdeditor-h1', label: store.loadTranslations['Heading 1'], data: 1, checked: false },
        { icon: 'mdeditor-h2', label: store.loadTranslations['Heading 2'], data: 2, checked: false },
        { icon: 'mdeditor-h3', label: store.loadTranslations['Heading 3'], data: 3, checked: false },
        { icon: 'mdeditor-h4', label: store.loadTranslations['Heading 4'], data: 4, checked: false },
        { icon: 'mdeditor-h5', label: store.loadTranslations['Heading 5'], data: 5, checked: false },
        { icon: 'mdeditor-h6', label: store.loadTranslations['Heading 6'], data: 6, checked: false }
    ]
    
    // 初始化更多菜单
    moreMenuItems.value = [
        { icon: 'mdeditor-bold', label: store.loadTranslations['Bold'], data: 'bold', checked: false },
        { icon: 'mdeditor-unordered', label: store.loadTranslations['Unordered list'], data: 'unordered', checked: false },
        { icon: 'mdeditor-ordered', label: store.loadTranslations['Ordered list'], data: 'ordered', checked: false }
    ]


    // 初始化Vditor编辑器
    vditor = new Vditor(editorRef.value, editorConfig)
    document.addEventListener('click', handleClickOutside)
    // 监听系统事件（关机、重启、锁屏、关闭应用）
    setupSystemEventListener()
})

onUnmounted(() => {
    for (const key in responseAIFunObj) {
        if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
            chatQWeb[key].disconnect(responseAIFunObj[key]);
        }
    }
});

// 监听modelValue变化，更新编辑器内容
watch(() => props.modelValue,
    (newVal) => {
        if (vditor && newVal.content !== vditor.getValue()) {
            vditor.setValue(newVal.content || '', true)
            canUndo.value = false
            canRedo.value = false
            isBold.value = false
            isUnorderedList.value = false
            isOrderedList.value = false
            currentHeadingLevel.value = 0
            // 更新 headingMenuItems 的 checked 状态
            headingMenuItems.value.forEach((item) => {
                item.checked = (item.data === 0)
            })
            currentHeadingIcon.value = 'mdeditor-text'
            currentHeadingMenuLabel.value = "Body text"  
            // 更新内容状态
            hasContent.value = newVal.content && newVal.content.trim().length > 0

            // 延迟处理新内容中的图片
            setTimeout(() => {
                setupImageErrorHandling()
            }, 100)
        }
    },
    { deep: true }
)

// 监听主题变化，重新处理图片
watch(() => props.isDarkMode,
    () => {
        // 主题变化时重新处理所有图片
        setTimeout(() => {
            setupImageErrorHandling(true)
        }, 100)
    }
)

// 监听更多菜单显示状态，计算菜单位置
watch(() => showMoreMenu.value, (newVal) => {
    if (newVal) {
        nextTick(() => {
            calculateMoreMenuPosition()
        })
    }
})

// 监听字号变化，重新计算更多菜单位置
watch(() => store.FontSize, (newFontSize) => {
    nextTick(() => {
        if (showMoreMenu.value) {
            calculateMoreMenuPosition()
        }
    })
}, { immediate: true })

onBeforeUnmount(() => {
    document.removeEventListener('click', handleClickOutside)

    // 移除图片错误事件监听器
    if (vditor && vditor.vditor && vditor.vditor.wysiwyg.element) {
        const editorElement = vditor.vditor.wysiwyg.element
        
        // 清理所有图片上的单独事件监听器
        const images = editorElement.querySelectorAll('img')
        images.forEach(img => {
            img.removeEventListener('error', handleImageError)
            img.removeEventListener('load', handleImageError)
        })
    }

    // 断开系统事件监听器
    if (systemEventConnection) {
        systemEventConnection.disconnect()
        systemEventConnection = null
    }

    // 断开 MutationObserver
    if (boldButtonObserver) {
        boldButtonObserver.disconnect()
        boldButtonObserver = null
    }

    // 断开无序列表按钮观察者
    if (unorderedListButtonObserver) {
        unorderedListButtonObserver.disconnect()
        unorderedListButtonObserver = null
    }

    // 断开有序列表按钮观察者
    if (orderedListButtonObserver) {
        orderedListButtonObserver.disconnect()
        orderedListButtonObserver = null
    }

    // 断开标题按钮观察者
    if (headingButtonObserver) {
        headingButtonObserver.disconnect()
        
        // 移除事件监听器
        if (vditor && vditor.vditor && vditor.vditor.wysiwyg && vditor.vditor.wysiwyg.element) {
            const editorElement = vditor.vditor.wysiwyg.element
            if (headingButtonObserver.clickHandler) {
                editorElement.removeEventListener('click', headingButtonObserver.clickHandler)
            }
            if (headingButtonObserver.keyupHandler) {
                editorElement.removeEventListener('keyup', headingButtonObserver.keyupHandler)
            }
        }
        if (headingButtonObserver.selectionChangeHandler) {
            document.removeEventListener('selectionchange', headingButtonObserver.selectionChangeHandler)
        }
        
        headingButtonObserver = null
    }

    // 断开图片变化观察者
    if (imageMutationObserver) {
        imageMutationObserver.disconnect()
        imageMutationObserver = null
    }

    // 清理保存定时器
    if (saveTimer.value) {
        clearTimeout(saveTimer.value)
        saveTimer.value = null
    }
    
    // 清理标题菜单关闭定时器
    if (headingMenuCloseTimer) {
        clearTimeout(headingMenuCloseTimer)
        headingMenuCloseTimer = null
    }

    // 销毁编辑器实例
    if (vditor) {
        vditor.destroy()
    }
})

// 暴露获取编辑器内容的方法
defineExpose({
    getValue: () => vditor ? vditor.getValue() : '',
    setValue: (value) => {
        if (vditor) {
            vditor.setValue(value)
            // 更新modelValue
            emit('update:modelValue', { content: value })
        }
    }
})
</script>

<style scoped lang="scss">

.markdown-editor {
    width: calc(100% - 8px);
    max-width: 1020px;
    height: 100%;
    display: flex;
    flex-direction: column;
    background: var(--uosai-color-chat-bg);
    position: relative;

    .top {
        width: 100%;
        height: 50px;
        z-index: 1000;
        display: flex;
        justify-content: space-between;
        align-items: center;
        background-color: var(--uosai-color-chat-header-bg);
        border-bottom: 1px solid var(--uosai-color-chat-border);
        position: relative;
        box-shadow: var(--uosai-md-editor-toolbar-boxshadow-bg);

        .toolbar-group {
            display: flex;
            align-items: center;

            > * {
                margin: 0 3px;
            }

            &.toolbar-left {
                flex: 0 0 auto;
                justify-content: flex-start;
                margin-left: 10px;
            }

            // 中组在窗口模式下绝对居中，侧边栏模式下自适应
            &.toolbar-center {
                flex: 1 1 auto;
                justify-content: center;
            }
            &.toolbar-center.force-center {
                position: absolute;
                left: 50%;
                transform: translateX(-50%);
            }

            &.toolbar-right {
                flex: 0 0 auto;
                justify-content: flex-end;
                margin-right: 10px;
            }
        }

        .toolbar-divider {
            width: 1px;
            height: 16px;
            background-color: var(--uosai-md-editor-toolbar-divider);
            margin: 0;
        }

        .toolbar-btn {
            height: 36px;
            display: flex;
            justify-content: center;
            align-items: center;
            cursor: pointer;
            border-radius: 8px;
            transition: background-color 0.2s ease;
            user-select: none;
            padding: 0 9px;

            .toolbar-icon {
                font-size: 20px;
                flex-shrink: 0;

                &.back-icon {
                    font-size: 12px;
                }
            }
            .h-downarrow {
                font-size: 6px;
            }

            .toolbar-text {
                margin-left: 7px;
                white-space: nowrap;
                font-size: 12px;
                font-weight: 500;
            }

            // 只保留图标的按钮 - 正方形, 36px
            &.icon-only {
                width: 36px;
                padding: 0;
            }
            // 带文字的按钮
            &.with-text {
                padding: 0 9px;
            }

            color: var(--uosai-md-editor-toolbar-button-text);
            background-color: var(--uosai-md-editor-toolbar-button-bg, transparent);
            &:hover:not(.disabled) {
                color: var(--uosai-md-editor-toolbar-button-text-hover);
                background-color: var(--uosai-md-editor-toolbar-button-bg-hover);
            }

            &:active:not(.disabled) {
                color: var(--activityColor);
                background-color: var(--uosai-md-editor-toolbar-button-bg-active);
            }

            // checked 状态（与 vditor 原生按钮的激活状态一致）
            &.checked {
                background-color: var(--uosai-md-editor-toolbar-button-bg-checked);
            }

            &.disabled {
                opacity: 0.5;
                cursor: not-allowed;
                pointer-events: auto;
            }
        }

        .download-btn-wrapper {
            position: relative;
            display: flex;
            align-items: center;
        }

        .heading-btn-wrapper {
            position: relative;
            display: flex;
            align-items: center;
        }
    }

    // 下载卡片右对齐样式覆盖
    :deep(.download-card-align-right) {
        left: auto !important;
        right: 0 !important;
    }

    // 侧边栏模式下的样式调整
    &:not(.window-mode) {
        .toolbar-group {
            margin: 0 5px;
        }
    }

    .editor {
        width: 100%;
        height: 100%;
        overflow: hidden;
    }

    .vditor-scrollbar {
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
    }

    .vditor-container {
        width: 100%;
        height: 100%;
        min-height: 500px;
        color: var(--uosai-md-editor-text);
        border: none !important; /* 添加边框设置为 none */

        // 隐藏工具栏
        :deep(.vditor-toolbar) {
            display: none !important;
        }

        // 隐藏悬浮工具栏
        :deep(.vditor-panel) {
            display: none !important;
        }

        /* 使用深度选择器 :deep() 穿透 Vue 的 scoped CSS，作用于 Vditor 动态生成的内容 */
        :deep(.vditor-reset) {
            overflow: hidden !important; /* 隐藏自带滚动条 */
            color: var(--uosai-md-editor-text); // 设置文本颜色
            font-family: var(--font-family);
            line-height: 1.2;
            background-color: transparent !important;
            padding: 10px 20px !important;

            h1 {
                font-weight: 500 !important;
                font-size: 1.71rem !important;
                padding-top: 20px !important;
                padding-bottom: 20px !important;
            }

            h2 {
                font-weight: 500 !important;
                font-size: 1.42rem !important;
                padding-top: 16px !important;
                padding-bottom: 16px !important;
            }

            h3, h4, h5, h6 {
                font-weight: 500 !important;
                font-size: 1.14rem !important;
                padding-top: 10px !important;
                padding-bottom: 10px !important;
            }

            h1, h2, h3, h4, h5, h6, p, strong, em, s, table {
                margin-top: 0 !important;
                margin-bottom: 0 !important
            }

            // 隐藏标题下划线
            h1, h2, h3, h4, h5, h6 {
                border-bottom: none !important;
            }

            // 普通文本
            p {
                font-weight: 400 !important;
                font-size: 1rem !important;
                padding-top: 10px !important;
                padding-bottom: 10px !important;
            }

            // 加粗
            strong {
                font-weight: 500 !important;
                font-size: 1rem !important;
            }

            // 斜体
            em {
                font-weight: 400 !important;
                font-size: 1rem !important;
            }

            // 删除线
            s {
                font-weight: 100 !important;
                font-size: 1rem !important;
            }

            // 表格
            table {
                font-weight: 400 !important;
                font-size: 1.14rem !important;
                border-collapse: separate !important;
                border-spacing: 0 !important;
                overflow: hidden !important;
                padding-top: 10px !important;
                padding-bottom: 10px !important;
                width: auto !important;
                max-width: 95% !important;
            }
            
            // 表格单元格基础样式
            table th,
            table td {
                border: 1px solid var(--uosai-md-editor-table-border) !important;
                border-radius: 0 !important;
                text-align: center !important;
                vertical-align: middle !important;
                padding: 10px 10px !important;
                line-height: 1.2 !important;
                word-wrap: break-word !important;
                word-break: break-word !important;
                overflow-wrap: break-word !important;
                white-space: normal !important;
                max-width: 200px !important;
                
            }

            // 表格表头背景色
            table th {
                font-weight: 500 !important;
                background-color: var(--uosai-md-editor-table-header-bg) !important;
            }

            table tr {
                background-color: transparent !important;
            }
            
            // 移除单元格之间的重复边框
            table th + th,
            table td + td {
                border-left: none !important;
            }
            table tr + tr th,
            table tr + tr td {
                border-top: none !important;
            }
            
            // 表格第一行的第一个单元格（左上角）
            table tr:first-child th:first-child {
                border-top-left-radius: 18px !important;
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格第一行的最后一个单元格（右上角）
            table tr:first-child th:last-child {
                border-top-right-radius: 18px !important;
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格第一行的中间单元格
            table tr:first-child th:not(:first-child):not(:last-child) {
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 修复表头与第一行数据之间的边框重叠问题
            // 当表格有thead时，tbody第一行的td不应该有顶部边框
            table thead + tbody tr:first-child td,
            table tbody tr:first-child td {
                border-top: none !important;
            }
            
            // 表格最后一行的第一个单元格（左下角）
            table tr:last-child td:first-child {
                border-bottom-left-radius: 18px !important;
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格最后一行的最后一个单元格（右下角）
            table tr:last-child td:last-child {
                border-bottom-right-radius: 18px !important;
            }
            
            // 表格最后一行的中间单元格
            table tr:last-child td:not(:first-child):not(:last-child) {
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格中间行的第一个单元格
            table tr:not(:first-child):not(:last-child) td:first-child {
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格中间行的最后一个单元格
            table tr:not(:first-child):not(:last-child) td:last-child {
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
            }
            
            // 表格中间行的中间单元格
            table tr:not(:first-child):not(:last-child) td:not(:first-child):not(:last-child) {
                border-right: 1px solid var(--uosai-md-editor-table-border) !important;
                border-bottom: 1px solid var(--uosai-md-editor-table-border) !important;
            }

            // 引用
            blockquote {
                color: var(--uosai-md-editor-blockquote-color) !important;
                border-left: 2px solid var(--uosai-md-editor-blockquote-border-left) !important; /* 设置引用块左侧边框颜色 */
                padding: 0 10px !important;
                margin: 10px 0px !important;

                p {
                    padding: 0 !important;
                    margin-bottom: 10px !important;
                }

                // 最后一个段落不添加底部外边距
                p:last-child {
                    margin-bottom: 0 !important;
                }
            }

            // 链接
            a {
                color: var(--activityColor) !important;
            }

            // 图片
            img {
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
            
            // .vditor-copy{
            //     display: none;
            // }
            
            /* 隐藏标题前面的"H*"标记 */
            h1::before,
            h2::before,
            h3::before,
            h4::before,
            h5::before,
            h6::before{
                content: none !important;
            }

            .vditor-wysiwyg__block::before {
                content: none !important;
            }

            /* 隐藏脚注块的before内容 */
            [data-type="footnotes-block"]::before {
                content: none !important;
            }
        }

        /* 窗口模式：左右 padding 为 80px */
        &.window-mode :deep(.vditor-reset) {
            padding: 10px 80px !important;
        }

        /* 侧边栏模式：左右 padding 为 20px */
        &.sidebar-mode :deep(.vditor-reset) {
            padding: 10px 20px !important;
        }

        /* 确保所有语言类名的代码块都不受高度限制 */
        :deep([class*="language-"]) {
            max-height: none !important;
            height: auto !important;
            overflow: visible !important;
        }
    }

    .handle-tip {
        position: absolute;
        left: 50%;
        transform: translateX(-50%);
        bottom: 45px;
        max-width: calc(100% - 60px); /* 左右各留40px边距 */
        width: fit-content;
        transition: bottom 0.3s ease;

        // 当需要向上平移时
        &.shifted-up {
            bottom: calc(45px + 44px + 10px); /* 45px(原始bottom) + 44px(tip高度) + 10px(间距) */
        }

        .tip-item-msg {
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 10px 12px 10px 10px;
            border-radius: 18px;
            border: 1px solid rgba(0, 0, 0, 0.05);
            box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.2);
            background-color: var(--uosai-color-tip-bg);

            .status-icon {
                display: flex;
                align-items: center;
                justify-content: center;
                width: 20px;
                height: 20px;
                // margin-left: 10px;
                margin-right: 10px;
                margin-top: 1px; /* 调整上边距以对齐文本 */
                

                img {
                    width: 20px;
                    height: 20px;

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

            .tip-item-msg-text {
                color: var(--uosai-color-tip);
                font-size: 0.93rem;
                font-weight: 500;
                font-style: normal;
                
                margin: 0 auto;
                text-align: center;
                width: fit-content;
                user-select: none;
                white-space: normal; /* 允许换行 */
                word-wrap: break-word; /* 长单词换行 */
                line-height: 1.2;
            }

            &.advanced-features {
                background-color: var(--uosai-color-tip-bg-qt6);
                backdrop-filter: blur(30px);
            }

        }
    }

    .bottom-tip {
        width: calc(100% - 20px);
        display: flex;
        align-items: center;
        margin-top: 7px;
        margin-bottom: 10px;

        .tip {
            color: var(--uosai-bottom-tip-color);
            font-size: 12px;
            font-weight: 500;
            font-style: normal;
            text-align: left;
            padding-left: 10px;
            user-select: none;
        }
    }
}

</style>