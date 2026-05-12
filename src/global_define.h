#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

#include <Qt>
#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
#define PARAM_SKIP_EMPTY QString::SkipEmptyParts
#else
#define  PARAM_SKIP_EMPTY Qt::SkipEmptyParts
#endif

namespace uos_ai {

inline constexpr char kDefaultAgentName[] = "default-agent";

inline constexpr char kAIDeepResearchAgentName[] = "ResearchAgent";
inline constexpr char kAIOutlineAgentName[] = "OutLineAgent";
inline constexpr char kAIArticleAdjustAgentName[] = "ArticleAdjustAgent";

// 初始化应用图标名称（在 Application 启动时调用一次）
void initApplicationIconName();

// 获取应用图标名称（init 后调用）
const char* getApplicationIconName();

inline constexpr char UosAPIAddress[] { "https://uosai.uniontech.com/api" };

enum WindowMode {
    WmNone = -1,
    WmMain = 0,
    WmMini = 1,
    WmSide = 2,
};

enum DockPosition {
    Top = 0,
    Right = 1,
    Bottom = 2,
    Left = 3
};

enum DockModel {
    Fashion = 0,
    Efficient = 1
};

enum SessionEvent {
    SeUnknown = 0,
    SeStarted = 1,
    SeFinished = 2,
    SeError = 3,
    SeMessage = 4,
};

enum FileEvent {
    FeUnknown     = 0,
    FeFileReady   = 1,  // file validated, frontend should show file card
    FeParseResult = 2,  // async parse complete
    FeNativeDrop  = 3,  // native webview drop, frontend decides whether to accept
    FeIncomingFiles = 4, // backend collected file paths, frontend applies unified upload constraints
};

enum AudioEvent {
    AeError = -1,
    AeSuccess = 0,
    AeLevelUpdated = 1,      // 录音音量级别更新
    AeTextReceived = 2,      // 语音识别文本接收
    AePlayFinished = 3,      // 音频播放完成
    AeRecordError = 4,       // 录音错误
    AePlayerError = 5,       // 播放错误
    AePlayDeviceChanged = 6, // 播放设备变化
    AeRecordDeviceChanged = 7 // 录音设备变化
};

enum SessionState {
    SsIdle = 0,
    SsReady,
    SsRunning = 3,
    SsWaiting = 4,
    SsFinished = 5,
};

enum ModelAbility {
    MaUnknown = 0,
    MaText = 1,
    MaImage = 1 << 1,
    MaToolCall = 1 << 2,
    MaReasoning = 1 << 3
};

Q_DECLARE_FLAGS(ModelAbilities, ModelAbility);

enum ModelArch {
    MaLanguage = 1,
    MaEmbedding,
    MaImageGeneration
};

enum ContentType {
    CntText = 0,  // 文本
    CntImage,  // 文生图
    CntFile,  // 文件
    CntTool, // 工具调用
    CntInstruction,  // 指令
    CntReasoning,  // 模型思考流（thinking token）
    CntAgentStep,  // Agent 任务步骤进度（title/status/items）
    CntOutline, // 大纲
    CntDocCard, // 文档卡片
    CntCommandCard, // 指令卡片
    CntGuessYouWant,  // 猜你想要
    CntError,  // 错误
};

enum NormalStatus {
    NsRunning = 0,
    NsCompleted,
    NsFailed,
    NsCanceled
};

enum GErrorType {
    NoError = 0,

    HttpError = 1,

    // session eror (1000 - 1199)
    InvalidSession = 1000,

    // assistant error (1200 - 1399)
    InvalidAssistant = 1200,

    // model error (1400 - 1499)
    InvalidModel = 1400,
    ModelExpired = 1401,
    ModelUsageLimitReached = 1402,
    ModelChatUsageLimitReached = 1403,
    ModelChatUsageClaimAgain = 1404,

    // file errors (1500 - 1599)
    FileInvalidSuffix      = 1500,
    FileSizeExceeded       = 1501,
    FileImageSizeExceeded  = 1502,
    FileParseNoText        = 1503,
    FileParseFailed        = 1504,

    // mcp error (1600 - 1699)
    AgentServerUnavailable = 1600,
    AgentServerInvaildContent,
    MCPSeverUnavailable,
    MCPToolError,

    // audio errors (1700 - 1799)
    AudioInputDeviceInvalid  = 1700,
    AudioOutputDeviceInvalid = 1701,
    AudioNetworkError        = 1702,

    // KnowledgeBase errors (1800 - 1899)
    KnowledgeBasePluginNotInstalled = 1801,
    KnowledgeBaseEmpty = 1802,
    KnowledgeBaseEmptyQuery = 1803,
};

enum TaskMode {
    UploadImage = 1,
    OverrideQuestion,
    AddKnowledgeBase,
    AddAskQuestion,
    ChangeToConversation,
    ChangeToDigitalMode,
};


struct GlobalUtil
{
    static QString contentTypeToString(ContentType type);
    static ContentType contentTypeFromString(const QString &str);
    static QString generateUuid();
    static QString generateMsId();
private:
    GlobalUtil();
};
}
#endif // GLOBAL_DEFINE_H
