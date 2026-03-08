import { defineStore } from "pinia";
import { hexToRgb } from "@/utils";

// 放在文件顶部或state外部
const CurrentAssistantFunctionButtonActiveIndex = [
    {
    assistantId: 0x0005, // AI_WRITING_ASSISTANT
    index: -1
    },
    {
    assistantId: 0x0006, // AI_TEXT_PROCESSING_ASSISTANT
    index: -1
    }
];

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
        PERSONAL_KNOWLEDGE_ASSISTANT: 0x0004,
        AI_WRITING_ASSISTANT: 0x0005, // 写作助手
        AI_TEXT_PROCESSING_ASSISTANT: 0x0006, // 文本处理助手
        AI_TRANSLATION_ASSISTANT: 0x0007, // 翻译助手
        PLUGIN_ASSISTANT:0x0100,
    },
    ChatAction:{
        ChatNone                : -1,     // 无
        ChatTextPlain           : 0,      // 纯文本聊天
        ChatFunctionCall        : 1,      // FunctionCall
        ChatText2Image          : 2,      // 文生图
        ChatTextThink           : 3,      // 思考内容
        ChatToolUse             : 4,      // 工具调用
    },
    ExtentionType:{
        None: 0,
        Conversation : 1,  //普通对话
        DocSummary: 2,  //文档总结
        WordSelectionLable:3,  //划词标签
        PerView:4,  //查看文档来源
        PromptTag:5,  //指令标签
        PictureId:6,  //图片Id
        LikeOrNot:7,  //点赞踩
        FunctionButton:8, //功能按钮
        McpBtnStatus:9, //MCP开关
        KnowledgeBaseBtnStatus:10, //知识库开关
    },
    DocParsingFileType:{
        Doc:0, //文档
        Image:1, //图片
    },
    ToolUseStatus:{
        Calling : 0,  //调用中
        Completed : 1, //完成
        Failed : 2, //失败
        Canceled : 3, //取消
    },
    ConversionMode:{
        Normal: 0, //普通模式
        Private: 1, //私密模式
    },
    ConversationModeStatus:0, //会话模式状态 0:普通模式 1:私密模式
    DeepSeek_Uos_Free: 81,  //免费官方账号模型类型
    IsDeepThink: true,  //是否深度思考
    IsSearchOnline: false,  //是否在线搜索
    IsOpenMcpServer: false,  //是否开启mcp服务
    IsOpenKnowledgeBase: false,  //是否开启知识库
    DefaultAgentName: "default-agent", // 默认MCP服务器
    IsInstallUOSAiAgent: false, // 是否安装UOSAI Agent
    LastModel: -1, // 最后使用的模型
    IsShowNonOfficialModelTip: false, // 是否显示过非官方模型提示
    IsGotFreeCredits: true,  // 是否领取过免费额度
    IsEnableAdvancedCssFeatures: false,  // 是否启用高级CSS特性（如backdrop-filter等）
    IsSimplifiedChinese: true,  // 是否为简体中文
    DocParsingStatusType:{
        Success:0x00,
        FileCountError:0x01,
        SuffixError:0x02,
        NoDocError:0x03,
        ExceedSize:0x04,
        ImageExceedSize:0x05
    },
    DocParserError:{
        Success:0x00,  //解析成功
        ParsingFailed:0x01,  //解析失败
        NoTextError:0x02,  //没有文字
        NoTextExtracted:0x03,  //没有提取到文字
    },
    QuestionAiAction:1,
    LikeOrNot:{  //点赞踩
        NONE:-1,
        LIKE:1,
        DISLIKE:2,
        EMPTY:3
    },
    CurrentAssistantFunctionButtonActiveIndex, // 直接引用外部定义的数组
    //日志等级定义
    LogLevel:{
        DEBUG:0,
        INFO:1,
        WARN:2,
        CRITICAL:3,
    },
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
            document.body.style.setProperty('--activityColorPromptTag', `rgba(${rgb},0.15)`);
            document.body.style.setProperty('--activityAddNewConversationBtnBgNormal', `rgba(${rgb},0.1)`);
            document.body.style.setProperty('--activityAddNewConversationBtnBgHover', `rgba(${rgb},0.15)`);
            document.body.style.setProperty('--activityAddNewConversationBtnBgActive', `rgba(${rgb},0.2)`);
            document.body.style.setProperty('--activityMcpserverAddConfirmbtnBg', `linear-gradient(rgba(${rgb}, 1) 0%, rgba(${rgb}, 1) 100%)`);
            document.body.style.setProperty('--activityColorPrivateModeInputBackgroundColor', `rgba(${rgb},0.08)`);
        },
        updateTheme(res) {
            // 浅色1  深色2
            const theme = res === 2 ? 'dark' : 'light';
            document.querySelector("html").setAttribute("class", theme);

            // 更新标签活动色
            const rgb = hexToRgb(this.activityColor).rgbStr;
            if (res == 2) {
                document.body.style.setProperty('--activityColorPromptTag', `rgba(${rgb},0.12)`);
                document.body.style.setProperty('--globalScrollbar-boxShadow', `rgba(0, 0, 0, 0.2) 0 0 0 0.5px, rgba(0, 0, 0, 0.2) 0 0 0 0.5px, rgba(255, 255, 255, 0.05) 0 0 0 0.5px inset, rgba(255, 255, 255, 0.05) 0 0 0 0.5px`);
            }else{
                document.body.style.setProperty('--activityColorPromptTag', `rgba(${rgb},0.15)`);
                document.body.style.setProperty('--globalScrollbar-boxShadow', `rgba(0, 0, 0, 0.05) 0 0 0 1px, rgba(0, 0, 0, 0.05) 0 0 10px 0`);
            }
        },
        updateFont(family, pixelSize) {
            console.log("pixelSize : ", pixelSize)
            document.documentElement.style.fontSize = pixelSize + 'px';  
            document.documentElement.style.fontFamily = family;
            document.body.style.setProperty('--font-family', family);
        },
        updateMainContentBackgroundColor(res) {
            console.log("updateMainContentBackgroundColor : ", res)
            document.body.style.setProperty("--main-content-background-color", res);
        }
    },
});