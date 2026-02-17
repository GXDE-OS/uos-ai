<template>
    <div class="private-welcome-page">
        <div class="topIcon">
            <img :src="isDarkMode ? 'icons/private-wlecome-dark.png' : 'icons/private-welcome-light.png'"/>
        </div>
        <div class="title">
            {{ store.loadTranslations['Now in Private Chat'] }}
        </div>
        <div class="description">
            {{ store.loadTranslations['Private Chat messages are not saved in history and will be permanently deleted when you leave the chat.'] }}
        </div>
    </div>
</template>
<script setup>
import { useGlobalStore } from '@/store/global'

const store = useGlobalStore()

const props = defineProps({
    isDarkMode: {
        type: Boolean,
        default: false
    }
});
</script>

<style lang="scss" scoped>
.private-welcome-page {
    width: 100%;
    height: 100%;
    display: flex;  // 调整为列布局
    flex-direction: column;
    justify-content: center;
    align-items: center;

    .topIcon {
        display: flex;
        justify-content: center;
        align-items: center;
        width: 48px;
        height: 48px;
        background-color: var(--uosai-color-private-welcome-page-icon-bg);
        border-radius: 10px;

        img {
            width: 48px;
            height: 48px;
            fill: var(--uosai-color-private-welcome-page-icon);
        }
    }

    .title {
        width:auto;
        height:25px;
        font-size:1.21rem;
        font-weight:700;
        text-align:center;
        color: var(--uosai-color-private-welcome-page-title);
        margin-top: 20px;
    }

    .description {
        width:auto;
        height:auto;
        font-size:1rem;
        font-weight:400;
        font-style:normal;
        text-align:center;
        color: var(--uosai-color-private-welcome-page-description);
        margin-top: 6px;
        margin: 6px 32px 0 32px;
    }
}
</style>