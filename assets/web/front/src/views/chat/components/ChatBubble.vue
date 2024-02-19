<template>
    <div class="chat-bubble" v-if="item">
        <!-- <div :class="`${item.role} item`" v-html="text"></div> -->
        <template v-if="props.item.anwsers && props.item.anwsers.length > 1">
            <div class="switch-btn" :class="{ disabled: showStop || recording }">
                <div class="prev btn" :class="{ disabled: activeIndex === 0 || recording }" @click="handleSwitch('prev')">
                    <svgIcon icon="arrow_left" />
                </div>
                <div style="user-select: none;">{{ activeIndex + 1 }}/{{ item.anwsers.length }}</div>
                <div class="next btn" :class="{ disabled: activeIndex + 1 === item.anwsers.length || recording }"
                    @click="handleSwitch('next')">
                    <svgIcon icon="arrow_right" />
                </div>
            </div>
        </template>
        <div :class="`${item.role} item`" :style="{ minWidth: content === '' || item.role === 'user' ? 'auto' : '200px' }">
            <div class="loading" v-if="content === '' && showStop && isLast"></div>
            <ImgBubble @handleShowTip="emit('handleShowTip', store.loadTranslations['Copied successfully'])" :disabled="recording || !!playAudioID"
                :content="content" v-else-if="Array.isArray(content)" />
            <pre v-else
                ref="target">{{ content }}<span v-show="showCopy" :class="{ disabled: recording }" class="go-config" @click="!recording && Qrequest(chatQWeb.launchLLMConfigWindow, true)"> {{store.loadTranslations['Go to configuration']}} ></span></pre>
            <div class="play-audio" v-show="item.role !== 'user' && content !== ''">
                <div class="model" v-if="item.anwsers && item.anwsers[activeIndex] && item.anwsers[activeIndex].llmName">
                    <img :src="`file://${item.anwsers[activeIndex].llmIcon}`" alt="">
                    <el-tooltip popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="item.anwsers[activeIndex].llmName">
                        <div class="name">{{ item.anwsers[activeIndex].llmName }}
                        </div>
                    </el-tooltip>
                </div>
                <template v-if="showPlay && content !== '' && !Array.isArray(content)">
                    <div class="play-animation" :style="{ visibility: isActive ? 'visible' : 'hidden' }">
                        <svgIcon :icon="inconName" />
                    </div>
                    <svgIcon v-if="netState&&hasOutput" :class="{ disabled: showStop || recording || !netState }" style=""
                        v-show="!isActive" icon="play" @click="!showStop && !recording && netState && hasOutput&& playTextAudio()" />
                    <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
                        :show-after="1000" :offset="2" :content="!hasOutput?store.loadTranslations['The sound output device is not detected, please check and try again!']:!netState ? store.loadTranslations['Voice broadcast is temporarily unavailable, please check the network!'] : ''">
                        <svgIcon :class="{ disabled: showStop || recording || !netState||!hasOutput }" style="" v-show="!isActive"
                            icon="play" @click="!showStop && !recording && netState && hasOutput&&playTextAudio()" />
                    </el-tooltip>
                    <svgIcon v-show="isActive" icon="stop" @click="stopPlayTextAudio" />
                </template>
                <svgIcon icon="copy" v-show="showPlay && !showCopy && content !== '' && !Array.isArray(content)"
                    class="copy-btn" @click="copy(content)" />
            </div>
        </div>
        <div v-if="!disabledRetry" :class="{ disabled: disabledRetry || recording }" class="retry-btn" v-show="showRetry"
            @click="!disabledRetry && !recording && emit('retryRequest')">
            <svgIcon icon="again" />
            {{store.loadTranslations['Regenerate']}}
        </div>
        <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="disabledRetry ? store.loadTranslations['Answer each question up to 5 times'] : ''">
            <div :class="{ disabledbtn: disabledRetry || recording }" class="retry-btn" v-show="showRetry"
                @click="!disabledRetry && !recording && emit('retryRequest')">
                <div class="retry-content">
                    <svgIcon icon="again" />
                    {{store.loadTranslations['Regenerate']}}
                </div>
            </div>
        </el-tooltip>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import { useIntervalFn } from '@vueuse/core'
import ImgBubble from './ImgBubble.vue'
import _ from "lodash";


