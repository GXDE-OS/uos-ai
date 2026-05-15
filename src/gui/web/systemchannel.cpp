#include "systemchannel.h"
#include "esystemcontext.h"
#include "dbus/networkmonitor.h"
#include "deepinabilitymanager.h"
#include "oscallcontext.h"
#include "gui/window/windowmanager.h"

#include <QIcon>
#include <QPixmap>
#include <QBuffer>
#include <QLoggingCategory>
#include <QDebug>
#include <QApplication>
#include <QColor>
#include <QClipboard>
#include <QFileInfo>
#include <QTimer>
#include <QImageReader>
#include <QFontInfo>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <DApplication>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)
Q_DECLARE_LOGGING_CATEGORY(logAIVUE)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

SystemChannel::SystemChannel(QObject *parent)
    : QObject(parent)
{
    // Initialize active color from system theme
    m_activeColor = DGuiApplicationHelper::instance()->applicationPalette()
                    .color(DPalette::Normal, DPalette::Highlight)
                    .name(QColor::HexRgb);

    m_themeColor = DGuiApplicationHelper::instance()->themeType();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SystemChannel::onThemeTypeChanged);
    connect(qApp, &DApplication::iconThemeChanged, this, &SystemChannel::themeIconChanged);
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged, this, &SystemChannel::networkChanged);

    const auto kServiceName = QStringLiteral("org.freedesktop.Notifications");
    const auto kObjectPath = QStringLiteral("/org/freedesktop/Notifications");
    const auto kInterfaceName = QStringLiteral("org.freedesktop.Notifications");

    m_notificationProxy.reset(new QDBusInterface(
        kServiceName,
        kObjectPath,
        kInterfaceName,
        QDBusConnection::sessionBus(),
        this
    ));
    if (!m_notificationProxy->isValid()) {
        qCWarning(logAIGUI) << "Freedesktop notification interface is invalid";
    }

    const bool actionConnected = QDBusConnection::sessionBus().connect(
        kServiceName,
        kObjectPath,
        kInterfaceName,
        QStringLiteral("ActionInvoked"),
        this,
        SLOT(onNotificationActionInvoked(uint,QString))
    );
    if (!actionConnected) {
        qCWarning(logAIGUI) << "Failed to connect ActionInvoked signal from org.freedesktop.Notifications";
    }

    const bool closedConnected = QDBusConnection::sessionBus().connect(
        kServiceName,
        kObjectPath,
        kInterfaceName,
        QStringLiteral("NotificationClosed"),
        this,
        SLOT(onNotificationClosed(uint,uint))
    );
    if (!closedConnected) {
        qCWarning(logAIGUI) << "Failed to connect NotificationClosed signal from org.freedesktop.Notifications";
    }

    // 初始化查询快捷键
    searchShortCut();
}

SystemChannel::~SystemChannel()
{
}

QString SystemChannel::getIconBase64(const QString &iconName, int width, int height)
{
    qCDebug(logAIGUI) << "Getting icon base64:" << iconName << "size:" << width << "x" << height;

    if (iconName.isEmpty()) {
        qCWarning(logAIGUI) << "Icon name is empty";
        return QString();
    }

    // Validate icon size parameters
    if (width <= 0 || height <= 0) {
        qCWarning(logAIGUI) << "Invalid icon size:" << width << "x" << height;
        return QString();
    }

    // Load icon from system theme
    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) {
        qCWarning(logAIGUI) << "Icon not found in theme:" << iconName;
        return QString();
    }

    // 获取系统缩放系数，在高DPI显示器上获取更高分辨率的图标
    qreal scaleFactor = qApp->devicePixelRatio();
    if (scaleFactor <= 0) {
        scaleFactor = 1.0;
    }

    // 计算实际像素尺寸（宽高乘以缩放系数）
    int actualWidth = qRound(width * scaleFactor);
    int actualHeight = qRound(height * scaleFactor);

    qCDebug(logAIGUI) << "Icon scaled size:" << actualWidth << "x" << actualHeight
                      << "with scale factor:" << scaleFactor;

    // Get pixmap with scaled size
    QPixmap pixmap = icon.pixmap(actualWidth, actualHeight);
    if (pixmap.isNull()) {
        qCWarning(logAIGUI) << "Failed to create pixmap for icon:" << iconName;
        return QString();
    }

    // Convert QPixmap to QByteArray
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    if (!buffer.open(QIODevice::WriteOnly)) {
        qCWarning(logAIGUI) << "Failed to open buffer for icon:" << iconName;
        return QString();
    }

    if (!pixmap.save(&buffer, "PNG")) {
        qCWarning(logAIGUI) << "Failed to save pixmap to buffer for icon:" << iconName;
        buffer.close();
        return QString();
    }
    buffer.close();

    // Convert to base64 with data URI prefix
    QString base64 = QString::fromLatin1(byteArray.toBase64());
    return QString("data:image/png;base64,%1").arg(base64);
}

