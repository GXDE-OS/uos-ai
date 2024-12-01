<template>
    <div class="switch-assistant">
        <template v-if="assistantList.length > 0">
            <div :class="disabled ? 'disabled assistant' : 'assistant'" style="display: flex;" @click="clickAssistantSwitch">
                <img :src='currentAssistant.iconPrefix + currentAssistant.icon + "-16.svg"' alt="">
                <el-tooltip v-if="currentAssistant" popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="currentAssistant.displayname">
                    <div class="assistant-name">
                        {{ currentAssistant.displayname}}
                    </div>
                </el-tooltip>
                <SvgIcon icon="combobox-arrow" />
            </div>
            <div class="assistant-menu" id="assistantMenu" v-on-click-outside.bubble="() => showAssistantMenu = false" 
                v-show="showAssistantMenu">
                <div class="assistant-menu-title">
                    {{store.loadTranslations['Assistant List']}}
                </div>
                <custom-scrollbar class="scrollbar" id="page-scroll" :autoHideDelay="2000" :thumbWidth="6"
                :wrapperStyle="{ width: '100%', height: 'calc(100% - 20px)' }" :style="{ height: '100%' }" :contentStyle="{'padding-right': '0'}">
                    <div class="assistant-menu-item" v-for="item in assistantList" :key="item.index" @click="clickAssistantItem(item)">
                        <div class="icon">
                            <img :src='item.iconPrefix + item.icon + "-32.svg"' alt="">
                        </div>
                        <div class="content">
                            <div class="name">{{ item.displayname }}</div>
                            <div class="description">{{ item.description }}</div>
                        </div>
                        <div class="active-icon" v-if="item.active">
                            <SvgIcon icon="ok-acitve" />
                        </div>
                        <div v-else class="pack"></div>
                    </div>
                </custom-scrollbar>

            </div>
        </template>
    </div>
    <div class="switch-model">
        <template v-if="accountList.length > 0">
            <div :class="disabled ? 'disabled model' : 'model'" style="display: flex;" @click="clickModelSwitch">
                <img :src="`file://${currentAccount.icon}`" alt="">
                <el-tooltip v-if="currentAccount" popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="currentAccount.displayname">
                    <div class="model-name">
                        {{ currentAccount.displayname || currentAccount.llmname }}
                    </div>
                </el-tooltip>
                <SvgIcon icon="combobox-arrow" />
            </div>
            <div class="model-menu" id="switchMenu" v-on-click-outside.bubble="() => showSwitchMenu = false" 
                v-show="showSwitchMenu">
                <div class="menu-item" v-for="item in accountList" :key="item.index" @click="clickModelItem(item)">
                    <div class="menu-img" v-if="item.active">
                        <SvgIcon icon="ok-acitve" />
                    </div>
                    <div v-else class="pack"></div>
                    <div class="name">{{ item.displayname || item.llmname }}</div>
                </div>
            </div>
        </template>
        <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="store.loadTranslations['No account, please configure an account']">
            <div class="no-model" @click="Qrequest(chatQWeb.launchLLMConfigWindow, true)">
                <SvgIcon icon="no-model" />
                {{store.loadTranslations['No account']}}
                <SvgIcon icon="combobox-arrow" />
            </div>
        </el-tooltip>
    </div>
</template>
<script setup>
import { vOnClickOutside } from '@/utils/VonClickOutside'
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import CustomScrollbar from 'custom-vue-scrollbar';

const { chatQWeb } = useGlobalStore();
const store = useGlobalStore();
const props = defineProps(['currentAccount', 'accountList', 'disabled', 'assistantList', 'currentAssistant'])
const emit = defineEmits(['update:currentAccount', 'update:accountList', 'update:assistantList', 'update:currentAssistant'])
const instance = getCurrentInstance()

const showSwitchMenu = ref(false)
const showAssistantMenu = ref(false)

const clickModelSwitch = async (e) => {
    if (!showSwitchMenu.value && !props.disabled) {
        const resAccount = await Qrequest(chatQWeb.queryLLMAccountList)
        llmAccountLstChanged({ list: resAccount, id: props.currentAccount.id })
        showSwitchMenu.value = !showSwitchMenu.value
        nextTick(() => {
            const localtion = e.target.getBoundingClientRect();
            const visualHeight = document.documentElement.clientHeight;
            const distanceB = visualHeight - localtion.bottom;
            const switchMenu = document.querySelector('#switchMenu')
            switchMenu.style.top = distanceB < switchMenu.clientHeight + 10 ? `-${switchMenu.clientHeight + 10}px` : '30px'
        })
    }
}

const clickAssistantSwitch = async (e) => {
    if (!showAssistantMenu.value && !props.disabled) {
        const resAssistant = await Qrequest(chatQWeb.queryAssistantList)
        // llmAccountLstChanged({ list: resAccount, id: props.currentAccount.id })
        showAssistantMenu.value = !showAssistantMenu.value
        nextTick(() => {
            // const localtion = e.target.getBoundingClientRect();
            // console.log("localtion.right: ", localtion.right);
            const visualWidth = document.documentElement.clientWidth;
            // console.log("visualWidth: ", visualWidth);

            const assistantMenu = document.querySelector('#assistantMenu');
            assistantMenu.style.top = `-${assistantMenu.clientHeight + 10}px`;
            // if (visualWidth < 460)
                // assistantMenu.style.left = `-${assistantMenu.clientWidth / 2 - 110}px`;
            // else
                // assistantMenu.style.left = `-${assistantMenu.clientWidth / 2}px`;
        })
    }
}

