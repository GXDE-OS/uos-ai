import {
  defineStore
} from "pinia";
import { hexToRgb } from "@/utils";
export const useGlobalStore = defineStore({
  id: "global",
  state: () => ({
    chatQWeb: null,
    activityColor: '',
    loadTranslations: {}
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
    },
    updateTheme(res) {
      // 浅色1  深色2
      const theme = res === 2 ? 'dark' : 'light'
      document.querySelector("html").setAttribute("class", theme);
    }
  },
});