QJsonObject SystemChannel::loadTranslations()
{
    static QJsonObject translations;
    if (!translations.isEmpty())
        return translations;

    translations["UOS AI"] = tr("UOS AI");  // 小U
    translations["Ask UOS AI, kiss your worries goodbye."] = tr("Ask UOS AI, kiss your worries goodbye.");  // 问问小U，事事无忧
    translations["Update History"] = tr("Update History");  // 更新记录
    translations["Settings"] = tr("Settings");  // 设置
    translations["Help"] = tr("Help");  // 帮助
    translations["About"] = tr("About");  // 关于
    translations["New Chat"] = tr("New Chat");  // 创建新对话
    translations["Temporary Chat"] = tr("Temporary Chat");  // 临时对话
    translations["AI Writing"] = tr("AI Writing");  // AI写作
    translations["AI Translation"] = tr("AI Translation");  // AI翻译
    translations["AI Knowledge Base"] = tr("AI Knowledge Base");  // AI知识库
    translations["MCP Server"] = tr("MCP Server");  // MCP服务
    translations["Commands"] = tr("Commands");  // 指令
    translations["Agent Store"] = tr("Agent Store");  // 智能体商店
    translations["More"] = tr("More");  // 更多
    translations["Chat History"] = tr("Chat History");  // 历史对话
    translations["Today"] = tr("Today");  // 今天
    translations["Yesterday"] = tr("Yesterday");  // 昨天
    translations["Last 7 Days"] = tr("Last 7 Days");  // 过去7天
    translations["Last 30 Days"] = tr("Last 30 Days");  // 过去30天
    translations["January"] = tr("January");  // 1月
    translations["February"] = tr("February");  // 2月
    translations["March"] = tr("March");  // 3月
    translations["April"] = tr("April");  // 4月
    translations["May"] = tr("May");  // 5月
    translations["June"] = tr("June");  // 6月
    translations["July"] = tr("July");  // 7月
    translations["August"] = tr("August");  // 8月
    translations["September"] = tr("September");  // 9月
    translations["October"] = tr("October");  // 10月
    translations["November"] = tr("November");  // 11月
    translations["year"] = tr("year");  // 年
    translations["Earlier"] = tr("Earlier");  // 更早
    translations["Delete"] = tr("Delete");  // 删除
    translations["Confirm Deletion"] = tr("Confirm Deletion");  // 删除对话确认
    translations["Confirm deletion"] = tr("Confirm deletion");
    translations["After deletion, this server will be unavailable. Proceed with caution."] = tr("After deletion, this server will be unavailable. Proceed with caution.");
    translations["This action will delete all content related to this chat from UOS AI."] = tr("This action will delete all content related to this chat from UOS AI.");  // 此操作将从 UOSAI 中删除该对话相关的所有内容。
    translations["Confirm Delete"] = tr("Confirm Delete");  // 确认删除
    translations["Cancel"] = tr("Cancel");  // 取消
    translations["Batch Manage"] = tr("Batch Manage");  // 批量管理
    translations["All"] = tr("All");  // 全部
    translations["Select All"] = tr("Select All");  // 全选
    translations["%1 conversations selected"] = tr("%1 conversations selected");  // 已选择%1个对话
    translations["No chat history yet"] = tr("No chat history yet");  // 您还没有历史对话
    translations["Chat content not found"] = tr("Chat content not found");  // 找不到对话内容
    translations["DeepThink"] = tr("DeepThink");  // 深度思考
    translations["Search"] = tr("Search");  // 联网搜索
    translations["Attachments"] = tr("Attachments");  // 附件
    translations["Upload Files"] = tr("Upload Files");  // 上传文件
    translations["Screenshot Q&A"] = tr("Screenshot Q&A");  // 截图问答
    translations["Voice Input"] = tr("Voice Input");  // 语音输入
    translations["Send"] = tr("Send");  // 发送
    translations["Ask a question..."] = tr("Ask a question...");  // 请输入您的问题
    translations["Voice Read"] = tr("Voice Read");  // 语音朗读
    translations["Stop Reading"] = tr("Stop Reading");  // 停止朗读
    translations["Copy"] = tr("Copy");  // 复制
    translations["Regenerate"] = tr("Regenerate");  // 重新生成
    translations["Re-edit"] = tr("Re-edit");  // 重新编辑
    translations["Stop Generating"] = tr("Stop Generating");  // 停止生成
    translations["Generation stopped"] = tr("Generation stopped");  // 已停止生成
    translations["You stopped this answer, "] = tr("You stopped this answer, ");  // 您已停止了本次回答，
    translations["please re-edit your question"] = tr("please re-edit your question");  // 重新编辑问题
    translations["Back to Bottom"] = tr("Back to Bottom");  // 回到底部
    translations["Thinking..."] = tr("Thinking...");  // 思考中
    translations["Deep think completed (took %1s)"] = tr("Deep think completed (took %1s)");  // 已深度思考（用时%1秒）
    translations["Parsing..."] = tr("Parsing...");  // 解析中
    translations["Parsing failed"] = tr("Parsing failed");  // 解析失败
    translations["You can add up to 50 files"] = tr("You can add up to 50 files");  // 最多只能添加50个文件
    translations["Summarize the core content of the file"] = tr("Summarize the core content of the file");  // 整理文件的核心内容
    translations["Model List"] = tr("Model List");  // 模型列表
    translations["Smart Recommendation"] = tr("Smart Recommendation");  // 智能推荐
    translations["Smart switch for best match"] = tr("Smart switch for best match");  // 智能切换最佳匹配
    translations["Local Model"] = tr("Local Model");  // 本地模型
    translations["Online Model"] = tr("Online Model");  // 在线模型
    translations["Add Model"] = tr("Add Model");  // 添加模型
    translations["Private Model"] = tr("Private Model"); //私有化模型
    translations["Official"] = tr("Official");  // 官方
    translations["AI-generated content is for reference only. Please verify its accuracy."] = tr("AI-generated content is for reference only. Please verify its accuracy.");  // AI生成的内容仅供参考，请注意甄别信息准确性。
    translations["Temporary chats are not saved in history. The content will be completely deleted upon leaving."] = tr("Temporary chats are not saved in history. The content will be completely deleted upon leaving.");  // 临时对话不会显示在历史记录中，离开对话界面时，其内容会被完全删除。
    translations["Maximum of 10 concurrent chats reached. Please try again later."] = tr("Maximum of 10 concurrent chats reached. Please try again later.");  // 最多同时进行10个对话，请稍后发送
    translations["View Now"] = tr("View Now");  // 立即查看
    translations["Remind Me Later"] = tr("Remind Me Later");  // 稍后再说
    translations["Infinite inspiration, worry-free writing"] = tr("Infinite inspiration, worry-free writing");  // 灵感无限，写作无忧
    translations["Enable to search the web for more real-time, comprehensive, and accurate references."] = tr("Enable to search the web for more real-time, comprehensive, and accurate references.");  // 开启后，会联网搜索资料，获取更实时、全面、准确的参考信息
    translations["Upload files/images as references"] = tr("Upload files/images as references");  // 上传文件/图片作为参考资料
    translations["Reference Outline / Local File"] = tr("Reference Outline / Local File");  // 参考大纲 / 本地文件
    translations["As reference material / As outline file"] = tr("As reference material / As outline file");  // 作为参考素材 / 作为大纲文件
    translations["File Upload"] = tr("File Upload");  // 文件上传
    translations["As Material"] = tr("As Material");  // 作为参考素材
    translations["As Outline"] = tr("As Outline");  // 作为大纲文件
    translations["Only supports uploading 1 outline file"] = tr("Only supports uploading 1 outline file");  // 仅支持上传1个大纲文件
    translations["Local Materials"] = tr("Local Materials");  // 本地素材
    translations["File Outline"] = tr("File Outline");  // 文件大纲
    translations["Outline"] = tr("Outline");  // 大纲
    translations["editor toc"] = QApplication::translate("MarkdownEditor", "Outline"); // 大纲
    translations["No outline"] = QApplication::translate("MarkdownEditor", "No outline"); // 暂无大纲
    translations["View All"] = tr("View All");  // 查看全部
    translations["Recent Creations"] = tr("Recent Creations");  // 最近创作
    translations["Upload File"] = tr("Upload File");  // 上传文件
    translations["Reference Outline"] = tr("Reference Outline");  // 参考大纲
    translations["Local File"] = tr("Local File");  // 本地文件
    translations["Heading"] = tr("Heading");  // 标题
    translations["Export Document"] = tr("Export Document");  // 导出文档
    translations["Exit Full Screen"] = tr("Exit Full Screen");  // 退出全屏
    translations["Untitled Document"] = tr("Untitled Document");  // 未命名文档
    translations["If you don't want local materials to be uploaded, you can do the following before generating content:"] = tr("If you don't want local materials to be uploaded, you can do the following before generating content:");  // 如果您不希望本地素材上传，可在输入框做如下操作后再来生成内容：
    translations["1. Switch to a local model (e.g., DeepSeek-R1-1.5B) or a privately deployed model"] = tr("1. Switch to a local model (e.g., DeepSeek-R1-1.5B) or a privately deployed model");  // 1、将模型切换为本地模型（如DepSeek-R1-1.5B）或私有化部署模型
    translations["2. Turn off \"Web Search\""] = tr("2. Turn off \"Web Search\"");  // 2、将"联网搜索"设置为未选中状态
    translations["Materials will be uploaded to the online model for analysis. Continue generating?"] = tr("Materials will be uploaded to the online model for analysis. Continue generating?");  // 素材将会上传至在线模型分析，是否继续生成？
    translations["Materials will be uploaded to the online model (%1) for analysis. Continue generating?"] = tr("Materials will be uploaded to the online model (%1) for analysis. Continue generating?");  // 素材将会上传至在线模型分析，是否继续生成？
    translations["Enable Privacy Mode"] = tr("Enable Privacy Mode");  // 一键开启隐私模式
    translations["Continue Generating"] = tr("Continue Generating");  // 继续生成
    translations["You can enter more requirements to optimize or adjust the generated content."] = tr("You can enter more requirements to optimize or adjust the generated content.");  // 您还可以继续输入更多要求，对已生成的内容优化或调整
    translations["Detected local outline uploaded. Analyzing outline content..."] = tr("Detected local outline uploaded. Analyzing outline content...");  // 检测到你已上传本地大纲，正在为你解析大纲内容
    translations["Generating outline content..."] = tr("Generating outline content...");  // 正在为你生成大纲内容
    translations["An editable outline has been generated. After confirming, click the blue button below to proceed to document generation."] = tr("An editable outline has been generated. After confirming, click the blue button below to proceed to document generation.");  // 已为你生成了可以自由编辑的大纲，确认无误后，点击下方蓝色按钮，进入到生成文档环节
    translations["Unable to parse the uploaded outline file. Please re-upload."] = tr("Unable to parse the uploaded outline file. Please re-upload.");  // 无法解析你上传的大纲文件，请重新上传
    translations["Re-upload Outline"] = tr("Re-upload Outline");  // 重新上传大纲
    translations["Directly generate outline using AI"] = tr("Directly generate outline using AI");  // 直接使用AI生成大纲
    translations["Add sub-chapter"] = tr("Add sub-chapter");  // 添加子章节
    translations["Delete sub-chapter"] = tr("Delete sub-chapter");  // 删除子章节
    translations["Delete chapter"] = tr("Delete chapter");  // 删除章节
    translations["Add chapter"] = tr("Add chapter");  // 增加章节
    translations["Enter chapter title"] = tr("Enter chapter title");  // 输入章节标题
    translations["Delete this heading?"] = tr("Delete this heading?");  // 是否删除该标题？
    translations["Generate document from outline"] = tr("Generate document from outline");  // 基于大纲生成文档
    translations["Save as Word"] = tr("Save as Word");  // 另存为Word
    translations["Save as PDF"] = tr("Save as PDF");  // 另存为PDF
    translations["Save as Markdown"] = tr("Save as Markdown");  // 另存为Markdown
    translations["Saving..."] = tr("Saving...");  // 另存中...
    translations["Saved successfully!"] = tr("Saved successfully!");  // 另存成功！
    translations["Failed to save, please try again."] = tr("Failed to save, please try again.");  // 另存失败，请再试一次
    translations["Undo"] = tr("Undo");  // 撤销
    translations["Redo"] = tr("Redo");  // 恢复
    translations["Body Text"] = tr("Body Text");  // 正文
    translations["Heading %1"] = tr("Heading %1");  // %1级标题
    translations["Bold"] = tr("Bold");  // 粗体
    translations["Italic"] = tr("Italic");  // 斜体
    translations["Strikethrough"] = tr("Strikethrough");  // 删除线
    translations["Link Text"] = tr("Link Text");  // 链接文本
    translations["Bulleted List"] = tr("Bulleted List");  // 无序列表
    translations["Numbered List"] = tr("Numbered List");  // 有序列表
    translations["Decrease Indent"] = tr("Decrease Indent");  // 减少缩进
    translations["Increase Indent"] = tr("Increase Indent");  // 增加缩进
    translations["Blockquote"] = tr("Blockquote");  // 引用
    translations["Divider"] = tr("Divider");  // 分割线
    translations["Copy Full Text"] = tr("Copy Full Text");  // 复制全文
    translations["Print Document"] = tr("Print Document");  // 打印文档
    translations["Share Document"] = tr("Share Document");  // 分享文档
    translations["Zoom Out"] = tr("Zoom Out");  // 缩小
    translations["Full Screen"] = tr("Full Screen");  // 全屏
    translations["Close"] = tr("Close");  // 关闭
    translations["Insert Link"] = tr("Insert Link");  // 插入链接
    translations["Text:"] = tr("Text:");  // 文本：
    translations["Link:"] = tr("Link:");  // 链接：
    translations["Please enter a valid link"] = tr("Please enter a valid link");  // 请输入有效的链接
    translations["Link text"] = tr("Link text");  // 链接文本
    translations["References"] = qApp->translate("MarkdownEditor", "References");  // 参考资料
    translations["Please enter the text to be translated first."] = tr("Please enter the text to be translated first.");  // 请先输入需要翻译的内容
    translations["Identifying source language..."] = tr("Identifying source language...");  // 我正在识别源语言语种...
    translations["My MCP Server"] = tr("My MCP Server");  // 我的MCP服务
    translations["Add MCP Server"] = tr("Add MCP Server");  // 添加MCP服务
    translations["Delete MCP Server"] = tr("Delete MCP Server");  // 删除MCP服务
    translations["Edit MCP Server"] = tr("Edit MCP Server");  // 编辑MCP服务
    translations["Built-in Only"] = tr("Built-in Only");  // 仅内置
    translations["Custom Added Only"] = tr("Custom Added Only");  // 仅自定义添加
    translations["To use MCP&Skills, install UOS AI Agent from the App Store first."] = tr("To use MCP&Skills, install UOS AI Agent from the App Store first.");  // 使用MCP服务需要先安装UOS AI Agent，请前往应用商店安装
    translations["To use AI Knowledge Base, install Embedding Plugins from App Store first."] = tr("To use AI Knowledge Base, install Embedding Plugins from App Store first.");  // 使用AI知识库需要先安装向量化模型插件，请前往应用商店安装
    translations["Install Now"] = tr("Install Now");  // 立即安装
    translations["System Settings"] = tr("System Settings");  // 系统设置
    translations["Bluetooth"] = tr("Bluetooth");  // 蓝牙
    translations["Wireless Network"] = tr("Wireless Network");  // WiFi
    translations["DND Mode"] = tr("DND Mode");  // 勿扰模式
    translations["Eye Comfort"] = tr("Eye Comfort");  // 护眼模式
    translations["Brightness"] = tr("Brightness");  // 屏幕亮度
    translations["Volume"] = tr("Volume");  // 系统音量
    translations["Font Size"] = tr("Font Size");  // 系统字号
    translations["App Store"] = tr("App Store");  // 应用商店
    translations["Schedule Management"] = tr("Schedule Management");  // 日程管理
    translations["Click to download"] = tr("Click to download");  // 点击前往下载
    translations["<10k downloads"] = tr("<10k downloads");  // <1万次下载
    translations["<100k downloads"] = tr("<100k downloads");  // <10万次下载
    translations["100k+ downloads"] = tr("100k+ downloads");  // 10万+次下载
    translations["%1 stars"] = tr("%1 stars");  // %1分
    translations["Click to go to app"] = tr("Click to go to app");  // 点击跳转到应用
    translations["Sunday"] = tr("Sunday");  // 星期日
    translations["Monday"] = tr("Monday");  // 星期一
    translations["Tuesday"] = tr("Tuesday");  // 星期二
    translations["Wednesday"] = tr("Wednesday");  // 星期三
    translations["Thursday"] = tr("Thursday");  // 星期四
    translations["Friday"] = tr("Friday");  // 星期五
    translations["Saturday"] = tr("Saturday");  // 星期六
    translations["MCP Servers"] = tr("MCP Servers");  // MCP服务
    translations["Select All MCP Servers"] = tr("Select All MCP Servers");  // 选择所有MCP服务
    translations["You can add and manage MCP servers"] = tr("You can add and manage MCP servers");  // 您可以添加、管理MCP服务
    translations["Describe"] = tr("Describe");  // 描述
    translations["JSON configuration"] = tr("JSON configuration");  // JSON配置
    translations["Please paste the MCP JSON configuration code into the input box."] = tr("Please paste the MCP JSON configuration code into the input box.");  // 请将MCP JSON配置代码粘贴到输入框中
    translations["Describe MCP server functions to facilitate quick search tools"] = tr("Describe MCP server functions to facilitate quick search tools");  // 描述MCP服务功能，方便快速搜索
    translations["Back"] = tr("Back");  // 返回
    translations["Free Credits Delivered"] = tr("Free Credits Delivered");  // 免费额度已送达
    translations["You've used up the free generation credits for your trial account. We've given you an extra 200 free credits valid this month. Explore more features and unlock UOS AI's limitless capabilities!"] = tr("You've've used up the free generation credits for your trial account. We've given you an extra 200 free credits valid this month. Explore more features and unlock UOS AI's limitless capabilities!");  // 当前试用账号模型的免费生成额度已用完，我们已为你额外赠送200次免费额度，本月有效。立即体验更多功能，探索UOS AI的无限可能吧！
    translations["Not Now"] = tr("Not Now");  // 暂不领取
    translations["writing_default_prompt"] = tr("I am {{enter identity/position}}. Help me write a {{report/article/outline/WeChat public account post/notice/research report/work summary/speech}} about {{enter topic}}, around {{1000}} words in length. The content requirements are {{enter requirements/content focus/writing style, etc..}}");
    translations["Manage Chat History"] = tr("Manage Chat History");  //管理历史对话
    translations["Voice Chat"] = tr("Voice Chat");  // 语音对话
    translations["Expand"] = tr("Expand");  // 展开
    translations["Collapse"] = tr("Collapse");  // 折叠
    translations["Edited on"] = tr("Edited on");  // 修改于
    translations["Created"] = tr("Created");  // 创建时间
    translations["You have %1 newly answered chats"] = tr("You have %1 newly answered chats");  // 您有%1个新对话已回答完毕
    translations["Search History"]=tr("Search History");  // 搜索历史会话
    translations["Confirm delete this conversation"] = tr("Confirm delete this conversation");  // 确认删除对话
    translations["This will remove all related content from UOS AI"] = tr("This will remove all related content from UOS AI");  // 此操作将从小U同学中删除该对话相关的所有内容
    translations["Use it now"] = tr("Use it now");  // 马上使用
    translations["My Skills"] = tr("My Skills");  // 我的技能
    translations["Open installation directory"] = tr("Open installation directory");  // 打开安装目录
    translations["Refresh"] = tr("Refresh");  // 刷新
    translations["Import Skill"] = tr("Import Skill");  // 导入技能
    translations["You can add and manage Skills"] = tr("You can add and manage Skills"); // 您可以添加、管理Skills
    translations["Exit Voice Chat"] = tr("Exit Voice Chat");  // 退出语音对话
    translations["The following %1 files are invalid and unavailable. Continue?"] = tr("The following %1 files are invalid and unavailable. Continue?");  // 以下%1个文件已失效，无法使用。是否继续？
    translations["The following file is invalid and unavailable. Continue?"] = tr("The following file is invalid and unavailable. Continue?");  // 以下文件已失效，无法使用。是否继续?
    // OLD
    translations["Go to configuration"] = tr("Go to configuration");
    translations["No account"] = tr("No account");
    translations["Input question"] = tr("Input question");
    translations["The content generated by AI is for reference only, please pay attention to the accuracy of the information."] = tr("The content generated by AI is for reference only, please pay attention to the accuracy of the information.");
    translations["Welcome to UOS AI"] = tr("Welcome to UOS AI");
    translations["Here are some of the things UOS AI can help you do"] = tr("Here are some of the things UOS AI can help you do");
    translations["Stop"] = tr("Stop");
    translations["Play"] = tr("Play");
    translations["Retry"] = tr("Retry");
    translations["Clear conversation history"] = tr("Clear conversation history");
    translations["Please connect the microphone and try again"] = tr("Please connect the microphone and try again");
    translations["Chat history cleared"] = tr("Chat history cleared");
    translations["Click to start/stop recording"] = tr("Click to start/stop recording");
    translations["Listening"] = tr("Listening");
    translations["Sleeping"] = tr("Sleeping");
    translations["Microphone not detected"] = tr("Microphone not detected");
    translations["Connection failed, click to try again"] = tr("Connection failed, click to try again");
    translations["Click on the animation%1 to activate"] = tr("Click on the animation%1 to activate");
    translations["Voice input is temporarily unavailable, please check the network!"] = tr("Voice input is temporarily unavailable, please check the network!");
    translations["Unable to connect to the server, please check your network or try again later."] = tr("Unable to connect to the server, please check your network or try again later.");
    translations["Voice conversation"] = tr("Voice conversation");
    translations["Click the animation or press Enter to send"] = tr("Click the animation or press Enter to send");
    translations["Stop recording after %1 seconds"] = tr("Stop recording after %1 seconds");
    translations["Thinking"] = tr("Thinking");
    translations["Click animation to interrupt"] = tr("Click animation to interrupt");
    translations["Answering"] = tr("Answering");
    translations["Your free account quota has been exhausted, please configure your model account to continue using it."] = tr("Your free account quota has been exhausted, please configure your model account to continue using it.");
    translations["Your free account has expired, please configure your model account to continue using it."] = tr("Your free account has expired, please configure your model account to continue using it.");
    translations["UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first."] = tr("UOS AI requires an AI model account to be configured before it can be used. Please configure a model account first.");
    translations["Activate"] = tr("Activate");
    translations["Voice input"] = tr("Voice input");
    translations["Voice broadcast is temporarily unavailable, please check the network!"] = tr("Voice broadcast is temporarily unavailable, please check the network!");
    translations["Turn off voice conversation"] = tr("Turn off voice conversation");
    translations["The picture has been generated, please switch to the chat interface to view it."] = tr("The picture has been generated, please switch to the chat interface to view it.");
    translations["No account, please configure an account"] = tr("No account, please configure an account");
    translations["Answer each question up to 5 times"] = tr("Answer each question up to 5 times");
    translations["Copied successfully"] = tr("Copied successfully");
    translations["Sound output device not detected"] = tr("Sound output device not detected");
    translations["The sound output device is not detected, please check and try again!"] = tr("The sound output device is not detected, please check and try again!");
    translations["Mode"] = tr("Mode");
    translations["Window Mode"] = tr("Window Mode");
    translations["Sidebar Mode"] = tr("Sidebar Mode");
    translations["Assistant List"] = tr("Assistant List");
    translations["Agent List"] = tr("Agent List");
    translations["UOS System Assistant"] = tr("UOS System Assistant");
    translations["Deepin System Assistant"] = tr("Deepin System Assistant");
    translations["Personal Knowledge Assistant"] = tr("Personal Knowledge Assistant");
    translations["Please configure the knowledge base"] = tr("Please configure the knowledge base");
    translations["knowledge base configure content"] = tr("Before using the [Personal Knowledge Assistant], it is necessary to configure the knowledge base. After configuring the knowledge base, AI will answer questions or generate content based on the content you have configured in the knowledge base.");
    translations["Please configure the large model"] = tr("Please configure the large model");
    translations["The personal knowledge assistant can only be used after configuring a large model."] = tr("The personal knowledge assistant can only be used after configuring a large model.");
    translations["To configure"] = tr("To configure");
    translations["To install"] = tr("To install");
    translations["Please install EmbeddingPlugins"] = tr("Please install [EmbeddingPlugins]");
    translations["EmbeddingPlugins install content"] = tr("This assistant requires the installation of the EmbeddingPlugins to run");

    // document summary
    translations["Drag files here to add them."] = tr("Drag files here to add them.");
    translations["You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc."] = tr("You can only add 3 files, supported file types include: txt, doc, docx, xls, xlsx, ppt, pptx, pdf, md, png, jpg, jpeg, code files, etc.");
    translations["You can only add a maximum of one file."] = tr("You can only add a maximum of one file.");
    translations["The file format is not supported."] = tr("The file format is not supported.");
    translations["Summarize the key content of the file."] = tr("Summarize the key content of the file.");
    translations["File Error"] = tr("File Error");
    translations["File has been deleted."] = tr("File has been deleted.");
    translations["The file size exceeds the 100MB limit."] = tr("The file size exceeds the 100MB limit.");
    translations["Upload a document"] = tr("Upload a document");
    translations["File deleted"] = tr("File deleted");
    translations["No text was parsed"] = tr("No text was parsed");
    translations["Reference"] = tr("Reference");

    // Instruction
    translations["Instruction"] = tr("Instruction");
    translations["Type \"/\" in the input box to activate."] = tr("Type \"/\" in the input box to activate.");
    translations["Please enter; “Ctrl+Enter” to change the line."] = tr("Please enter; “Ctrl+Enter” to change the line.");
    translations["Enter your question, or enter \"/\" to select a command\n\"Ctrl+Enter\"  to start a new line"] = tr("Enter your question, or enter \"/\" to select a command\n\"Ctrl+Enter\"  to start a new line");

    //联网搜索
    translations["Search complete."] = tr("Search complete.");
    translations["Click to view results"] = tr("Click to view results");

    // textToPicture
    translations["edit"] = tr("edit");
    translations["save"] = tr("save");
    translations["copy"] = tr("copy");

    // markdown
    translations["lines of code collapsed"] = tr("lines of code collapsed");
    translations["Expand"] = tr("Expand");

    // 2.6需求
    translations["Thinking has stopped"] = tr("Thinking has stopped");
    translations["Deeply thought (%1 seconds)"] = tr("Deeply thought (%1 seconds)");

    // 2.7需求
    translations["New Conversation"] = tr("New Conversation");
    translations["History"] = tr("History");
    translations["No History Records"] = tr("No History Records");
    translations["Are you sure to delete the conversation? It will be unrecoverable once deleted."] = tr("Are you sure to delete the conversation? It will be unrecoverable once deleted.");
    translations["The %1 agent used in this conversation has been deleted"] = tr("The %1 agent used in this conversation has been deleted");
    translations["This conversation cannot be viewed. To view it, please install the %1 agent and try again."] = tr("This conversation cannot be viewed. To view it, please install the %1 agent and try again.");
    translations["The original conversation model has been deleted. We have switched to a new model for you to continue the conversation."] = tr("The original conversation model has been deleted. We have switched to a new model for you to continue the conversation.");

    // 2.8需求
    translations["Recommendations"] = tr("Recommendations");
    translations["No Model"] = tr("No Model");
    translations["No model available. Please install or configure a model in the settings."] = tr("No model available. Please install or configure a model in the settings.");
    translations["Please Describe the Content Theme and Requirements for Your Creation."] = tr("Please Describe the Content Theme and Requirements for Your Creation.");
    translations["Please Enter the Content You Want to Translate and Specify the Target Language. Default Translation is to Chinese."] = tr("Please Enter the Content You Want to Translate and Specify the Target Language. Default Translation is to Chinese.");
    translations["Please Enter the Text You Need to Process and Specify Your Requirements."] = tr("Please Enter the Text You Need to Process and Specify Your Requirements.");
    translations["New Agent Added"] = tr("New Agent Added");
    translations["New Writing, Text Processing, and Translation Agents have been added. Check them out now."] = tr("New Writing, Text Processing, and Translation Agents have been added. Check them out now.");
    translations["Try it"] = tr("Try it");
    translations["Write an article based on the following document:"] = tr("Write an article based on the following document:");
    translations["Translate the following document into English:"] = tr("Translate the following document into English:");

    // 2.9需求
    translations["Add Mcp Server"] = tr("Add Mcp Server");  //添加MCP服务器
    translations["Add Server"] = tr("Add Server");
    translations["Add failed! Error reason:"] = tr("Add failed! Error reason:");
    translations["MCP environment missing. Please install 【UOS AI Agent】"] = tr("MCP environment missing. Please install 【UOS AI Agent】");
    translations["Calling"] = tr("Calling");
    translations["Completed"] = tr("Completed");
    translations["Call Failed"] = tr("Call Failed");
    translations["Cancelled"] = tr("Cancelled");
    translations["params"] = tr("params");
    translations["result"] = tr("result");
    translations["For MCP Server, switch to officially released model \"DeepSeek-Trial Account\""] = tr("For MCP Server, switch to officially released model \"DeepSeek-Trial Account\"");
    translations["Enter MCP Server command, e.g., \"Change system to dark mode for me\""] = tr("Enter MCP Server command, e.g., \"Change system to dark mode for me\"");
    translations["Agent server is not available"] = tr("Agent server is not available");  //智能体服务不可用 11000
    translations["Agent server exception"] = tr("Agent server exception");  //智能体服务异常 11001
    translations["MCP server is not available"] = tr("MCP server is not available");  //MCP服务不可用 11100
    translations["Confirm"] = tr("Confirm");
    translations["Automate multi-file and multi-app tasks with one command using MCP Service. Try it now!"] = tr("Automate multi-file and multi-app tasks with one command using MCP Service. Try it now!");
    translations["Use later"] = tr("Use later");
    translations["Enable MCP Server"] = tr("Enable MCP Server");
    translations["After installing the MCP environment \"UOS AI Agent\", click the "] = tr("After installing the MCP environment \"UOS AI Agent\", click the ");
    translations[" and select \"uos-mcp\" in the MCP server list."] = tr(" and select \"uos-mcp\" in the MCP server list.");
    translations["Try saying: \"Change system to dark mode\"."] = tr("Try saying: \"Change system to dark mode\".");
    translations["Try it now"] = tr("Try it now");
    translations["Add Mcp Server[GuidePage]"] = tr("Add Mcp Server[GuidePage]");  //新增MCP服务
    translations["First-time users: Install MCP environment \"UOS AI Agent\" via App Store."] = tr("First-time users: Install MCP environment \"UOS AI Agent\" via App Store.");
    translations["The JSON file format is incorrect, please check and submit again"] = tr("The JSON file format is incorrect, please check and submit again");
    translations["Install Now >"] = tr("Install Now >");  // 去安装

    // 2.10需求
    translations["General Chat"] = tr("General Chat");  // 普通对话
    translations["Now in Private Chat"] = tr("Now in Private Chat");  // 已经进入隐私对话
    translations["Private Chat messages are not saved in history and will be permanently deleted when you leave the chat."] = tr("Private Chat messages are not saved in history and will be permanently deleted when you leave the chat.");  // 隐私对话不会显示在历史记录中，离开对话界面时，其内容会被完全删除。
    translations["Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported."] = tr("Screenshot Q&A    Shortcut (Ctrl+Alt+Q), up to 3 images supported.");  // 截图问答 快捷键（Ctrl+Alt+Q），最多支持 3 张图
    translations["Cannot be used during screen recording"] = tr("Cannot be used during screen recording");  // 截图问答 不能在屏幕录制中使用
    translations["You can upload up to 3 files or image"] = tr("You can upload up to 3 files or image");  // 文件和图片的总数最多为3个
    translations["Please delete the abnormal file and send it again"] = tr("Please delete the abnormal file and send it again");  // 请删除异常文件再发送。
    translations["Add Private Chat"] = tr("Add Private Chat");  // 添加隐私对话
    translations["Add [Screenshot Q&A]"] = tr("Add [Screenshot Q&A]");  // 添加截图问答
    translations["Take a screenshot and send the content to UOS AI. You can also upload an image directly."] = tr("Take a screenshot and send the content to UOS AI. You can also upload an image directly.");
    translations["OK"] = tr("OK");
    translations["Next"] = tr("Next");
    translations["Add [ Private Chat Mode ] - Chats will not be saved."] = tr("Add [ Private Chat Mode ] - Chats will not be saved.");  // 添加隐私对话
    translations["No text extracted"] = tr("No text extracted");  // 未提取到文字
    translations["Image size exceeds 15 MB"] = tr("Image size exceeds 15 MB");  // 文件大小超过15 MB

    // 2.11需求
    translations["After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base."] = tr("After opening the knowledge base, answers will be based on its content. Response speed depends on machine performance and the size of the knowledge base.");  // 打开知识库后，会基于知识库回答问题。回答速度受机器性能和知识库数量影响。
    translations["Knowledge base unavailable when any command or MCP is selected."] = tr("Knowledge base unavailable when any command or MCP is selected.");  // 在选中任意指令或MCP时，知识库不可用。
    translations["MCP is disabled while the knowledge base is active."] = tr("MCP is disabled while the knowledge base is active.");  // 在使用知识库时，MCP功能不可用。
    translations["Commands disabled while knowledge base is active."] = tr("Commands disabled while knowledge base is active.");  // 在使用知识库时，指令功能不可用。
    translations["Copy succeeded."] = tr("Copy succeeded.");  // 复制成功。
    translations["Copy failed. Please try again."] = tr("Copy failed. Please try again.");  // 复制失败. 请重试。
    translations["Searching"] = tr("Searching");  // 搜索中
    translations["%1 reference documents have been obtained (%2s)"] = tr("%1 reference documents have been obtained (%2s)");  // 搜索到%1个参考文档（用时%2秒）
    translations["Clear History"] = tr("Clear History");  // 清除历史记录
    translations["Delete all records?"] = tr("Delete all records?");  // 是否要删除全部记录？
    translations["Once deleted, the content cannot be recovered!"] = tr("Once deleted, the content cannot be recovered!");  // 删除后，内容将无法恢复！
    translations["Recommend official models"] = tr("Recommend official models");
    translations["Disable MCP"] = tr("Disable MCP");

    // 2.12需求
    translations["It is recommended to use the official model \"DeepSeek-Trial Account\""] = tr("It is recommended to use the official model \"DeepSeek-Trial Account\"");  // 推荐使用官方模型"DeepSeek-试用账号"
    translations["Quick Open"] = tr("Quick Open");  // 快速唤起
    translations["MCP Server Upgrade to Automatic Mode"] = tr("MCP Server Upgrade to Automatic Mode");  // MCP服务升级自动模式
    translations["MCP Server have been upgraded to automatic mode, allowing you to access all MCP Server with just click "] = tr("MCP Server have been upgraded to automatic mode, allowing you to access all MCP Server with just click ");  //MCP服务升级自动模式，仅需打开
    translations[". This allows you to automate tasks like system setup and file processing with just one click."] = tr(". This allows you to automate tasks like system setup and file processing with just one click.");   //按钮即可使用所有MCP服务。您可以一句话完成系统设置、文件处理等自动化任务。
    translations["Adding MCP Server has been moved to Settings."] = tr("Adding MCP Server has been moved to Settings.");  // 添加MCP服务移至设置
    translations["To add more MCP Server, go to Settings > MCP Server."] = tr("To add more MCP Server, go to Settings > MCP Server.");  // 如需添加更多MCP服务，请到"设置-MCP服务"中添加。
    translations["Got it"] = tr("Got it");  // 知道了
    translations["Complimentary Model Credits"] = tr("Complimentary Model Credits");  //  【福利】模型额度赠送
    translations["The current system offers the DeepSeek trial account model, which automatically refreshes the free quota at the beginning of each month, allowing you to use it worry-free."] = tr("The current system offers the DeepSeek trial account model, which automatically refreshes the free quota at the beginning of each month, allowing you to use it worry-free.");  // 当前系统免费模型DeepSeek-试用账号，将为你在每月1日自动赠送200次文本对话额度，当月有效。
    translations["Claim Credits"] = tr("Claim Credits");  // 领取额度
    translations["Get a free account"] = tr("Get a free account");  // 领取免费账号
    translations["Claim Free Credits"] = tr("Claim Free Credits");  // 领取免费额度
    translations["Successfully Claimed"] = tr("Successfully Claimed");  // 领取成功
    translations["Failed to Claim. Please Try Again."] = tr("Failed to Claim. Please Try Again.");  // 领取失败，请再试一次
    translations["Enable MCP Server&"] = tr("Enable MCP Server&");  // 开启MCP服务
    translations["Disable MCP Server"] = tr("Disable MCP Server");  // 关闭MCP服务
    translations["Configure MCP Server"] = tr("Configure MCP Server");  // 配置MCP服务
    translations["Enabling MCP Server Features"] = tr("Enabling MCP Server Features");
    translations["Some third-party MCP server features carry certain risks. Please use them with caution. If you enable this service, a built-in tool will detect and automatically download necessary dependencies. This download process will incur data charges. Please be aware of these risks and proceed with caution."] = tr("Some third-party MCP server features carry certain risks. Please use them with caution. If you enable this service, a built-in tool will detect and automatically download necessary dependencies. This download process will incur data charges. Please be aware of these risks and proceed with caution.");
    translations["I have understood and agree to use this service"] = tr("I have understood and agree to use this service");

    translations["Light Theme"] = qApp->translate("TitleBarMenu", "Light Theme");
    translations["Dark Theme"] = qApp->translate("TitleBarMenu", "Dark Theme");
    translations["System Theme"] = qApp->translate("TitleBarMenu", "System Theme");
    translations["Theme"] = qApp->translate("TitleBarMenu", "Theme");
    return translations;
}