const llmAccountLstChanged = (res) => {
    const { id, list } = res
    console.log("switch llmAccountLstChanged: ", res)
    const _list = JSON.parse(list)
    const index = _list.findIndex(item => item.id === props.currentAccount.id)
    if (!id) emit('update:currentAccount', {})
    // if (index > -1) {
    //     _list[index].active = true
    //     emit('update:currentAccount', _list[index])
    // } else {
        _list.forEach(element => {
            if (element.id === id) {
                element.active = true
                emit('update:currentAccount', element)
            }
        });
    // }

    emit('update:accountList', _list)
}

const clickAssistantItem = async (item) => {
    await Qrequest(chatQWeb.setCurrentAssistantId, item.id)
    props.assistantList.forEach(element => {
        element.active = false
        if (element.id === item.id) {
            element.active = true
        }
    });
    showAssistantMenu.value = false
    emit('update:currentAssistant', item)
}

const clickModelItem = async (item) => {
    await Qrequest(chatQWeb.setCurrentLLMAccountId, item.id)
    props.accountList.forEach(element => {
        element.active = false
        if (element.id === item.id) {
            element.active = true
        }
    });
    showSwitchMenu.value = false
    emit('update:currentAccount', item)
}

instance.proxy.$Bus.on("llmAccountLstChanged", llmAccountLstChanged);
onBeforeUnmount(() => {
    instance.proxy.$Bus.off('llmAccountLstChanged', llmAccountLstChanged)
})
defineExpose({ showSwitchMenu })
// defineExpose({ showAssistantMenu })
</script>
<style lang="scss" scoped>
.switch-assistant {
    display: flex;
    align-items: center;
    cursor: pointer;
    flex-shrink: 0;
    user-select: none;
    margin-right: 2px;

    .assistant {
        padding: 0px 5px 0px 5px;
        height: 36px;
        max-width: 150px;
        align-items: center;
        background-color: var(--uosai-color-modelbtn-bg);
        // border-radius: 8px;
        border-top-left-radius: 8px;
        border-bottom-left-radius: 8px;

        &.disabled {
            cursor: not-allowed;

            &:active {
                background-color: var(--uosai-color-modelbtn-bg);
                // border-radius: 8px;
                border-top-left-radius: 8px;
                border-bottom-left-radius: 8px;

                .assistant-name {
                    color: var(--uosai-color-modelbtn);
                }

                .icon-combobox-arrow {
                    color: var(--uosai-color-modelbtn);
                }
            }
        }

        .icon-combobox-arrow {
            width: 8px;
            height: 5px;
            fill: var(--uosai-color-modelbtn);
        }

        &:not(.disabled):hover {
            background: var(--uosai-color-modelbtn-hover);
            // border-radius: 8px;
            border-top-left-radius: 8px;
            border-bottom-left-radius: 8px;
        }

        &:not(.disabled):active {
            background: var(--uosai-color-modelbtn-active);
            // border-radius: 8px;
            border-top-left-radius: 8px;
            border-bottom-left-radius: 8px;

            .assistant-name {
                color: var(--activityColor);
            }

            .icon-combobox-arrow {
                fill: var(--activityColor);
            }
        }
    }

    .assistant-name {
        color: var(--uosai-color-modelbtn);
        font-size: 0.93rem;
        font-weight: 400;
        font-style: normal;
        letter-spacing: 0px;
        margin-left: 5px;
        min-width: 90px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;

        &:not(.disabled):hover {
            color: var(--uosai-color-modelbtn-hover-color);
        }

        &:not(.disabled):active {
            color: var(--activityColor);

            .icon-combobox-arrow {
                fill: var(--activityColor);
            }
        }
    }
}

.switch-model {
    display: flex;
    align-items: center;
    cursor: pointer;
    flex-shrink: 0;
    user-select: none;
    position: relative;

    .model {
        padding: 0px 5px 0px 5px;
        height: 36px;
        align-items: center;
        background-color: var(--uosai-color-modelbtn-bg);
        // border-radius: 8px;
        border-top-right-radius: 8px;
        border-bottom-right-radius: 8px;

        &.disabled {
            cursor: not-allowed;

            &:active {
                background-color: var(--uosai-color-modelbtn-bg);
                border-radius: 8px;

                .model-name {
                    color: var(--uosai-color-modelbtn);
                }

                .icon-combobox-arrow {
                    color: var(--uosai-color-modelbtn);
                }
            }
        }

        .icon-combobox-arrow {
            width: 8px;
            height: 5px;
            fill: var(--uosai-color-modelbtn);
        }

        &:not(.disabled):hover {
            background: var(--uosai-color-modelbtn-hover);
            // border-radius: 8px;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
        }

        &:not(.disabled):active {
            background: var(--uosai-color-modelbtn-active);
            // border-radius: 8px;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;

            .model-name {
                color: var(--activityColor);
            }

            .icon-combobox-arrow {
                fill: var(--activityColor);
            }
        }
    }

    .model-name {
        color: var(--uosai-color-modelbtn);
        font-size: 0.93rem;
        font-weight: 400;
        font-style: normal;
        letter-spacing: 0px;
        margin-left: 5px;
        max-width: 90px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;

        &:not(.disabled):hover {
            color: var(--uosai-color-modelbtn-hover-color);
        }

        &:not(.disabled):active {
            color: var(--activityColor);

            .icon-combobox-arrow {
                fill: var(--activityColor);
            }
        }
    }
}

