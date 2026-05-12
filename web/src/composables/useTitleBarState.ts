import { ref } from "vue";

// 模块级单例——标题栏助手名称透明度（0=完全不可见，1=完全可见），由 WelcomeView 写入，Workspace 读取
const titleBarOpacity = ref(0);
const assistantName = ref("");
// 滚动容器是否已向上滚动（scrollTop > 0），由 ChatView 写入、Workspace 读取
const titleBarScrolled = ref(false);

export function useTitleBarState() {
    return { titleBarOpacity, assistantName, titleBarScrolled };
}