QString SystemChannel::getActiveColor() const
{
    return m_activeColor;
}

void SystemChannel::copyToClipboard(const QString &data, int type)
{
    if (static_cast<CopyDataType>(type) == CopyText) {
        qCDebug(logAIGUI) << "Copying text to clipboard";
        if (ESystemContext::isTreeland()) {
            QTimer::singleShot(10, this, [data]{
                QClipboard *clip = QApplication::clipboard();
                clip->setText(data);
            });
            return;
        }

        QClipboard *clip = QApplication::clipboard();
        clip->setText(data);
    } else if (static_cast<CopyDataType>(type) == CopyImage) {
        QString filePath = Util::imageData2TmpFile(m_ttpDir.path(), data);
        qCInfo(logAIGUI) << "Copying image to clipboard:" << filePath;

        QFileInfo imageInfo(filePath);

        if (imageInfo.exists()) {
            QImageReader imageReader(imageInfo.filePath());
            QImage inputImage = imageReader.read();

            if (!inputImage.isNull()) {
                QClipboard *clipboard = QGuiApplication::clipboard();
                clipboard->setImage(inputImage);
            } else {
                qCWarning(logAIGUI) << "ImageReader failed:"
                           << " path=" << imageInfo.filePath()
                           << " error=" << imageReader.errorString();
            }
        } else {
            qCWarning(logAIGUI) << "Image file not found:" << filePath;
        }
    }
}

