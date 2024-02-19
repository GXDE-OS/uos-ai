<template>
    <div class="img-bubble">
        <div class="img-list">
            <div class="img-item" v-for="item in content">
                <img @click="Qrequest(chatQWeb.previewImage, item)" :src="`file://${item}`" alt="">
                <div class="img-btn">
                    <div class="btn" :class="{ disabled }" @click="!disabled && Qrequest(chatQWeb.saveImageAs, item)">
                        <SvgIcon icon="save" />
                    </div>
                    <div class="btn" style="margin-left: 5px;" @click="handleCopy(item)">
                        <SvgIcon icon="copy" />
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
import { Qrequest } from "@/utils";

const { chatQWeb } = useGlobalStore()
const props = defineProps(['content', 'disabled'])
const emit = defineEmits(['handleShowTip'])
const handleCopy = async (item) => {
    await Qrequest(chatQWeb.copyImage2Clipboard, item)
    emit('handleShowTip')
}

</script>

<style lang="scss" scoped>
.img-bubble {
    .img-list {
        display: flex;
        justify-content: flex-start;
        flex-wrap: wrap;

        .img-item {
            min-width: calc((100% - 10px)/2);
            flex: 1;
            position: relative;

            .img-btn {
                position: absolute;
                bottom: 5px;
                right: 5px;
                display: none;

                .btn {
                    border-radius: 4px;
                    background-color: var(--uosai-color-imgbubble-bg);
                    width: 24px;
                    height: 24px;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    cursor: pointer;

                    svg {
                        fill: rgba(255, 255, 255, 0.9);
                        width: 14px;
                        height: 13px;
                    }

                    .icon-copy {
                        width: 13px;
                        height: 13px;
                    }

                    &:hover {
                        background-color: var(--uosai-color-imgbubble-hover);

                        svg {
                            fill: rgba(255, 255, 255, 1);
                        }
                    }
                }
            }

            &:hover {
                .img-btn {
                    display: flex;
                }
            }

            img {
                width: 100%;
                height: 100%;
            }
        }

        .img-item:nth-child(2n) {
            margin-left: 10px
        }

        .img-item:nth-child(-n+2) {
            margin-bottom: 10px
        }

    }
}
</style>
