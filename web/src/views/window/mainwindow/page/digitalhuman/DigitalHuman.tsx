import { defineComponent, computed, onMounted, onBeforeUnmount, ref } from "vue";
import SilenceState from "./components/SilenceState";
import ListenState from "./components/ListenState";
import ThinkingState from "./components/ThinkingState";
import AnswerState from "./components/AnswerState";
import ErrorState from "./components/ErrorState";
import DigitalHumanAction from "./components/DigitalHumanAction";
import { useBackendStore, useDigitalHumanStore } from "@/stores";
import { AudioEvent, DigitalHumanState } from "@/types/DigitalHuman";

export default defineComponent({
    name: "DigitalHuman",
    emits: {
        // 状态变化事件
        stateChange: (state: DigitalHumanState) => true,
    },
    setup(props, { emit }) {
        const backendStore = useBackendStore();
        const digitalHumanStore = useDigitalHumanStore();
        const audioChannel = backendStore.audioChannel as any;

        // 快捷键
        const shortcutList = ref<string[]>([]);

        // 设置 backendStore 到 digitalHumanStore
        digitalHumanStore.backendStore = backendStore;

        // 音频事件回调函数
        const audioEventCallback = (event: AudioEvent, id: string, json: string) => {
            handleAudioStateChange(event, id, json);
        };

        // ================================用户操作======================================
        // 处理状态组件点击事件
        const handleStateClick = (state: DigitalHumanState) => {
            digitalHumanStore.handleStateClick(state, backendStore);
        };

        // 处理发送按钮点击事件
        const handleSendClick = () => {
            digitalHumanStore.handleSendClick(backendStore);
        };

        // 处理停止按钮点击事件
        const handleStopClick = () => {
            digitalHumanStore.handleStopClick(backendStore);
        };

        // 监听状态变化事件
        const handleAudioStateChange = (audioState: AudioEvent, id: string, result: string) => {
            digitalHumanStore.handleAudioStateChange(audioState, id, result);

            // 处理文本接收完成后的发送逻辑
            // if (audioState === AudioEvent.AeTextReceived) {
            //     try {
            //         const ret = JSON.parse(result);
            //         if (ret && ret.isEnd && !digitalHumanStore.manualClickBtnStop) {
            //             digitalHumanStore.stopRecorderAndSend(backendStore);
            //         }
            //     } catch (error) {
            //         console.error("handleAudioStateChange error", result, error);
            //     }
            // }
        };

        // 监听音频事件
        onMounted(async () => {
            // 监听音频事件
            if (audioChannel && audioChannel.audioEvent) {
                audioChannel?.audioEvent.connect(audioEventCallback);
            }

            // 加载快捷键
            const shortcuts = await backendStore.requestSystem("getCurrentTalkShortcut");
            console.log("current talk shortcuts:", shortcuts);
            // 解析快捷键字符串，如 "<Shift><Control><Alt><Super>A" 转换为 ["Shift", "Control", "Alt", "Super", "A"]
            if (shortcuts && typeof shortcuts === "string") {
                // 移除尖括号并分割成数组
                const keys = shortcuts.replace(/[<>]/g, " ").trim().split(/\s+/);
                shortcutList.value = keys.filter((key) => key.length > 0);

                // 将Control转换为Ctrl
                shortcutList.value = shortcutList.value.map((key) => (key === "Control" ? "Ctrl" : key));
            } else {
                shortcutList.value = [];
            }
        });

        // 组件卸载时清除所有计时器
        onBeforeUnmount(() => {
            digitalHumanStore.clearAllTimers();
            if (audioChannel && audioChannel.audioEvent) {
                // 移除音频事件监听器
                audioChannel?.audioEvent.disconnect(audioEventCallback);
            }
        });

        return {
            currentStateComponent: computed(() => {
                switch (digitalHumanStore.digitalHumanState) {
                    case DigitalHumanState.Silence:
                        return SilenceState;
                    case DigitalHumanState.Listen:
                        return ListenState;
                    case DigitalHumanState.Thinking:
                        return ThinkingState;
                    case DigitalHumanState.Answer:
                        return AnswerState;
                    case DigitalHumanState.Error:
                        return ErrorState;
                    default:
                        return SilenceState;
                }
            }),
            digitalHumanState: computed(() => digitalHumanStore.digitalHumanState),
            scale: computed(() => digitalHumanStore.scale),
            scale1: computed(() => digitalHumanStore.scale1),
            handleStateClick,
            handleSendClick,
            handleStopClick,
            showCountdown: computed(() => digitalHumanStore.showCountdown),
            countdownValue: computed(() => digitalHumanStore.countdownValue),
            errorMessage: computed(() => digitalHumanStore.errorMessage),
            model: computed(() => digitalHumanStore.model),
            shortcutList,
        };
    },

    render() {
        const StateComponent = this.currentStateComponent;

        return (
            <div class="digital-human">
                {this.digitalHumanState === DigitalHumanState.Listen ? (
                    <StateComponent scale={this.scale} scale1={this.scale1} onClick={this.handleStateClick} />
                ) : (
                    <StateComponent onClick={this.handleStateClick} />
                )}
                <DigitalHumanAction
                    state={this.digitalHumanState}
                    showCountdown={this.showCountdown}
                    countdownValue={this.countdownValue}
                    errorMessage={this.errorMessage}
                    model={this.model}
                    shortcutList={this.shortcutList}
                    onStart={this.handleSendClick}
                    onStop={this.handleStopClick}
                    onEnter={this.handleStateClick}
                />
            </div>
        );
    },
});
