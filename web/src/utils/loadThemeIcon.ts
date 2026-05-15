import { ref, watch, type Ref } from "vue";
import { useBackendStore } from "@/stores";

export interface UseThemeIconOptions {
    /** 图标宽度，默认 16 */
    width?: number;
    /** 图标高度，默认 16 */
    height?: number;
    /** 加载失败时是否 fallback 到 iconName 原值，默认 false */
    fallbackToName?: boolean;
}

/**
 * 加载系统主题图标的 composable
 *
 * 封装从后端获取系统图标 base64 的逻辑，内置竞态条件处理。
 *
 * @param iconName - 图标名称（响应式）
 * @param options - 配置选项
 * @returns 图标 URL（base64、iconName 或空字符串）
 *
 * @example
 * const iconName = ref("uos-ai-assistant");
 * const iconUrl = useThemeIcon(iconName, { width: 24, height: 24 });
 * // iconUrl.value 会在 iconName 变化时自动更新
 *
 * // 带 fallback 的场景（加载失败时返回原 iconName）
 * const iconUrl = useThemeIcon(iconName, { fallbackToName: true });
 */
export function useThemeIcon(
    iconName: Ref<string | undefined>,
    options: UseThemeIconOptions = {},
): Ref<string> {
    const { width = 16, height = 16, fallbackToName = false } = options;
    const backend = useBackendStore();
    const iconUrl = ref<string>("");
    // 请求版本号，防止异步请求竞态覆盖
    let loadId = 0;

    const loadIcon = async () => {
        const currentLoadId = ++loadId;

        if (!iconName.value) {
            iconUrl.value = "";
            return;
        }

        try {
            const base64 = await backend.requestSystem(
                "getIconBase64",
                iconName.value,
                width,
                height,
            );
            // 仅当这是最新请求时才更新结果
            if (currentLoadId === loadId) {
                const result = (base64 as string) || "";
                iconUrl.value = result || (fallbackToName ? iconName.value : "");
            }
        } catch (e) {
            console.warn(`[useThemeIcon] Failed to load icon: ${iconName.value}`, e);
            if (currentLoadId === loadId) {
                iconUrl.value = fallbackToName ? iconName.value : "";
            }
        }
    };

    watch(
        [iconName, () => backend.themeIconVersion],
        loadIcon,
        { immediate: true },
    );

    return iconUrl;
}

/**
 * 一次性加载系统主题图标（非响应式）
 *
 * 适用于只需加载一次图标的场景，如 onMounted 中调用。
 *
 * @param iconName - 图标名称
 * @param width - 图标宽度，默认 16
 * @param height - 图标高度，默认 16
 * @returns Promise<string> - 图标 base64 URL 或空字符串
 */
export async function loadThemeIcon(
    iconName: string,
    width: number = 16,
    height: number = 16,
): Promise<string> {
    const backend = useBackendStore();

    if (!iconName) {
        return "";
    }

    try {
        const base64 = await backend.requestSystem(
            "getIconBase64",
            iconName,
            width,
            height,
        );
        return (base64 as string) || "";
    } catch (e) {
        console.warn(`[loadThemeIcon] Failed to load icon: ${iconName}`, e);
        return "";
    }
}
