import { defineComponent, PropType, ref, watch } from "vue";
import type { SwitchCardData } from "@/types/conversation";
import SvgIcon from "@/components/SvgIcon";
import Switch from "@/components/Switch";
import { useBackendStore } from "@/stores";
import CardBase from "./CardBase";

/**
 * 开关卡片组件
 * 用于显示和控制开关状态（如蓝牙、WiFi、勿扰模式等）
 */
export default defineComponent({
  name: "SwitchCard",

  components: {
    SvgIcon,
    Switch,
    CardBase,
  },

  props: {
    data: {
      type: Object as PropType<SwitchCardData>,
      required: true,
    },
  },

  emits: {
    cardClick: (data: SwitchCardData) => !!data,
    switchChange: (data: SwitchCardData) => !!data,
  },

  setup(props, { emit }) {
    const backendStore = useBackendStore();
    // 本地开关状态，用于 UI 更新
    const localSwitch = ref(props.data.switch);

    // 监听 props 变化，更新本地状态
    watch(
      () => props.data.switch,
      (newSwitch) => {
        localSwitch.value = newSwitch;
      },
    );

    // 获取图标名称（根据title）
    const getIconName = (title: string) => {
      switch (title) {
        case "Wireless Network":
          return "wifi";
        case "Bluetooth":
          return "bluetooth";
        case "DND Mode":
          return "disturb";
        case "Eye Comfort":
          return "eye-protection";
        default:
          return "controlcenter";
      }
    };

    const handleCardClick = () => {
      emit("cardClick", props.data);

      // 跳转到控制中心
      try {
        // 根据标题判断跳转到哪个模块
        let module = "";
        if (props.data.title === "Eye Comfort") {
          module = "display";
        } else if (props.data.title === "Wireless Network") {
          module = "network";
        } else if (props.data.title === "Bluetooth") {
          module = "bluetooth";
        } else if (props.data.title === "DND Mode") {
          module = "notification"; // 通知模块
        } else {
          module = "display"; // 默认显示设置
        }
        backendStore.requestSystem("openControlCenter", module);
      } catch (error) {
        console.error("Failed to open control center:", error);
      }
    };

    const handleSwitchChange = async () => {
      const newState = !localSwitch.value;

      // 立即更新本地状态
      localSwitch.value = newState;

      // 创建新的数据对象，切换开关状态
      const newData = {
        ...props.data,
        switch: newState,
      };
      emit("switchChange", newData);

      // 调用后端接口更新开关状态
      try {
        // 根据标题调用不同的后端接口
        if (props.data.title === "Eye Comfort") {
          await backendStore.requestSystem("toggleEyesProtection", newState);
        } else if (props.data.title === "DND Mode") {
          await backendStore.requestSystem("doNoDisturb", newState);
        } else if (props.data.title === "Bluetooth") {
          await backendStore.requestSystem("doBluetoothConfig", newState);
        } else if (props.data.title === "Wireless Network") {
          await backendStore.requestSystem("switchWifi", newState);
        } else {
          // 其他开关，尝试通用处理
          console.warn("Unsupported switch type:", props.data.title);
        }
      } catch (error) {
        console.error("Failed to toggle switch:", error);
        // 如果后端调用失败，恢复本地状态
        localSwitch.value = !newState;
      }
    };

    return {
      localSwitch,
      handleCardClick,
      handleSwitchChange,
      getIconName,
      backendStore,
    };
  },

  render() {
    const { data } = this.$props;
    const icon = this.getIconName(data.title);

    return (
      <CardBase
        title={data.title}
        cardClassSuffix="switch"
        settingsIcon="controlcenter"
        showActionIcon={true}
        dividerWidth="230px"
        onActionClick={this.handleCardClick}
      >
        {/* 下半部分：功能开关 */}
        <div class="switch-card__left">
          <SvgIcon icon={icon} class="switch-card__bottom-icon" size={[16, 16]} />
          <span class="switch-card__bottom-title">{this.backendStore.translate(data.title)}</span>
        </div>
        <div class="switch-card__right">
          <Switch
            value={this.localSwitch}
            onChange={(value: boolean) => {
              this.handleSwitchChange();
            }}
          />
        </div>
      </CardBase>
    );
  },
});