void SystemChannel::copyReplyText(const QString &reply)
{
    copyToClipboard(reply, CopyText);
}

void SystemChannel::copyImage2Clipboard(const QString &imagePath)
{
    copyToClipboard(imagePath, CopyImage);
}

bool SystemChannel::isNetworkAvailable()
{
    return NetworkMonitor::getInstance().isOnline();
}

void SystemChannel::openFile(const QString &filePath)
{
    emit openFileComplete(filePath, Util::openFileFromPath(filePath));
}

void SystemChannel::openUrl(const QString &url)
{
    if (Util::launchUosBrowser(url))
        return;

    qCInfo(logAIGUI) << "Opening URL in default browser:" << url;
    Util::launchDefaultBrowser(url);
}

void SystemChannel::writeLog(int level, const QString &msg)
{
    switch (level)
    {
    case 0:
        qCDebug(logAIVUE) << msg;
        break;
    case 1:
        qCInfo(logAIVUE) << msg;
        break;
    case 2:
        qCWarning(logAIVUE) << msg;
        break;
    case 3:
        qCCritical(logAIVUE) << msg;
        break;
    default:
        break;
    }
}

bool SystemChannel::networkAvailable()
{
    return NetworkMonitor::getInstance().isOnline();
}

QString SystemChannel::fontInfo()
{
    QFontInfo info(qApp->font());
    return info.family() + "#" + QString::number(info.pixelSize());
}

