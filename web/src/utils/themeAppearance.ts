import { hexToRgb } from "./hexToRgb";
import { useMainWindowStore } from "@/stores/mainwindow";

export const updateActiveColor = (activeColor: string) => {
    console.log("activeColor : ", activeColor);
    document.documentElement.style.setProperty("--system-active-color", activeColor);
    const rgb = hexToRgb(activeColor).rgbStr; // Convert hex to rgb
    document.documentElement.style.setProperty("--system-active-color-alpha-05", `rgba(${rgb},0.05)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-10", `rgba(${rgb},0.1)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-20", `rgba(${rgb},0.2)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-30", `rgba(${rgb},0.3)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-40", `rgba(${rgb},0.4)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-50", `rgba(${rgb},0.5)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-60", `rgba(${rgb},0.6)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-70", `rgba(${rgb},0.7)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-80", `rgba(${rgb},0.8)`);
    document.documentElement.style.setProperty("--system-active-color-alpha-90", `rgba(${rgb},0.9)`);
};

export const updateFont = (fontInfo: string) => {
    console.log("fontInfo : ", fontInfo); // 字体大小
    const fontInfoList = fontInfo.split("#");
    document.documentElement.style.fontFamily = fontInfoList[0] as string; // 字体类型
    document.documentElement.style.fontSize = fontInfoList[1] + "px"; // 字体大小
    document.body.style.setProperty("--font-family", fontInfoList[0]);
};

export const updateThemeColor = (themeColor: number) => {
    console.log("themeColor : ", themeColor);
    const theme = themeColor === 2 ? "dark" : "light";
    document.querySelector("html")?.setAttribute("class", theme);

    // 更新 isDarkMode 状态
    const mainWindowStore = useMainWindowStore();
    mainWindowStore.isDarkMode = themeColor === 2;
};