const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()
const props = defineProps(['item', 'isLast', 'showStop', 'playAudioID', 'recording', 'netState','hasOutput'])
const emit = defineEmits(['handleShowTip', 'retryRequest', 'update:playAudioID'])
const activeIndex = ref(0)
const disabledRetry = computed(() => props.item.anwsers && props.item.anwsers.length === 5)
const errcodeArr = [-9000, -9001, -9003, -9002]
const showCopy = computed(() => {
    if (props.item.role !== 'user') {
        return errcodeArr.includes(props.item.anwsers[activeIndex.value].status)
    }
    return errcodeArr.includes(props.item.status)
})

const content = computed(() => {
    if (props.item.role !== 'user') {
        const { content, type, status } = props.item.anwsers[activeIndex.value]
        if (status >= 0 && type === 2 && content) return JSON.parse(content)
        return content
    }
    return props.item.content
})
const showRetry = computed(() => {
    return !props.showStop && props.isLast && props.item.role !== 'user' && !errcodeArr.includes(props.item.status)
})
const showPlay = computed(() => {
    if (props.item.role === 'user') return false
    if (props.isLast && props.showStop) return false
    return true
})

const copy = (value) => {
    let oInput = document.createElement('textarea');
    oInput.value = value
    document.body.appendChild(oInput);
    oInput.select();
    document.execCommand('Copy');
    oInput.remove();
    emit('handleShowTip', store.loadTranslations['Copied successfully'])
}

const handleSwitch = (type) => {
    if (props.showStop || props.recording) return
    if (type === 'prev' && activeIndex.value > 0) activeIndex.value = activeIndex.value - 1
    if (type === 'next' && activeIndex.value < props.item.anwsers.length - 1) activeIndex.value = activeIndex.value + 1
}

let index = 1
const inconName = ref('yinpin-1')
const { pause, resume, isActive } = useIntervalFn(() => {
    index++;
    if (index > 9) index = 1
    inconName.value = 'yinpin-' + index
}, 300, { immediate: false })

const playTextAudio = async () => {
    const { content, talkID } = props.item.anwsers[activeIndex.value]
    const res = await Qrequest(chatQWeb.playTextAudio, talkID, content, true)
    if(res){
        emit('update:playAudioID', talkID)
        resume()
    }
}
const stopPlayTextAudio = async () => {
    await Qrequest(chatQWeb.stopPlayTextAudio)
    emit('update:playAudioID', '')
    pause()
}
watch(() => props.item.anwsers, (newValue) => {
    if (Array.isArray(newValue)) {
        activeIndex.value = newValue.length - 1
    }
}, { deep: true, immediate: true })
watch(() => props.playAudioID, (newValue) => {
    if (!props.item.anwsers) return
    const { talkID } = props.item.anwsers[activeIndex.value]
    if (newValue !== talkID) pause()
})
</script>