int SystemChannel::themeColor()
{
    return m_themeColor;
}

void SystemChannel::switchThemeColor(int tc)
{
    DGuiApplicationHelper::ColorType type = DGuiApplicationHelper::UnknownType;

    if (tc == 1) {
        type = DGuiApplicationHelper::LightType;
    } else if (tc == 2) {
        type = DGuiApplicationHelper::DarkType;
    }

    DGuiApplicationHelper::instance()->setPaletteType(type);
}

int SystemChannel::themeColorOption()
{
    auto type = DGuiApplicationHelper::instance()->paletteType();
    if (type == DGuiApplicationHelper::LightType)
        return 1;
    else if (type == DGuiApplicationHelper::DarkType)
        return 2;

    return 0;
}

bool SystemChannel::isEnableAdvancedCssFeatures()
{
#ifdef COMPILE_ON_QT6
    return true;
#else
    return false;
#endif
}

void SystemChannel::updateVolume(int percent)
{
    qCDebug(logAIGUI) << "Updating volume to:" << percent;

    if (percent < 0 || percent > 100) {
        qCWarning(logAIGUI) << "Invalid volume value:" << percent;
        return;
    }

    // 调用UOSAbilityManager的音量调整方法
    QJsonObject argsObj;
    argsObj["volume"] = percent;
    OSCallContext ctx = UosAbility()->volumeAdjustment(argsObj);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to update volume:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Volume updated successfully to:" << percent;
    }
}

