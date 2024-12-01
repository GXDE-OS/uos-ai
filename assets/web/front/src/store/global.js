import { defineStore } from "pinia";
import { hexToRgb } from "@/utils";
export const useGlobalStore = defineStore({
  id: "global",
  state: () => ({
    chatQWeb: null,
    activityColor: '',
    loadTranslations: {},
    AssistantType: {
      UOS_AI: 0x0001,
      UOS_SYSTEM_ASSISTANT: 0x0002,
      DEEPIN_SYSTEM_ASSISTANT: 0x0003,
      PERSONAL_KNOWLEDGE_ASSISTANT: 0x0004
    }
  }),
  getters: {},
  actions: {
    updateActivityColor(color) {
      console.log(color)
      this.activityColor = color;
      // 更新活动色
      const rgb = hexToRgb(color).rgbStr;
      // 设置样式
      document.body.style.setProperty("--activityColor", color);
      document.body.style.setProperty('--boxShadow', `rgba(${rgb},0.3)`);
      document.body.style.setProperty('--borderColor', `rgba(${rgb},0.2)`);
      document.body.style.setProperty('--backgroundColor', `rgba(${rgb},0.1)`);
      document.body.style.setProperty('--activityColorHover', `rgba(${rgb},0.9)`);
    },
    updateTheme(res) {
      // 浅色1  深色2
      const theme = res === 2 ? 'dark' : 'light';
      document.querySelector("html").setAttribute("class", theme);
    },
    updateFont(family, pixelSize) {
      document.documentElement.style.fontSize = pixelSize + 'px';  
      document.documentElement.style.fontFamily = family;
      document.body.style.setProperty('--font-family', family);
    }
  },
});
