<!-- <template> -->
    <!-- <div class="chat-top">
        <div class="logo"
            :style="`opacity:${historyLength === 0 || router.currentRoute.value.name == 'DigitalImage' ? 0 : 1} ;`">
            <img draggable="false" style="user-select: none;" src="../../../svg/icon.svg" alt="">
        </div>
        <div class="title"
            :style="`opacity:${historyLength === 0 || router.currentRoute.value.name == 'DigitalImage' ? 0 : 1} ;`">UOS
            AI
        </div>
        <div class="right-btn">
            <div class="btn"
                :class="{ disabledTop: (recording || localmodel) && router.currentRoute.value.name == 'Chat' || (showStop && videoStatus !== 5 && videoStatus !== 10) }"
                @click="!((recording || localmodel) && router.currentRoute.value.name == 'Chat') && !(showStop && videoStatus !== 5 && videoStatus !== 10) && handelModel()"
                @mouseover="handleMouseOver()" @mouseout="handleMouseOut()">
                <svgIcon icon="change-normal" />
                <Tips :tip="tipCon" :class="className" v-show="isTipsVisible"> </Tips>
            </div>

            <div class="btn" v-on-click-outside.bubble="handelOutside" :class="{ disabledTop: recording, showSeting }"
                @click="!recording && (showSeting = !showSeting)">
                <svgIcon icon="seting-normal" style="width: 18px;height: 18px;" />
                <div class="seting-pop" v-show="showSeting">
                    <div class="seting-item" @mouseover="showMode = true" @mouseout="showMode = false">
                        {{ useGlobalStore().loadTranslations['Mode'] }}
                        <svgIcon icon="arrow_right"  style="width: 8px;height: 8px;" />
                    </div>
                    <div class="seting-item" @click="Qrequest(chatQWeb.launchLLMConfigWindow, false)">
                        {{ useGlobalStore().loadTranslations['Settings'] }}
                    </div>
                    <div class="separator"></div>
                    <div class="seting-item" @click="Qrequest(chatQWeb.launchAboutWindow)">
                        {{ useGlobalStore().loadTranslations['About'] }}
                    </div>
                </div>
                <div class="mode-pop" v-show="showMode" @mouseover="showMode = true">
                    <div class="mode-item" @click="Qrequest(chatQWeb.setDisplayMode, 0)">
                        <SvgIcon icon="ok-acitve" style="width: 10px;height: 10px;" />
                        {{ useGlobalStore().loadTranslations['Window Mode'] }}
                    </div>
                    <div class="mode-item" @click="Qrequest(chatQWeb.setDisplayMode, 1)">
                        <SvgIcon icon="ok-acitve" style="width: 10px;height: 10px;" />
                        {{ useGlobalStore().loadTranslations['Sidebar Mode'] }}
                    </div>
                </div>
            </div>
            <div class="close btn" @click="handleClose">
                <svgIcon icon="close" style="width: 12px;" />
            </div>
        </div>
    </div> -->
<!-- </template> -->
<!-- <script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import { useRouter } from "vue-router";
import Tips from "../../../components/tips/tips.vue";
import { vOnClickOutside } from '@/utils/VonClickOutside'

const router = useRouter();
const { chatQWeb } = useGlobalStore()
const props = defineProps(['historyLength', 'recording', 'showStop', 'localmodel', 'videoStatus'])
const emit = defineEmits(['update:playAudioID', 'sigAudioASRError', 'routeJump'])