void SystemChannel::updateBrightness(int percent)
{
    qCDebug(logAIGUI) << "Updating brightness to:" << percent;

    if (percent < 0 || percent > 100) {
        qCWarning(logAIGUI) << "Invalid brightness value:" << percent;
        return;
    }

    // 调用UOSAbilityManager的亮度调整方法
    OSCallContext ctx = UosAbility()->doDiplayBrightness(percent, 0);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to update brightness:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Brightness updated successfully to:" << percent;
    }
}

void SystemChannel::openControlCenter(const QString &module)
{
    qCDebug(logAIGUI) << "Opening control center to module:" << module;

    // 直接调用UOSAbilityManager的通用接口
    OSCallContext ctx = UosAbility()->openControlCenter(module);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to open control center module:" << module << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Control center module opened successfully:" << module;
    }
}

void SystemChannel::updateFontSize(float size)
{
    qCDebug(logAIGUI) << "Updating font size to:" << size;

    if (size < 9.0 || size > 20.0) {
        qCWarning(logAIGUI) << "Invalid font size:" << size;
        return;
    }

    // 调用UOSAbilityManager的字号设置方法
    OSCallContext ctx = UosAbility()->doSystemFontSize(size);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to update font size:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Font size updated successfully to:" << size;
    }
}

