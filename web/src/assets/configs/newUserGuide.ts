import type { NewUserGuidePage } from "@/components/NewUserGuideDialog";
import guideEn01Dark from "@/assets/images/guide_en_01_dark.png";
import guideEn01Light from "@/assets/images/guide_en_01_light.png";
import guideEn02Dark from "@/assets/images/guide_en_02_dark.png";
import guideEn02Light from "@/assets/images/guide_en_02_light.png";
import guideEn03Dark from "@/assets/images/guide_en_03_dark.png";
import guideEn03Light from "@/assets/images/guide_en_03_light.png";
import guideEn04Dark from "@/assets/images/guide_en_04_dark.png";
import guideEn04Light from "@/assets/images/guide_en_04_light.png";
import guideEn05Dark from "@/assets/images/guide_en_05_dark.png";
import guideEn05Light from "@/assets/images/guide_en_05_light.png";
import guideZh01Dark from "@/assets/images/guide_zh_01_dark.png";
import guideZh01Light from "@/assets/images/guide_zh_01_light.png";
import guideZh02Dark from "@/assets/images/guide_zh_02_dark.png";
import guideZh02Light from "@/assets/images/guide_zh_02_light.png";
import guideZh03Dark from "@/assets/images/guide_zh_03_dark.png";
import guideZh03Light from "@/assets/images/guide_zh_03_light.png";
import guideZh04Dark from "@/assets/images/guide_zh_04_dark.png";
import guideZh04Light from "@/assets/images/guide_zh_04_light.png";
import guideZh05Dark from "@/assets/images/guide_zh_05_dark.png";
import guideZh05Light from "@/assets/images/guide_zh_05_light.png";

type Translate = (key: string) => string;
type GuideLocale = "en" | "zh";

interface NewUserGuideImageConfig {
    light: string;
    dark: string;
}

interface NewUserGuidePageConfig {
    image: Record<GuideLocale, NewUserGuideImageConfig>;
    titleKey: string;
    descriptionKeys: string[];
}

const createGuideImageConfig = (
    enLight: string,
    enDark: string,
    zhLight: string,
    zhDark: string,
): Record<GuideLocale, NewUserGuideImageConfig> => ({
    en: {
        light: enLight,
        dark: enDark,
    },
    zh: {
        light: zhLight,
        dark: zhDark,
    },
});

// Keep copy as translation keys so it resolves after backend.loadTranslations() completes.
const NEW_USER_GUIDE_PAGE_CONFIGS: NewUserGuidePageConfig[] = [
    {
        image: createGuideImageConfig(guideEn01Light, guideEn01Dark, guideZh01Light, guideZh01Dark),
        titleKey: "UOS AI 3.0: All-New UI Upgrade",
        descriptionKeys: [
            "New Windowed Mode for a fresh interaction experience",
            "Clear sidebar navigation for organized access and ease of use",
            "A wider, focused workspace with richer content",
            "Clearer hierarchy to keep core tasks focused and immersive",
        ],
    },
    {
        image: createGuideImageConfig(guideEn02Light, guideEn02Dark, guideZh02Light, guideZh02Dark),
        titleKey: "Multi-tasking to save your valuable time",
        descriptionKeys: [
            "Silent background multi-tasking: No more waiting",
            "Assign tasks anytime without breaking your flow",
            "Get instant progress updates without constant monitoring",
        ],
    },
    {
        image: createGuideImageConfig(guideEn03Light, guideEn03Dark, guideZh03Light, guideZh03Dark),
        titleKey: "Immersive split-screen writing: Accurate, secure, and traceable",
        descriptionKeys: [
            "Split-screen chat & edit: No more window switching",
            'Deep data "feeding" for well-grounded content creation',
            "Outline first with manual correction and one-click export",
            "On-device/Private models ensure data privacy and security",
        ],
    },
    {
        image: createGuideImageConfig(guideEn04Light, guideEn04Dark, guideZh04Light, guideZh04Dark),
        titleKey: "System control & vast Skills: All within a single command",
        descriptionKeys: [
            "Visual system control: Adjust fonts, toggle Wi-Fi, and more",
            "High-frequency office skills: Writing, translation, and merging",
            "One-click Skill import to unlock endless capabilities",
        ],
    },
    {
        image: createGuideImageConfig(guideEn05Light, guideEn05Dark, guideZh05Light, guideZh05Dark),
        titleKey: "Batch management and precise search for history",
        descriptionKeys: [
            "Global quick search: Access history in seconds",
            "Agent-based filtering for precise chat history search",
            "Efficient batch management for a clean and organized workspace",
        ],
    },
];

const getGuideLocale = (isChineseLanguage: boolean): GuideLocale => (isChineseLanguage ? "zh" : "en");

const getGuideImage = (
    imageConfig: Record<GuideLocale, NewUserGuideImageConfig>,
    isDarkMode: boolean,
    isChineseLanguage: boolean,
) => {
    const locale = getGuideLocale(isChineseLanguage);
    const variant = imageConfig[locale];
    return isDarkMode ? variant.dark : variant.light;
};

export const getNewUserGuidePages = (
    translate: Translate,
    isDarkMode: boolean,
    isChineseLanguage: boolean,
): NewUserGuidePage[] => {
    return NEW_USER_GUIDE_PAGE_CONFIGS.map((page) => ({
        image: getGuideImage(page.image, isDarkMode, isChineseLanguage),
        title: translate(page.titleKey),
        description: page.descriptionKeys.map((descriptionKey) => translate(descriptionKey)),
    }));
};