const tipCon = computed(() => {
    if ( router.currentRoute.value.name == 'Chat' ) {
        return useGlobalStore().loadTranslations['Voice conversation'];
    } else {
        return useGlobalStore().loadTranslations['Turn off voice conversation'];
    }
});
const className = ref("bot-left"); //tips组件tips位置类
const isTipsVisible = ref(false); //tips是否显示
let tipsTimerId = null;
/**鼠标移入tips显示 */
const handleMouseOver = () => {
    tipsTimerId && clearTimeout(tipsTimerId);
    tipsTimerId = setTimeout(() => {
        isTipsVisible.value = true;
    }, 2000); // 延迟2秒
};
/**鼠标移出tips隐藏 */
const handleMouseOut = () => {
    tipsTimerId && clearTimeout(tipsTimerId);
    isTipsVisible.value = false;
};
const handelModel = async () => {
    const res = await Qrequest(chatQWeb.queryLLMAccountList);
    if (JSON.parse(res).length === 0) {
        Qrequest(chatQWeb.launchLLMConfigWindow, true);
    } else {
        if (router.currentRoute.value.name == "DigitalImage") {
            emit('routeJump')
        } else {
            await Qrequest(chatQWeb.stopPlayTextAudio)
            emit('update:playAudioID', '')
            router.push("/DigitalImage");
        }
    }
};
const handleClose = async () => {
    await Qrequest(chatQWeb.closeChatWindow)
}
const showSeting = ref(false)
const showMode = ref(false)
const handelOutside = () => {
    setTimeout(() => showSeting.value = false, 0)
    setTimeout(() => showMode.value = false, 0)
}
defineExpose({ handelModel, showSeting, showMode })
</script>

<style lang="scss" scoped>
.chat-top {
    height: 50px;
    margin-bottom: 25px;
    padding-left: 10px;
    position: relative;
    display: flex;
    align-items: center;
    flex-shrink: 0;
    z-index: 10;

    .logo {
        width: 30px;
        height: 30px;
        img {
            width: 30px;
            height: 30px;
        }
    }

    .title {
        color: var(--uosai-color-title);
        font-size: 1.21rem;
        font-weight: 600;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        user-select: none;
    }

    .right-btn {
        position: absolute;
        top: 50%;
        right: 0;
        transform: translate(0, -50%);
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .btn {
        position: relative;
        width: 50px;
        height: 50px;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;

        &.showSeting:not(.disabledTop) {
            background: var(--uosai-color-svgbtn-bg);

            svg {
                fill: var(--uosai-color-svgbtn-hover);
            }
        }

        &.disabledTop {
            svg {
                opacity: 0.4 !important;
            }

            cursor: not-allowed !important;
        }

        &:not(.disabledTop):hover {
            background: var(--uosai-color-svgbtn-bg);

            svg {
                fill: var(--uosai-color-svgbtn-hover);
            }
        }

        &:not(.disabledTop):active {
            background: var(--uosai-color-svgbtn-bg);

            svg {
                fill: var(--activityColor);
            }
        }

        svg {
            width: 14px;
            height: 14px;
            fill: var(--uosai-color-svgbtn);
        }

        .seting-pop {
            position: absolute;
            top: 50px;
            left: -55px;
            // border: 1px solid rgba(0, 0, 0, 0.05);
            box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.2);
            opacity: 1;
            background-color: rgba(255, 255, 255, 1);
            width: 150px;
            height: 112px;
            border-radius: 8px;
            padding-top: 8px;
            overflow: hidden;

            .separator {
                width: 150px;
                height: 2px;
                background-color: rgba(0, 0, 0, 0.1);
            }

            .seting-item {
                padding-left: 32px;
                height: 34px;
                line-height: 34px;
                color: #000;
                user-select: none;
                font-size: 1rem;

                &:hover {
                    background-color: var(--activityColor);
                    color: #fff;
                }
            }
        }

        .mode-pop {
            position: absolute;
            top: 50px;
            left: -205px;
            box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.2);
            opacity: 1;
            background-color: rgba(255, 255, 255, 1);
            width: 150px;
            height: 76px;
            border-radius: 8px;
            padding-top: 8px;
            overflow: hidden;

            .mode-item {
                padding-left: 32px;
                height: 34px;
                line-height: 34px;
                color: #000;
                user-select: none;
                font-size: 1rem;

                &:hover {
                    background-color: var(--activityColor);
                    color: #fff;
                }
            }
        }

    }
}

.dark {
    .seting-pop {
        background-color: rgba(27, 27, 27, 1) !important;

        .separator {
            background-color: rgba(255, 255, 255, 0.05);
        }

        .seting-item {
            color: #fff !important;
        }
    }
}
</style> -->