<style lang="scss" scoped>
.chat-bubble {
    .item {
        padding: 10px 15px;
        font-size: 13px;
        font-weight: 500;
        font-style: normal;
        max-width: 87.5%;
        width: fit-content;
        position: relative;
        border-radius: 8px;

        pre {
            word-break: break-all;
            white-space: pre-wrap;
            line-height: 25px;
        }

        // .copy-btn {
        //     position: absolute;
        //     bottom: -20px;
        //     right: 15px;
        //     z-index: 9999;
        //     border-radius: 8px;
        //     border: 1px solid rgba(0, 0, 0, 0.05);
        //     box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.2);
        //     background-color: rgba(247, 247, 247);
        //     width: 36px;
        //     height: 36px;
        //     cursor: pointer;
        //     display: flex;
        //     align-items: center;
        //     justify-content: center;
        //     opacity: 0;
        //     transition-property: opacity;
        //     transition-duration: 1s;

        //     svg {
        //         fill: #000;
        //     }
        // }

        // &:hover {
        //     .copy-btn {
        //         opacity: 1;
        //     }
        // }

        .play-audio {
            margin-top: 10px;
            display: flex;
            align-items: center;
            width: 100%;

            .icon-play,
            .icon-stop,
            .icon-copy {
                fill: var(--uosai-color-clear);
                color: var(--uosai-color-clear);
                width: 14px;
                height: 14px;
                cursor: pointer;

                &:not(.disabled):hover {
                    fill: var(--uosai-color-clear-hover);
                    color: var(--uosai-color-clear-hover);
                }

                &:not(.disabled):active {
                    fill: var(--activityColor);
                    color: var(--activityColor);
                }
            }

            .play-animation {
                width: 72px;
                height: 20px;
                margin-left: auto;
                margin-right: 10px;

                svg {
                    width: 72px;
                    height: 20px;
                    fill: var(--activityColor);
                    opacity: 1;
                }
            }

            .icon-copy {
                margin-left: 11px;
            }

            .model {
                color: var(--uosai-color-model-name);
                font-size: 12px;
                font-weight: 500;
                font-style: normal;
                user-select: none;
                display: flex;
                align-items: center;
                width: 50%;

                .name {
                    max-width: 90%;
                    white-space: nowrap;
                    overflow: hidden;
                    text-overflow: ellipsis;
                    width: fit-content;
                }

                img {
                    width: 14px;
                    height: 14px;
                    margin-right: 7px;
                }
            }
        }


    }

    .user {
        background-color: var(--activityColor);
        color: rgba(255, 255, 255, 1);
        margin-bottom: 20px;
        margin-left: auto;
        box-shadow: 0px 4px 6px var(--borderColor);
    }

    .assistant {
        border: 1px solid rgba(0, 0, 0, 0.05);
        box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.08);
        background-color: var(--uosai-color-assistant-bg);
        color: var(--uosai-color-shortcut);
        margin-bottom: 20px;
        margin-right: auto;
        min-width: 200px;
    }

    .loading {
        width: 22px;
        height: 6px;
        animation-name: loadingChange;
        animation-duration: 1.5s;
        animation-iteration-count: infinite;
    }

    .go-config {
        font-size: 13px;
        color: var(--activityColor);
        cursor: pointer;
    }

    @keyframes loadingChange {
        0% {
            background: url(../../../svg/1-loading.svg) no-repeat 0px center;
        }

        50% {
            background: url(../../../svg/2-loading.svg) no-repeat 0px center;

        }

        100% {
            background: url(../../../svg/3-loading.svg) no-repeat 0px center;

        }
    }

    @keyframes loadingChangedark {
        0% {
            background: url(../../../svg/1-loading-dark.svg) no-repeat 0px center;
        }

        50% {
            background: url(../../../svg/2-loading-dark.svg) no-repeat 0px center;

        }

        100% {
            background: url(../../../svg/3-loading-dark.svg) no-repeat 0px center;

        }
    }

    .retry-btn {
        padding: 6px 15px;
        border-radius: 8px;
        box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.05);
        // border: 1px solid rgba(0, 0, 0, 0.05);
        background-color: var(--uosai-color-shortcut-bg);
        width: fit-content;
        color: var(--activityColor);
        font-size: 13px;
        font-weight: 500;
        font-style: normal;
        cursor: pointer;
        user-select: none;
        margin-top: -10px;
        display: flex;
        align-items: center;
        margin-left: 1px;
        margin-bottom: 5px;
        &:not(.disabledbtn):hover {
            // border: 1px solid rgba(0, 0, 0, 0.05);
            background-color: var(--uosai-color-shortcut-hover);
        }

        &:not(.disabledbtn):active {
            box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.1);
            background-color: var(--uosai-color-shortcut-active);
        }

        svg {
            fill: var(--activityColor);
            width: 12px;
            height: 14px;
            margin-right: 6px;
        }
        .retry-content {
            display: flex;
            align-items: center;
        }
        &.disabledbtn .retry-content {
            opacity: 0.4;
        }
        &.disabledbtn {
            background-color: var(--uosai-bg-retry);
            border-color: var(--uosai-border-color-retry);
            cursor: not-allowed;
        }
    }

    .switch-btn {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        color: var(--uosai-color-shortcut);
        font-size: 12px;
        margin-bottom: 9px;
        height: 16px;
        letter-spacing: 2px;
        line-height: 12px;
        padding-right: 10px;

        .btn {
            width: 16px;
            height: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            border-radius: 3px;

            &.disabled {
                opacity: 0.4;
            }

            &:not(.disabled):hover {
                background-color: var(--uosai-color-modelbtn-bg);
            }

            &:not(.disabled):active {
                background-color: var(--uosai-color-modelbtn-hover);

                svg {
                    fill: var(--activityColor);
                }
            }
        }

        .prev {
            margin-right: 5px;
        }

        .next {
            margin-left: 5px;
        }

        svg {
            width: 5px;
            height: 8px;
            fill: var(--uosai-color-shortcut);
        }
    }
}

.dark {
    .loading {
        animation-name: loadingChangedark !important;
    }
    .retry-btn{
        box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.15);
    }
}
</style>
