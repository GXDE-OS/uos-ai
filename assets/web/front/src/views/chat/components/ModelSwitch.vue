<template>
    <div class="switch-model">
        <template v-if="accountList.length > 0">
            <div :class="disabled ? 'disabled model' : 'model'" :style="{'display': 'flex', 'max-width': props.isWindowMode ? '166px' : '110px'}" @click="clickModelSwitch">
                <img :src="`file://${currentAccount.icon}`" alt="">
                <el-tooltip v-if="currentAccount" popper-class="uos-tooltip" effect="light" :show-arrow="false"
                    :enterable="false" :show-after="1000" :offset="2" :content="currentAccount.displayname">
                    <div class="model-name">
                        {{ currentAccount.displayname || currentAccount.llmname }}
                    </div>
                </el-tooltip>
                <SvgIcon icon="combobox-arrow" />
            </div>
            <div class="model-menu" id="switchMenu" v-on-click-outside.bubble="() => showSwitchMenu = false" :style="{'height':accountList.length >= 10 ? '360px' : 'auto'}"
                v-show="showSwitchMenu">
                <custom-scrollbar class="model-scrollbar" id="model-page-scroll" :autoHideDelay="2000" :thumbWidth="6"
                :wrapperStyle="{ width: '100%', height: '100%' }" :style="{ height: '100%' }" :contentStyle="{'padding-right': '0'}">
                    <div class="menu-item" v-for="item in accountList" :key="item.index" @click="clickModelItem(item)">
                        <div class="menu-img" v-if="item.active">
                            <SvgIcon icon="ok-acitve" />
                        </div>
                        <div v-else class="pack"></div>
                        <div class="name">{{ item.displayname || item.llmname }}</div>
                    </div>
                    <!-- 分割线 -->
                     <div class="menu-split-line"></div>
                    <div class="menu-item" @click="Qrequest(chatQWeb.launchLLMConfigWindow, false)">
                        <div class="pack"></div>
                        <div class="name">{{ "+ " + store.loadTranslations['Add Model'] }}</div>
                    </div>
                </custom-scrollbar>
            </div>
        </template>
        <el-tooltip v-else popper-class="uos-tooltip" effect="light" :show-arrow="false" :enterable="false"
            :show-after="1000" :offset="2" :content="store.loadTranslations['No model available. Please install or configure a model in the settings.']">
            <div class="no-model" @click="Qrequest(chatQWeb.launchLLMConfigWindow, false)">
                <SvgIcon icon="no-model" />
                <div class="no-model-text">{{store.loadTranslations['No Model']}}</div>
                <!-- <SvgIcon icon="combobox-arrow" /> -->
            </div>
        </el-tooltip>
    </div>
</template>

<script setup>
import { vOnClickOutside } from '@/utils/VonClickOutside'
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";
import CustomScrollbar from 'custom-vue-scrollbar';
import { ref, getCurrentInstance } from 'vue';

const { chatQWeb } = useGlobalStore();
const store = useGlobalStore();
const props = defineProps(['currentAccount', 'accountList', 'disabled', 'isWindowMode'])
const emit = defineEmits(['update:currentAccount', 'update:accountList', 'update:currentAccountChanged'])
const instance = getCurrentInstance()

const showSwitchMenu = ref(false)

const clickModelSwitch = async (e) => {
    if (!showSwitchMenu.value && !props.disabled) {
        const resAccount = await Qrequest(chatQWeb.queryLLMAccountList)
        llmAccountLstChanged({ list: resAccount, id: props.currentAccount.id })
        showSwitchMenu.value = !showSwitchMenu.value
    }
}

const llmAccountLstChanged = (res) => {
    const { id, list } = res
    console.log("switch llmAccountLstChanged: ", res)
    const _list = JSON.parse(list)
    _list.forEach(element => {
        if (element.id === id) {
            element.active = true
            emit('update:currentAccount', element)
        }
    });
    emit('update:accountList', _list)
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
    emit('update:currentAccountChanged', item)
}

instance.proxy.$Bus.on("llmAccountLstChanged", llmAccountLstChanged);
onBeforeUnmount(() => {
    instance.proxy.$Bus.off('llmAccountLstChanged', llmAccountLstChanged)
})

defineExpose({ showSwitchMenu })
</script>

<style lang="scss" scoped>
.switch-model {
    display: flex;
    align-items: center;
    cursor: pointer;
    flex-shrink: 0;
    user-select: none;
    position: relative;
    margin-right: 10px;
    margin-top: 6px;
    max-width: 100%;
    height: 30px;

    .model {
        padding: 0px 5px 0px 5px;
        height: 30px;
        align-items: center;
        background-color: var(--uosai-think-search-normal-bg);
        border-radius: 8px;
        display: flex;
        box-sizing: border-box;

        .model-name {
            color: var(--uosai-color-modelbtn);
            font-size: 0.93rem;
            font-weight: 400;
            font-style: normal;
            letter-spacing: 0px;
            margin: 0 5px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;

            &:not(.disabled):active {
                color: var(--uosai-color-modelbtn-active-color);

                .icon-combobox-arrow {
                    fill: var(--uosai-color-modelbtn-active-color);
                }
            }
        }

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
            overflow: visible;
        }

        &:not(.disabled):hover {
            background: var(--uosai-color-modelbtn-hover);
            border-radius: 8px;
        }

        &:not(.disabled):active {
            background: var(--uosai-color-modelbtn-active);
            border-radius: 8px;

            .model-name {
                color: var(--uosai-color-modelbtn-active-color);
            }

            .icon-combobox-arrow {
                fill: var(--uosai-color-modelbtn-active-color);
            }
        }
    }
}

.model-menu {
    border-radius: 8px;
    box-shadow: 0px 6px 20px rgba(0, 0, 0, 0.2);
    background: var(--uosai-color-switchmenu-bg);
    padding: 8px 0;
    cursor: auto;
    position: absolute;
    bottom: 45px;
    // right: 0;
    width: fit-content;
    min-width: 162px;
    z-index: 10000;
    max-height: 360px;

    .model-scrollbar{
        overflow-y: overlay;
        overflow-x: hidden;
        height: 100%;
        width: 100%;
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
        display: flex;
        align-items: center;
        padding-right: 10px;

        .name {
            white-space: nowrap;
        }

        &:hover {
            background-color: var(--activityColor);
            color: #fff;

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

    .menu-split-line {
        width: 100%;
        height: 2px;
        background-color: var(--uosai-color-modelbtn-splitline);
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
    overflow: hidden;

    .icon-combobox-arrow {
        width: 8px;
        height: 5px;
        margin-left: 10px;
        overflow: visible;
    }
    .icon-no-model {
        margin-right: 5px;
        overflow: visible;
    }
    .no-model-text{
        white-space: nowrap;
        text-overflow: ellipsis;
        overflow: hidden;
    }
}
</style> 