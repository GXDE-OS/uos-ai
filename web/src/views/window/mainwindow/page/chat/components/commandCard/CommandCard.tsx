import { defineComponent, PropType } from "vue";
import type { CommandCardData } from "@/types/conversation";
import SwitchCard from "./SwitchCard";
import SliderCard from "./SliderCard";
import FontSizeCard from "./FontSizeCard";
import AppStoreCard from "./AppStoreCard";
import ScheduleCard from "./ScheduleCard";

/**
 * 指令卡片基础组件
 * 根据卡片数据中的title选择对应的子组件
 */
export default defineComponent({
    name: "CommandCard",

    components: {
        SwitchCard,
        SliderCard,
        FontSizeCard,
        AppStoreCard,
        ScheduleCard,
    },

    props: {
        data: {
            type: Object as PropType<CommandCardData>,
            required: true,
        },
    },

    emits: {
        cardClick: (data: CommandCardData) => !!data,
    },

    setup(props, { emit }) {
        const handleCardClick = (data: CommandCardData) => {
            emit("cardClick", data);
        };

        return {
            handleCardClick,
        };
    },

    render() {
        const { data } = this.$props;

        // 根据cardData中的title来判断卡片类型
        const title = data.cardData?.title;

        console.log("[CommandCard] Rendering card with title:", title, "cardData:", data.cardData);

        if (!title) {
            console.warn("[CommandCard] No title found in cardData");
            return null;
        }

        // Switch类型：根据title判断
        const switchTitles = ["Wireless Network", "Bluetooth", "DND Mode", "Eye Comfort"];
        if (switchTitles.includes(title)) {
            return <SwitchCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }

        // Slider类型：根据title判断
        const sliderTitles = ["Brightness", "Volume"];
        if (sliderTitles.includes(title)) {
            return <SliderCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }

        // FontSize类型：根据title判断
        if (title === "Font Size") {
            return <FontSizeCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }

        // 其他类型
        if (title.includes("App Store")) {
            return <AppStoreCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }
        if (title.includes("Schedule")) {
            return <ScheduleCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }

        // 兼容旧版本：根据cardType字符串判断
        if (data.cardType === "switch_card") {
            return <SwitchCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }
        if (data.cardType === "slider_card") {
            return <SliderCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }
        if (data.cardType === "app_store_card") {
            return <AppStoreCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }
        if (data.cardType === "schedule_card") {
            return <ScheduleCard data={data.cardData} onCardClick={(_cardData: any) => this.handleCardClick(data)} />;
        }

        console.warn("[CommandCard] Unknown cardType/title:", data.cardType, title);
        return null;
    },
});
