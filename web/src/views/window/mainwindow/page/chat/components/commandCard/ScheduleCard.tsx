import { defineComponent, PropType, ref } from "vue";
import type { ScheduleCardData } from "@/types/conversation";
import CardBase from "./CardBase";
import SvgIcon from "@/components/SvgIcon";
import { useBackendStore } from "@/stores";
import { useThemeIcon } from "@/utils/loadThemeIcon";

/**
 * 日程卡片组件
 * 用于显示日程信息
 */
export default defineComponent({
    name: "ScheduleCard",

    components: {
        CardBase,
        SvgIcon,
    },

    props: {
        data: {
            type: Object as PropType<ScheduleCardData>,
            required: true,
        },
    },

    emits: {
        cardClick: (data: ScheduleCardData) => !!data,
    },

    setup(props, { emit }) {
        const backendStore = useBackendStore();

        // 使用 useThemeIcon 加载日历主题图标
        const calendarIconName = ref("dde-calendar");
        const calendarIconUrl = useThemeIcon(calendarIconName, { width: 16, height: 16 });

        const handleCardClick = async () => {
            emit("cardClick", props.data);

            // 跳转到日历应用
            try {
                backendStore.requestSystem("openCalendar", props.data.subject, props.data.startTime, props.data.endTime);
            } catch (error) {
                console.error("Failed to open calendar:", error);
            }
        };

        return {
            handleCardClick,
            backendStore,
            calendarIconUrl,
        };
    },

    methods: {
        formatDateString(dateString: string): { year: number; month: number; day: number; weekday: string } {
            try {
                const date = new Date(dateString);
                const days = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
                return {
                    year: date.getFullYear(),
                    month: date.getMonth() + 1,
                    day: date.getDate(),
                    weekday: this.backendStore.translate(days[date.getDay()] || 'Sunday')
                };
            } catch (error) {
                console.error("Failed to parse date:", dateString);
                return { year: new Date().getFullYear(), month: 1, day: 1, weekday: this.backendStore.translate('Sunday') };
            }
        },

        formatTime(timeString: string): string {
            try {
                const date = new Date(timeString);
                let hours = date.getHours();
                const minutes = date.getMinutes();
                const ampm = hours >= 12 ? 'pm' : 'am';
                hours = hours % 12;
                hours = hours ? hours : 12; // the hour '0' should be '12'
                const minutesStr = minutes < 10 ? '0' + minutes : minutes;
                return `${hours}:${minutesStr}${ampm}`;
            } catch (error) {
                console.error("Failed to parse time:", timeString);
                return timeString;
            }
        }
    },

    render() {
        const { data } = this.$props;
        const dateInfo = this.formatDateString(data.startTime);
        const startTime = this.formatTime(data.startTime);
        const endTime = this.formatTime(data.endTime);

        return (
            <CardBase
                title={this.backendStore.translate("Schedule Management")}
                cardClassSuffix="schedule"
                settingsIcon="calendar"
                settingsText={this.backendStore.translate("Schedule Management")}
                showActionIcon={true}
                dividerWidth="340px"
                onActionClick={this.handleCardClick}
            >
                {/* 自定义顶部插槽：使用日历主题图标 */}
                {{
                    top: () => (
                        <div class="card-base__top card-base__top--schedule">
                            {this.calendarIconUrl ? (
                                <img
                                    src={this.calendarIconUrl}
                                    class="card-base__settings-icon card-base__settings-icon--schedule"
                                    alt="calendar"
                                />
                            ) : (
                                <SvgIcon
                                    icon="calendar"
                                    class="card-base__settings-icon card-base__settings-icon--schedule"
                                    size={[16, 16]}
                                />
                            )}
                            <span class="card-base__settings-text card-base__settings-text--schedule">
                                {this.backendStore.translate("Schedule Management")}
                            </span>
                        </div>
                    ),
                    default: () => (
                        <>
                            {/* 日期显示 */}
                            <div class="schedule-card__date-display">
                                {`${dateInfo.year}/${String(dateInfo.month).padStart(2, '0')}/${String(dateInfo.day).padStart(2, '0')} ${dateInfo.weekday} ${startTime}`}
                            </div>

                            {/* 会议内容红色渐变文本框 */}
                            <div class="schedule-card__meeting-content">
                                {/* 红色竖向线条 */}
                                <div class="schedule-card__meeting-bar"></div>
                                <div class="schedule-card__meeting-text">{data.subject || '会议'}</div>
                            </div>
                        </>
                    )
                }}
            </CardBase>
        );
    },
});
