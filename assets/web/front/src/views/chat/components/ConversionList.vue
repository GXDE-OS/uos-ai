<template>
  <div class="conversion-list-container" v-show="show">
    <!-- 原有列表容器保持不变 -->
    <div class="history-list-container">
      <div class="mask"></div>
      <div class="conversion-list-wrapper">
        <transition name="slide-up">
          <div class="conversion-list" v-show="showContent" :class="{ 'advanced-features': store.IsEnableAdvancedCssFeatures }">
            <!-- 新增标题栏 -->
            <div class="header">
              <div class="header-left">
                <span class="title">{{ store.loadTranslations['History'] }}</span>
                <el-tooltip popper-class="uos-tooltip clear-history-list-tooltip" effect="light" placement="top" :show-arrow="false" :enterable="false"
                  :show-after="1000" :offset="5" :content="store.loadTranslations['Clear History']">
                  <div class="clear-btn" :class="{ disabled: isHistoryEmpty }" @click="handleClearHistory">
                    <SvgIcon icon="trash"/>
                  </div>
                </el-tooltip>
              </div>
              <div class="close-btn" @click="handleClose">
                <SvgIcon icon="close"/>
              </div>
            </div>
            <custom-scrollbar class="history-list-scrollbar" id="historyListScroll" :autoHideDelay="2000" :thumbWidth="6"
              :wrapperStyle="{height: 'calc(100% - 54px) !important'}" :style="{ width: '100%', height: '100%'}" v-show="history.length > 0">
              <div v-for="item in history"
                :key="item.conversationId"
                class="history-item"
                @click="selectItem(item)"
                @mouseenter="handleHover(item, true)"
                @mouseleave="handleHover(item, false)">
                <span class="title-text">{{ item.conversationTitle }}</span>
                <div class="right-content">
                  <div
                    class="date-label"
                    v-show="!itemHoverStates[item.conversationId]">
                    {{ formatDate(item.conversationTimestamp) }}
                  </div>
                  <div
                    class="delete-btn"
                    v-show="itemHoverStates[item.conversationId]"
                    @click.stop="handleDelete(item)">
                    <SvgIcon icon="trash"/>
                  </div>
                </div>
              </div>

            </custom-scrollbar>
            <div class="empty-history" v-show="history.length === 0">
              <img :src="isDarkMode? 'icons/no-history-dark.png':'icons/no-history-light.png'" alt="" class="empty-icon"/>
              <p class="empty-text">{{ store.loadTranslations['No History Records'] }}</p>
            </div>
          </div>
        </transition>
      </div>
    </div>
  </div>
</template>

<script setup>
import CustomScrollbar from 'custom-vue-scrollbar';
import { useGlobalStore } from "@/store/global";
const store = useGlobalStore()
import { computed, ref, onMounted, watch } from 'vue';

const props = defineProps({
  historyList: {},
  isDarkMode: Boolean,
  show: Boolean
})

const emit = defineEmits(['showAlertDialog','selectHistoryItem', 'close'])

// Add new state to control animation
const showContent = ref(false)

// Watch for parent changes to properly animate
watch(() => props.show, (newValue) => {
  if (newValue) {
    // 当父组件显示历史记录时
    setTimeout(() => {
      showContent.value = true
    }, 10) // 减少延时，让动画更快开始
  } else {
    // 当父组件隐藏历史记录时
    showContent.value = false
  }
}, { immediate: true })

// Handle close with animation
const handleClose = () => {
  showContent.value = false
  setTimeout(() => {
    emit('close')
  }, 300) // 匹配过渡动画时间
}

const history = computed(() => {
    return props.historyList
        ? Object.values(props.historyList)
            .filter(item => item.conversationTitle !== '') // 新增过滤条件
            .sort((a, b) => {
                // 降序排列（最新记录在前）
                return b.conversationTimestamp - a.conversationTimestamp
            })
        : []
})

const isHistoryEmpty = computed(() => {
    return history.value.length === 0
})