.icon-combobox-arrow {
    width: 8px;
    height: 5px;
    margin-left: 10px;
    // opacity: 0.7;
}

.assistant-menu {
    // height: fit-content;
    border-radius: 18px;
    box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.2);
    background: var(--uosai-color-assistantmenu-bg);
    padding: 8px 4px;
    cursor: auto;
    position: absolute;
    width: 330px;
    // min-width: 330px;
    height: 345px;
    max-height: 345px;
    z-index: 10000;
    overflow: hidden;
    right: 0;

    .assistant-menu-title {
        width: 100%;
        margin-bottom: 4px;
        margin-left: 14px;
        font-size: 1.14rem;
        color: var(--uosai-color-title);
    }

    .scrollbar {
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
        width: 100%;
        .assistant-menu-item {
            opacity: 1;
            color: var(--uosai-color-title);
            cursor: pointer;
            display: flex;
            align-items: center;
            margin: 10px 6px;
            background-color: var(--uosai-color-modelbtn-bg);
            border-radius: 8px;
            padding: 15px 8px 10px 20px;

            &:hover {
                background-color: var(--activityColor);
                color: #fff;

                .active-icon {
                    svg {
                        fill: #fff;
                    }
                }
                .content {
                    .name, .description{
                        color: #fff;
                    }
                }
            }

            .icon {
                width: 30px;
                height: 30px;
                img{
                    width: 30px;
                    height: 30px;
                }
            }
            .content {
                display: flex;
                flex-direction: column;
                width: 100%;
                margin: 0 40px 0 20px;

                .name {
                    font-size: 1rem;
                    font-weight: 500;
                    color:  var(--uosai-color-title);
                    margin-bottom: 1px;
                }
                .description {
                    font-size: 0.86rem;
                    color:  var(--uosai-color-title);
                }
            }
            .active-icon,
            .pack {
                width: 14px;
                height: 14px;
                // margin-right: 7px;
                margin-left: 10px;
            }

            .active-icon {
                display: flex;
                align-items: center;

                svg {
                    fill: var(--uosai-color-title);
                    width: 14px;
                    height: 12px;
                }
            }
        }
    }
}

.model-menu {
    height: auto;
    border-radius: 8px;
    // border: 1px solid rgba(0, 0, 0, 0.05);
    box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.2);
    background: var(--uosai-color-switchmenu-bg);
    padding: 8px 0;
    cursor: auto;
    position: absolute;
    top: 30px;
    right: 0;
    width: fit-content;
    min-width: 162px;
    z-index: 10000;


    .menu-title {
        opacity: 0.5;
        color: rgba(0, 0, 0, 1);
        font-size: 0.85rem;
        font-weight: 400;
        font-style: normal;
        letter-spacing: 0px;
        text-align: left;
        margin-left: 10px;
        // margin-top: 10px;
        margin-bottom: 4px;
    }

    .scroll {
        max-height: 170px;
        overflow-y: auto;
    }

    .menu-item {
        opacity: 1;
        color: var(--uosai-color-title);
        font-size: 1rem;
        font-style: normal;
        letter-spacing: 0px;
        cursor: pointer;
        height: 34px;
        line-height: 34px;
        // position: relative;
        display: flex;
        align-items: center;
        padding-right: 10px;

        .name {
            white-space: nowrap;
        }

        &:hover {
            background-color: var(--activityColor);
            color: #fff;
            // box-shadow: 0px 4px 6px rgba(44, 167, 248, 0.4);

            .menu-img {
                svg {
                    fill: #fff;
                }
            }
        }

        .menu-img,
        .pack {
            width: 14px;
            height: 14px;
            margin-right: 7px;
            margin-left: 10px;
        }

        .menu-img {
            display: flex;
            align-items: center;

            svg {
                width: 13px;
                height: 11px;
                fill: var(--uosai-color-title);
                width: 14px;
                height: 12px;
            }
        }
    }
}

.no-model {
    display: flex;
    align-items: center;
    color: var(--uosai-color-modelbtn);
    font-size: 1rem;
    padding: 0px 5px 0px 5px;
    align-items: center;
    height: 36px;
    background-color: var(--uosai-color-modelbtn-bg);
    border-radius: 8px;

    svg {
        margin-bottom: -2px;
        margin-right: 5px;
    }
}
</style>
