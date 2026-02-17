<template>
    <div class="uos-ai-recommend" :style="{'margin-left': isWindowMode ? '37px' : '25px'}" v-show="assistantListShow.length > 0">
        <!-- 分割线 -->
        <div class="uos-ai-recommend-line" v-show="isWindowMode" />
        <div class="title" :style="{'margin-right': isWindowMode ? '37px' : '25px', 'justify-content': isWindowMode ? 'center' : 'left', 'margin-top': isWindowMode ? '0px' : '40px'}">{{store.loadTranslations['Recommendations']}}</div>
        <div class="recommend-list">
            <div
                v-for="(assistant, index) in assistantListShow.slice(0, 5)"
                :key="assistant.id"
                @click="handleClick(assistant)">
                <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                :show-after="1000" :offset="2" :content="assistant.displayname" :disabled="tooltipDisabled[index]">
                    <div class="recommend-item" :style="{'width': recommendItemWidth + 'px', 'margin-bottom': isWindowMode ? '0px' : '10px'}">
                        <div class="icon">
                            <img :src='assistant.iconPrefix + assistant.icon + "-32.svg"' style="width: 20px; height: 20px;" alt="">
                        </div>
                        <span :ref="el => itemTexts[index] = el" :style="{ 'overflow': 'hidden', 'text-overflow': 'ellipsis', 'white-space': 'nowrap', 'max-width': recommendItemWidth -  53 +'px'}">
                            {{ assistant.displayname }}
                        </span>
                    </div>
                </el-tooltip>
            </div>
            <!-- 更多按钮 -->
            <div class="recommend-item" :style="{'width': recommendItemWidth + 'px', 'margin-bottom': isWindowMode ? '0px' : '10px'}"
                v-show="assistantListShow.length > 5" 
                @click="handleMoreClick">
                <div class="icon" style="padding-top: 3.5px;">
                    <SvgIcon icon="more-icon" style="width: 20px; height: 20px;"/>
                </div>
                <span>{{store.loadTranslations['More']}}</span>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref } from 'vue'
import { useGlobalStore } from "@/store/global";
const store = useGlobalStore()
const emit = defineEmits(['update:currentAssistant', 'showAssistantList'])

const props = defineProps({
    assistantList: {
        type: Array,
        required: true
    },
    isWindowMode: {
        type: Boolean,
        required: true
    }
})

function handleClick(assistant) {
    // 这里可以根据 assistant 做跳转或其他操作
    emit('update:currentAssistant', assistant)
}

function handleMoreClick() {
    // 这里可以处理更多按钮的点击事件
    emit('showAssistantList')
}

const assistantListShow = computed(() => {
    // 去掉type为1的assistant
    const filteredList = props.assistantList.filter(assistant => assistant.type !== store.AssistantType.UOS_AI)
    // 使用 Fisher-Yates 洗牌算法打乱数组
    const shuffled = [...filteredList]
    for (let i = shuffled.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]]
    }
    return shuffled
})

const recommendItemWidth = computed(() => {
    if (!props.isWindowMode) {
        return '148'
    }

    if (assistantListShow.value.length <= 5) {
        return '168'
    }else {
        return '138'
    }
})

const itemTexts = ref([]);
const tooltipDisabled = ref([]);
const systemFontSizeObserver = ref(null);

watch([() => props.isWindowMode, assistantListShow], () => {
  nextTick(() => {
    itemTexts.value = [];
  });
}, { immediate: true });

const isTextOverflow = (index) => {
  return new Promise(resolve => {
    const check = () => {
      if (!itemTexts.value[index]) {
        requestAnimationFrame(check);
        return;
      }

      const element = itemTexts.value[index];
      if (element.clientWidth === 0) {
        requestAnimationFrame(check);
        return;
      }

      const isOverflow = element.scrollWidth > element.clientWidth;
      resolve(isOverflow);
    };
    check();
  });
};

// 更新 tooltip 状态
const updateTooltipState = async () => {
    tooltipDisabled.value = await Promise.all(
        assistantListShow.value.slice(0, 5).map(async (_, index) => {
            return !(await isTextOverflow(index));
        })
    );
};

// 初始化系统字号变化监听
const initSystemFontSizeObserver = () => {
  systemFontSizeObserver.value = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
      if (mutation.type === 'attributes' && mutation.attributeName === 'style') {
        updateTooltipState();
        break;
      }
    }
  });

  // 监听 body 元素的 style 变化
  systemFontSizeObserver.value.observe(document.body, {
    attributes: true,
    attributeFilter: ['style'],
    subtree: false
  });

  // 监听 html 元素的 style 变化
  const htmlElement = document.documentElement;
  systemFontSizeObserver.value.observe(htmlElement, {
    attributes: true,
    attributeFilter: ['style'],
    subtree: false
  });
};

watchEffect(() => {
  nextTick(() => {
    updateTooltipState();
  });
});

onMounted(() => {
  initSystemFontSizeObserver();
});

onBeforeUnmount(() => {
  if (systemFontSizeObserver.value) {
    systemFontSizeObserver.value.disconnect();
  }
});
</script>

<style lang="scss" scoped>


.uos-ai-recommend {

}

.uos-ai-recommend-line {
    width: 890px;
    height: 1px;
    background-color: var(--uosai-color-shortcut-border);
    margin-top: 8px;
    margin-bottom: 9px;
}

.title {
    display: flex;
    justify-content: center;
    font-size: 0.93rem;
    font-weight: 500;
    color: var(--uosai-color-shortcut-title);
    margin-bottom: 11px;
}
.recommend-list {
    display: flex;
    flex-wrap: wrap;

    .recommend-item {
        display: flex;
        align-items: center;
        width: 170px;
        height: 54px;
        /* background: #f5f6fa; */
        /* border-radius: 8px; */
        cursor: pointer;
        /* padding: 8px 12px; */
        transition: background 0.2s;
        align-items: center;
        border-radius: 12px;
        border: 1px solid var(--uosai-color-shortcut-border);
        box-shadow: 0 2px 3px var(--uosai-color-shortcut-shadow);
        background-color: var(--uosai-color-shortcut-bg);
        cursor: pointer;
        margin-right: 10px;
        font-size: 0.93rem;
        font-weight: 500;
        color: var(--uosai-color-shortcut-title);
        &:hover {
            background-color: var(--uosai-color-shortcut-hover);
        }

        &:active {
            background-color: var(--uosai-color-shortcut-active);
        }
        &:hover {
            background-color: var(--uosai-color-shortcut-hover);
        }

        &:active {
            background-color: var(--uosai-color-shortcut-active);
        }
    }
}


.icon {
    width: 20px;
    height: 20px;
    margin-left: 15px;
    margin-right: 8px;
    padding-top: 2.5px;
}
</style> 