void SystemChannel::toggleEyesProtection(bool enabled)
{
    qCDebug(logAIGUI) << "Toggling eyes protection to:" << enabled;

    // 调用UOSAbilityManager的护眼模式设置方法
    OSCallContext ctx = UosAbility()->doDiplayEyesProtection(enabled);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to toggle eyes protection:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Eyes protection toggled successfully to:" << enabled;
    }
}

void SystemChannel::doBluetoothConfig(bool enabled)
{
    qCDebug(logAIGUI) << "Toggling bluetooth to:" << enabled;

    // 调用UOSAbilityManager的蓝牙配置方法
    OSCallContext ctx = UosAbility()->doBluetoothConfig(enabled);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to toggle bluetooth:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Bluetooth toggled successfully to:" << enabled;
    }
}

void SystemChannel::doNoDisturb(bool enabled)
{
    qCDebug(logAIGUI) << "Toggling no disturb to:" << enabled;

    // 调用UOSAbilityManager的勿扰模式方法
    OSCallContext ctx = UosAbility()->doNoDisturb(enabled);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to toggle no disturb:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "No disturb toggled successfully to:" << enabled;
    }
}

void SystemChannel::switchWifi(bool enabled)
{
    qCDebug(logAIGUI) << "Toggling wifi to:" << enabled;

    // 调用UOSAbilityManager的WiFi切换方法
    OSCallContext ctx = UosAbility()->switchWifi(enabled);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to toggle wifi:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Wifi toggled successfully to:" << enabled;
    }
}
void SystemChannel::openAppStore(const QString &appPackage)
{
    qCDebug(logAIGUI) << "Opening app store for app:" << appPackage;

    // 调用UOSAbilityManager的应用商店方法
    // 使用doDownloadApp打开应用商店的特定应用安装页面
    OSCallContext ctx = UosAbility()->doDownloadApp(appPackage);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to open app store for app:" << appPackage << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "App store opened successfully for app:" << appPackage;
    }
}