const isDarkMode = computed(() => {
	return props.isDarkMode
})

const selectItem = (item) => {
  	emit('selectHistoryItem', item)
}

// 添加hover状态管理
const itemHoverStates = ref({})
const handleHover = (item, isHovering) => {
  	itemHoverStates.value[item.conversationId] = isHovering
}

// 添加日期格式化方法
const formatDate = (timestamp) => {
    const date = new Date(timestamp * 1000)
    const now = new Date()
    const currentYear = now.getFullYear()

    // 获取今天的0点时间
    const todayStart = new Date(now.setHours(0, 0, 0, 0))
    const todayEnd = new Date(now.setHours(23, 59, 59, 999))
    // 获取昨天的0点时间
    const yesterdayStart = new Date(todayStart)
    yesterdayStart.setDate(yesterdayStart.getDate() - 1)

    // 获取年份判断函数
    const getYear = d => d.getFullYear().toString()
    // 获取月日格式
    const getMD = d => `${d.getMonth() + 1}-${d.getDate()}` // 简化的月日格式

    if (date > todayEnd) return `${getYear(date)}-${getMD(date)}`
    if (date >= todayStart) return store.loadTranslations['Today']
    if (date >= yesterdayStart) return store.loadTranslations['Yesterday']

    // 按年份判断格式
    return parseInt(date.getFullYear()) === currentYear
        ? getMD(date)
        : `${getYear(date)}-${getMD(date)}` // 2024-1-1 格式
}

// 新增状态管理
const selectedItem = ref(null)

// 修改删除处理逻辑
const handleDelete = async (item) => {
	selectedItem.value = item
	emit('showAlertDialog', item)
}

// 新增清空历史记录处理逻辑
const handleClearHistory = async () => {
  emit('showClearHistoryAlertDialog')
}
</script>

<style lang="scss" scoped>
.conversion-list-wrapper {
  position: absolute;
  width: 100%;
  height: 70vh;
  bottom: 0;
  left: 0;
  overflow: hidden;
  z-index: 10;
  perspective: 1000px;
}

.conversion-list {
  position: absolute;
  bottom: 0;
  left: 50%;
  transform: translateX(-50%);
  width: 100%;
  height: 90%;
  z-index: 2;
  background-color: var(--uosai-chat-bg);
  border-radius: 18px 18px 0px 0px;
  backface-visibility: hidden;
  border: 1px solid var(--uosai-color-shortcut-border);
  box-shadow: 0px -6px 20px 0 var(--uosai-history-list-box-shadow);
  
  &.advanced-features {
    background-color: var(--uosai-chat-bg-qt6);
    backdrop-filter: blur(20px);
  }
}

.slide-up-enter-active {
  animation: slideUp 0.35s cubic-bezier(0.0, 0.0, 0.2, 1) forwards;
}

.slide-up-leave-active {
  animation: slideDown 0.25s cubic-bezier(0.4, 0.0, 0.6, 1) forwards;
}

@keyframes slideUp {
  from {
    transform: translate(-50%, 100%);
  }
  to {
    transform: translate(-50%, 0);
  }
}

@keyframes slideDown {
  from {
    transform: translate(-50%, 0);
  }
  to {
    transform: translate(-50%, 100%);
  }
}

