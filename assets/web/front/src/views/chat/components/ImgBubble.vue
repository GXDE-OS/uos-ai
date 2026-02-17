<template>
    <div class="img-bubble">
        <div v-show="isPosterShow" style="margin-bottom: 10px;">{{getTitle}}</div>
        <div class="img-list">
            <div class="img-item" v-for="(item, index) in imageItemUrls"
                :style="{'min-width':!isWindowMode ? 'calc((100% - 10px)/2)' : 'calc((100% - 30px)/4)', 
                         'margin-right':imageItemUrls.length > 1 && isWindowMode && index != imageItemUrls.length -1 ? '10px' : '0px', 
                         'margin-left':imageItemUrls.length > 1 && !isWindowMode && (index + 1) % 2 == 0 ? '10px': '0px',
                         'margin-bottom':imageItemUrls.length > 1 && !isWindowMode && (index < imageItemUrls.length - 2) ? '10px': '0px'}">
                <img @click="previewImage(content[index])" :src="`${item}`" alt="">
                <div class="img-btn" v-show="!isPPTShow">
                    <div v-show="isPosterShow" class="btn" @click="imgEdit(index)">
                        <SvgIcon icon="edit" />
                        <span>{{store.loadTranslations['edit']}}</span>
                    </div>
                    <div class="btn" :class="{ disabled }" @click="!disabled && Qrequest(chatQWeb.saveImageAs, content[index], true)">
                        <SvgIcon icon="download" />
                        <span>{{store.loadTranslations['save']}}</span>
                    </div>
                    <div class="btn" @click="handleCopy(content[index])">
                        <SvgIcon icon="copy" />
                        <span>{{store.loadTranslations['copy']}}</span>
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
const store = useGlobalStore()
const props = defineProps(['content', 'disabled', 'isPPTShow', 'isPosterShow','isWindowMode'])
const emit = defineEmits(['handleShowTip' ,'clickOnPPT', 'clickOnEdit'])
const handleCopy = async (item) => {
    await Qrequest(chatQWeb.copyImage2Clipboard, item)
    emit('handleShowTip')
}
const previewImage = (item) => {
    if (!props.isPPTShow) {
        Qrequest(chatQWeb.previewImage, item)
    } else {
       emit('clickOnPPT')
    }
}

const imgEdit = (index) => {
     emit('clickOnEdit', index)
}

const getTitle = computed(() => {
     if (props.isPosterShow) {
        if (props.content.length>1){
            return '已为您生成海报：'
        } else {
            return '海报内容已更新：'
        }
     }
     return ''
})

const imageItemUrls = computed(() => {
    let urls = []
    for (let index = 0; index < props.content.length; index++) {
        const element = genImg(props.content[index])
        urls.push(element)
    }
    return urls
})

const genImg = (imgBase64_) =>{
    const blob = base64ToBlob(imgBase64_);
    return createObjectURL(blob);
}

// 将Base64字符串转换为Blob
function base64ToBlob(base64) {
    const byteCharacters = atob(base64);
    const byteNumbers = new Array(byteCharacters.length);
    for (let i = 0; i < byteCharacters.length; i++) {
        byteNumbers[i] = byteCharacters.charCodeAt(i);
    }
    const byteArray = new Uint8Array(byteNumbers);
    return new Blob([byteArray], { type: 'image/png' });
}

// 创建Object URL
function createObjectURL(blob) {
  return URL.createObjectURL(blob);
}

const isWindowMode = computed(() => {
    return props.isWindowMode
})
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
                bottom: 0px;
                right: 5px;
                display: none;
                flex-direction: column;
                align-items: flex-start;

                .btn {
                    border-radius: 8px;
                    background-color: rgba(0, 0, 0, 0.5);
                    box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.1);
                    backdrop-filter: blur(2px);
                    border: 1px solid rgba(255, 255, 255, 0.1);
                    width: 60px;
                    height: 24px;
                    margin-bottom: 6px;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    cursor: pointer;

                    span {
                        opacity: 1; /* 文本的不透明度 */
                        color: rgba(255, 255, 255, 0.9); /* 文本颜色，带有透明度 */
                        font-weight: 400; /* 字体粗细 */
                        font-style: normal; /* 字体样式 */
                        letter-spacing: 0px; /* 字间距 */
                        text-align: left; /* 文本对齐方式 */
                    }

                    svg {
                        fill: rgba(255, 255, 255, 0.9);
                        margin-right: 3px;
                        width: 12px;
                        height: 12px;
                    }

                    &:hover {
                        background-color: rgba(0, 0, 0, 0.6);

                        svg {
                            fill: rgba(255, 255, 255, 1);
                        }

                        span {
                            color: rgba(255, 255, 255, 1);
                        }
                    }
                
                    &:active {
                        background-color:  rgba(0, 0, 0, 0.7);

                        svg {
                            fill: var(--activityColor);
                        }

                        span {
                            color: var(--activityColor);
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

        // .img-item:nth-child(2n) {
        //     margin-left: 10px
        // }

        // .img-item:nth-child(-n+2) {
        //     margin-bottom: 10px
        // }


    }
}
</style>