void SystemChannel::openAppStoreTab(const QString &tabName)
{
    qCDebug(logAIGUI) << "Opening app store for tab:" << tabName;
    OSCallContext ctx = UosAbility()->doShowStoreTab(tabName);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to open app store for tab:" << tabName << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "App store opened successfully for tab:" << tabName;
    }
}

void SystemChannel::openCalendar(const QString &subject, const QString &startTime, const QString &endTime)
{
    Q_UNUSED(subject);
    Q_UNUSED(startTime);
    Q_UNUSED(endTime);

    qCDebug(logAIGUI) << "Opening calendar application";

    // 调用UOSAbilityManager启动日历应用
    OSCallContext ctx = UosAbility()->doAppLaunch("calendar", true);

    if (ctx.error != OSCallContext::NonError) {
        qCWarning(logAIGUI) << "Failed to open calendar:" << ctx.errorInfo;
    } else {
        qCDebug(logAIGUI) << "Calendar opened successfully";
    }
}

void SystemChannel::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    //Check if the theme's changed.
    if (m_themeColor != themeType) {
        m_themeColor = themeType;
        emit themeColorChanged(m_themeColor);
    }

    //Check if the active color is changed.
    QString activeColor = DGuiApplicationHelper::instance()->applicationPalette()
                          .color(DPalette::Normal, DPalette::Highlight)
                          .name(QColor::HexRgb);
    if (m_activeColor != activeColor) {
        m_activeColor = activeColor;
        emit activeColorChanged(m_activeColor);
    }
}

void SystemChannel::onFontChanged(const QFont &font)
{
    QFontInfo fontInfo(font);
    emit fontChanged(fontInfo.family() + "#" + QString::number(fontInfo.pixelSize()));
}


QString SystemChannel::getCurrentShortcut()
{
    return m_uosAiShortcut.accel;
}

QString SystemChannel::getCurrentTalkShortcut()
{
    searchShortCut();
    return m_uosAiTalkShortcut.accel;
}

QStringList SystemChannel::normalizeNotificationActions(const QVariantList &actions) const
{
    QStringList normalizedActions;
    QString pendingActionKey;

    for (const QVariant &actionVariant : actions) {
        const QVariantMap actionMap = actionVariant.toMap();
        if (!actionMap.isEmpty()) {
            const QString actionKey =
                actionMap.value(QStringLiteral("key")).toString().trimmed().isEmpty()
                    ? actionMap.value(QStringLiteral("id")).toString().trimmed()
                    : actionMap.value(QStringLiteral("key")).toString().trimmed();
            const QString actionText =
                actionMap.value(QStringLiteral("text")).toString().trimmed().isEmpty()
                    ? actionMap.value(QStringLiteral("label")).toString().trimmed()
                    : actionMap.value(QStringLiteral("text")).toString().trimmed();
            if (!actionKey.isEmpty()) {
                normalizedActions << actionKey << (actionText.isEmpty() ? actionKey : actionText);
            }
            continue;
        }

        const QVariantList actionPair = actionVariant.toList();
        if (actionPair.size() >= 2) {
            const QString actionKey = actionPair[0].toString().trimmed();
            const QString actionText = actionPair[1].toString().trimmed();
            if (!actionKey.isEmpty()) {
                normalizedActions << actionKey << (actionText.isEmpty() ? actionKey : actionText);
            }
            continue;
        }

        const QString actionTextOrKey = actionVariant.toString().trimmed();
        if (actionTextOrKey.isEmpty()) {
            continue;
        }
        if (pendingActionKey.isEmpty()) {
            pendingActionKey = actionTextOrKey;
        } else {
            normalizedActions << pendingActionKey << actionTextOrKey;
            pendingActionKey.clear();
        }
    }

    return normalizedActions;
}

uint SystemChannel::notify(const QString &title,
                           const QString &body,
                           const QString &icon,
                           const QVariantList &actions,
                           int timeoutMs)
{
    if (!m_notificationProxy || !m_notificationProxy->isValid()) {
        qCWarning(logAIGUI) << "Failed to send notification: invalid notification proxy";
        return 0;
    }

    QString appName = qApp ? qApp->applicationDisplayName() : QString();
    if (appName.isEmpty() && qApp) {
        appName = qApp->applicationName();
    }
    if (appName.isEmpty()) {
        appName = QStringLiteral("UOS AI");
    }

    const QStringList normalizedActions = normalizeNotificationActions(actions);
    QDBusReply<uint> reply = m_notificationProxy->call(
        QStringLiteral("Notify"),
        appName,
        0U,
        icon,
        title,
        body,
        normalizedActions,
        QVariantMap(),
        timeoutMs
    );
    if (!reply.isValid()) {
        qCWarning(logAIGUI) << "Failed to send notification:" << reply.error().message();
        return 0;
    }

    const uint notificationId = reply.value();
    if (notificationId > 0) {
        m_ownedNotificationIds.insert(notificationId);
    }
    return notificationId;
}

void SystemChannel::closeNotification(uint notificationId)
{
    if (notificationId == 0 || !m_notificationProxy || !m_notificationProxy->isValid()) {
        return;
    }
    m_notificationProxy->call(QStringLiteral("CloseNotification"), notificationId);
}

void SystemChannel::onNotificationActionInvoked(uint notificationId, const QString &actionKey)
{
    // if (!m_ownedNotificationIds.contains(notificationId)) {
    //     return;
    // }
    if (actionKey == QStringLiteral("remind_later")) {
        closeNotification(notificationId);
    }
    emit notificationActionInvoked(notificationId, actionKey);
}

void SystemChannel::onNotificationClosed(uint notificationId, uint reason)
{
    // if (!m_ownedNotificationIds.contains(notificationId)) {
    //     return;
    // }
    m_ownedNotificationIds.remove(notificationId);
    emit notificationClosed(notificationId, static_cast<int>(reason));
}

void SystemChannel::searchShortCut(){
    if (ESystemContext::isTreeland()) {
        qCDebug(logAIGUI) << "Skip shortcut registration in treeland environment";
        return;
    }

    qCInfo(logAIGUI) << "Initializing application shortcuts";
    ShortcutManager& shortcutMgr = ShortcutManager::getInstance();

    if (!shortcutMgr.isValid()) {
        qCWarning(logAIGUI) << "ShortcutManager D-Bus interface is not valid";
        return;
    }

    QString wordwizardName = tr("UOS AI FollowAlong/Write");

    // 查询现有快捷键
    QList<ShortcutInfo> shortcuts = shortcutMgr.searchShortcuts("UOS AI");
    ShortcutInfo uosAiShortcut;  // 主界面快捷键
    ShortcutInfo uosAiTalkShortcut;  // 数字人快捷键

    for (const ShortcutInfo &shortcut : shortcuts) {
        if (shortcut.id == "UOS AI") {
            uosAiShortcut = shortcut;
        }

        if (shortcut.id == "UOS AI Talk") {
            uosAiTalkShortcut = shortcut;
        }
    }
    m_uosAiShortcut = uosAiShortcut;
    m_uosAiTalkShortcut = uosAiTalkShortcut;
    return;
}