.history-list-container {
	position: fixed;
	top: 0;
	left: 0;
	width: 100vw;
	height: 100vh;
	box-shadow: 0px -6px 20px 0 var(--uosai-history-list-box-shadow);

	.header {
		display: flex;
		justify-content: space-between;
		align-items: center;
		padding-top: 10px;
		padding-left: 18px;
		padding-bottom: 10px;
		padding-right: 10px;
		font-size: 1.14rem;
		font-weight: 500;
		height: 24px;
		color: var(--uosai-color-svgbtn-hover);

    .header-left {
      display: flex;
      align-items: center;
      user-select: none;
	  }

	  .clear-btn {
      z-index: 3;
      display: flex;
      justify-content: center;
      align-items: center;
      margin-left: 8px;
      margin-top: 2px;
      width: 20px;
      height: 20px;

      svg {
          fill: var(--uosai-color-prompt-icon);
          width: 16px;
          height: 18px;
      }

      &.disabled {
          opacity: 0.5;
          cursor: not-allowed;
          pointer-events: none;
      }

      &:not(.disabled):hover {
          border-radius: 4px;
          background-color: var(--uosai-color-clear-hover-bg);
      }

      &:not(.disabled):active {
          border-radius: 4px;
          background-color: var(--uosai-color-clear-hover-bg);

          svg {
              fill: var(--activityColor);
          }
      }
	}

		.close-btn {
			display: flex;
			align-items: center;
			justify-content: center;
			cursor: pointer;
			width: 16px;
			height: 16px;
			margin-right: 8px;
			svg {
				fill: var(--uosai-color-prompt-icon);
				width: 9px;
				height: 9px;
			}
			&:not(.disabled):hover {
				svg {
					fill: var(--uosai-color-prompt-icon-hover);
				}
			}
		}
	}

}

.mask {
  height: 100vh;
  width: 100vw;
  background-color: var(--uosai-history-list-mask-bg);
  position: fixed;
  z-index: 1;
  cursor: pointer; /* 指示可点击关闭 */
  opacity: 0;
  transition: opacity 0.3s ease;
}

.conversion-list-container:not([style*="display:none"]):not([style*="display: none"]) .mask {
  opacity: 1;
}

.history-item {
	display: flex;
	justify-content: space-between;
	align-items: center;
	height: 30px;
	padding-left: 8px;
	margin: 0px 10px;
	cursor: pointer;
	font-size: 0.9rem;
	font-weight: 400;
	color: var(--uosai-color-assistantmenu-name);

	&:hover {
		border-radius: 8px;
		background-color: var(--uosai-history-list-item-hover-bg);
	}
	&:active {
		border-radius: 8px;
		background-color: var(--uosai-history-list-item-active-bg);
	}

	.title-text {
		flex: 1 1 calc(100% - 90px); /* 占据75%宽度 */
		min-width: 0; /* 修复flex布局溢出问题 */
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
	}

	.right-content {
		flex-shrink: 0; /* 防止右侧内容被压缩 */
		width: 80px; /* 固定70宽度 */
		margin-left: 10px;
		margin-right: 10px;
		text-align: right;
		display: inline-flex;
		align-items: center;
		justify-content: flex-end; // 新增这一行
		.date-label {
			color: var(--uosai-history-date-color);
			font-size: 0.85em;
			opacity: 0.8;
		}

		.delete-btn {
			z-index: 3;
			display: flex;
			justify-content: center;
			align-items: center;
			width: 20px;
			height: 20px;

			svg {
			    fill: var(--uosai-color-prompt-icon);
			    width: 12px;
			    height: 14px;
			}

      &:not(.disabled):hover {
          border-radius: 4px;
          background-color: var(--uosai-color-svgbtn-bg);
      }

      &:not(.disabled):active {
        border-radius: 4px;
        background-color: var(--uosai-color-svgbtn-bg);

          svg {
              fill: var(--activityColor);
          }
      }
		}
	}
}

.history-list-scrollbar {
    height: calc(100% - 54px) !important; // 减去标题栏高度
    overflow-y: auto !important;
    padding-bottom: 10px; // 添加底部内边距
}

.scrollbar__wrapper {
  	padding: 0px !important;
}

.empty-history {
  display: flex;
  flex-direction: column;
  align-items: center;
  height: 100%;
  margin-top: 62px;
  user-select: none;

  .empty-icon {
    width: 300px;
    height: 190px;
  }

  .empty-text {
    margin-top: 16px;
    color: var(--uosai-color-prompt-tag);
    font-size: 0.9rem;
	font-weight: 400;
	line-height: 19px;
  }
}
</style>