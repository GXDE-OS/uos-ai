#ifndef EAIEXECUTOR_H
#define EAIEXECUTOR_H

#include "esingleton.h"
#include "eaicallbck.h"
#include "eaifaqinit.h"
#include "session.h"
#include "chatwindow.h"
#include "serverdefs.h"
#include "global.h"
#include "uossimplelog.h"
#include "eaiprompt.h"

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QMap>
#include <QVariant>
#include <QDBusServiceWatcher>
#include <QVector>
#include <QJsonObject>
#include <QLocale>
#include <QTemporaryDir>
#include <QQueue>
#include <QtConcurrent>
#include "uosfreeaccounts.h"
class EAiPrompt;
class EAiExecutor : public QObject
{
    Q_OBJECT

    SINGLETONIZE_CLASS(EAiExecutor)

    explicit EAiExecutor(QObject *parent = nullptr);

public:
    enum AiAction {
        None,
        Conversation = 1, //对话模式
        DocumentSummary, //文档总结
        MaxAiAction,
    };

    enum AiTaskType {
        OverrideDocument = 1,
        ParserDocument,
        OverrideQuestion,
    };

    enum ConversionMode{
        Normal = 0, //普通模式
        Private = 1, //私密模式
    };

    enum ConversionType{
        Text = 0, //文本模式
        TextFile = 1, //文本加文件
        TextImage = 2, //文本加图
        TextImageFile = 3, //文本图加文件
    };

    bool initAiSession();

    void sendRequest(const QString &llmId, uos_ai::ChatChunk &chatChunk, QObject *caller, const QString &notifier);
    QString sendWordWizardRequest(const QString &llmId, const QString &prompt, QObject *caller, const QString &notifier);

    Q_DECL_DEPRECATED QString sendAiRequst(const QString &llmId, QSharedPointer<EAiPrompt> prompt, QSharedPointer<EAiCallback> callback, bool isFAQGeneration = false);
    void clearAiRequest(const QObject *aiProxy);
    void cancelAiRequst(const QString &id);

    //Account manager interface
    QString currentLLMAccountId();
    QString uosAiLLMAccountId();
    QString queryLLMAccountList();
    QString queryUosAiLLMAccountList();
    bool setCurrentLLMAccountId(const QString &id);
    bool setUosAiLLMAccountId(const QString &id);

    QString currentAssistantId();
    QString queryAssistantList();
    bool setCurrentAssistantId(const QString &id);
    QString currentAssistantName();
    AssistantType currentAssistantType();

    //Config ai account
    void launchLLMConfigWindow(bool showAddllmPage = false, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString & locateTitle = "");
    void launchLLMConfigWindowAndGetFreeDialog();

    void launchKnowledgeBaseConfigWindow();

    void launchMcpConfigWindow();

    void launchAboutWindow();

    //Get Ai FAQ
    QString getRandomAiFAQ();
    QString getRandomAiFAQByFunction(int type, const QString &function);
    QString getAssistantFunctions(int type);
    QString getFunctionTemplate(int type, const QString &function, const QString &contain);

    //Set ChatWindow
    void setChatWindow(ChatWindow *window);
    void showToast(const QString &messge);

    void getHistoryFromChatInfo();

    /**
     * @brief 是否需要从零启动（需要等待前端渲染）
     * @return true: 从零启动; false: 只需激活窗口
     */
    bool showChatWindow();

    /**
     * @brief 给快捷面板使用，判断主窗口是不是激活状态
     * @return
     */
    bool chatWindowActive();
    //Close chat window
    void closeChatWindow();

    //Record api
    bool startRecorder(int mode);
    bool stopRecorder();
    bool isRecording();
    bool isAudioInputAvailable();
    bool isAudioOutputAvailable();

    //TTS api
    bool playTextAudio(const QString &id, const QString &text, bool isEnd, bool isPlayOutline);
    bool stopPlayTextAudio();

    //Sound effect
    void playSystemSound(int effId);
    //Chat history apis
    Q_DECL_DEPRECATED void logChatRecord(const QString &reqId,
                       const QString &question,
                       const QStringList &answer,
                       bool isRetry,
                       int err, const QString &errorInfo,
                       int actionType,
                       const QString &llmIcon,
                       const QString &llmName,
                       const QString &docSummaryParam);
    QString getChatRecords(bool lastRec);
    void clearChatRecords();

    void logConversations(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QVector<uos_ai::Conversations> &convs);
    QString getConversations();
    QString createNewConversation();
    void removeConversation(const QString &assistantId, const QString &conversationId);
    void removeAllConversation();

    QString getConversationHistoryList();
    bool setCurrentConversationId(const QString &assistantId, const QString &conversationId);
    QString getLastConversation(const QString &assistantId);

    //Image operations
    QString saveImageAs(const QString &imageData, bool saveAs);
    bool previewImage(const QString &imageData);
    bool previewImageForPath(const QString &imagePath);
    void copyImg2Clipboard(const QString &imageData);

    //Network state check
    bool isNetworkAvailable();
    bool isKnowledgeBaseExist() { return m_knowledgeBaseExist; }
    bool isEmbeddingPluginsExist() { return m_embeddingPluginsExist; }

    void personalFAQGenerate();
    void openInstallWidget(const QString &appname);

    void documentSummarySelect();
    void documentSummaryForOfficeSelect(int category);
    QString processClipboardData();
    void onDocSummaryDragInView(const QStringList &docPaths, const QString &defaultPrompt);
    void documentSummaryParsing(const QString &id, const QString &docPath);
    void openFile(const QString &filePath);
    void openUrl(const QString &url);

    void wordWizardContinueChat(const uos_ai::Conversations &conv, int type);
    void wordWizardAskAI(const QString &question, int type);
    void appendWordWizardConv(int type);
    void appendWordWizardQuestion(int type);
    void webViewLoadFinished();

    void previewRefDoc(const QString &docPath, const QStringList &docContents);

    QString getInstList();

    void rateAnwser(const int questionIdx, const int answerIdx, int rate, const QString &extJson);

    void setTitleBarStatus(bool status);

    bool showWarningDialog(const QString assistantId , const QString conversationId, const QString msg, bool isDelete, bool isLlmDelete, bool isAllConvDelete);
    bool showRmMcpServerDlg(const QString &name);
    void showUpdateDialog(const QString &msg, const QString &appName);
    bool showRemoveFileDialog(const QString &message);
    bool showAllowUploadFilesAlert(int modelType, const QString &modelDisplayName, bool searchOnline);

    void showPromptWindow();
    void changeFreeAccountGuide(bool isShowFreeAccountGuide, bool isPreShow = false);

    void updateUpdatePromptDB(bool isClicked);
    void updateUpdateFreeAccountGuideDB(bool isClicked);

    // Agent
    QStringList getMcpServers(const QString &agentName);
    bool getThirdPartyMcpAgreement();
    bool isInstallUosAiAgent(const QString &agentName);// mcp环境检查

    // Screenshot
    void startScreenshot();
    void setInputFileSize(int inputFileSize);

    //Private chat
    void setConversationMode(int mode);

    void showKnowledgeBaseErrorDialog(int type);

    bool showLostFileWarningDlg(const QString lostFileList);

    // mcp环境缺失弹窗
    bool showInstallUosAIAgentDlg();

    bool getFreeCredits(bool isShowDlg);  // 领取免费额度

    bool getIsShowFreeAccountGuide() {
        return m_isShwoFreeAccountGuide;
    }

    // 查询当前快捷键设置
    QString getCurrentShortcut();

    // 当前是否为简体中文
    bool isSimplifiedChinese();
    void showGetFreeCreditsResultDlg(bool isSuccess);

    // 大纲删除确认框
    bool isDeleteOutlineTitle();

    // 获取下载列表图标
    QString getDownloadListIcon(QString fileSuffix);
    bool downloadFile(const QString &id, const QString &title, const QString &content, const QString &suffix);

    // 打印文档
    void printDocument(const QString &html, const QString &title);

    // 更新当前应答侧index
    void updateAnswresActiveIndex(int activeIndex);

    // 禁止切换数字形象
    void setDigitalImageDisable(bool disable);

    // 是否从数字形象强制切换为聊天
    bool isActiveChatFromDigitalImage();

private:
    void startScreenshotInternal();

signals:
    void llmAccountLstChanged(const QString &currentAccountId,
                              const QString &accountLst);
    void uosAiLlmAccountLstChanged();
    //ASR signal
    void audioASRInComing(const QString &text, bool isEnd);
    void audioASRError(int err, const QString &errorInfo);
    void audioInputDeviceChange(bool valid);
    void audioOutputDeviceChanged(bool valid);
    void audioSampleLevel(int level);
    //TTS signal
    void playTTSFinished(const QString &id);
    void playTTSError(int code, const QString &errorString);

    //TTP(Text to picture)
    void textToPictureFinish(const QString &id, const QStringList &paths);

    //Ai type api
    void chatConversationType(const QString &id, int action);

    //Network state changed
    void netStateChanged(bool isOnline);

    void knowledgeBaseExistChanged( bool exist);
    void knowledgeBaseFAQGenFinished();
    void localLLMStatusChanged(bool isExist);
    void embeddingPluginsStatusChanged(bool isExist);

    void sigAiReplyNone(int act, QString reply, int err);

    void docSummaryParsingStart(const QString &docPath, const QString &iconData, const QString &defaultPrompt, int error);
    void docSummaryForOffice(const QString filesDataJson, int error, int category);
    void docDragInViewParserResult(const QString &id, int error, const QString &docPath, const QString &docContent);
    void openFileFromPathResult(bool ok);

    void sigAppendWordWizardConv(int type);
    void sigWebViewLoadFinished();

    void previewReference(const QString &reference);

    void pptCreateSuccess(const QString &id, const QStringList &paths);
    void pptChangeSuccess(const QString &id, const QStringList &paths);

    void posterCreateSuccess(const QStringList &id, const QStringList &paths);

    void sigOverrideQues(const QString &question);

    void sigAssistantListChanged();

    void sigMainContentBackgroundColor(QString color);

    void sigGetNewHistoryList(QString currentConversationId);

    void sigHideHistoryList();

    void sigToShowPromptWindow(int titleBarBtnWidth);

    void sigToChangeFreeAccountGuide(bool isShowFreeAccountGuide, bool isPreShow);

    void sigInputFocus();

    void sigStartScreenshot();

    void sigShowTip(const QString &msg);

    void sigWordWizardAsk(const QString &askQuestion);

    void sigClaimUsageResult(bool ret, const QString &msg);
    void sigClaimAgain(bool claimAgain);

    void sigIconThemeChanged();

    void sigDownloadFileFinished(const QString &id, bool result);

    void sigActiveChatFromDigitalImage();
public slots:
    void onPPTCreated(const QString &id, const QList<QByteArray> &imageData);
    void onPPTChanged(const QString &id, const QList<QByteArray> &imageData);
    void onPosterCreated(const QList<QString> &idList, const QList<QByteArray> &imageData);

protected slots:
    void onCommandsReceived(const QVariantMap &commands);
    void onChatTextReceived(const QString &callId, const QString &chatText);
    void onChatError(const QString &id, int code, const QString &errorString);
    void onTextToPictureData(const QString &id, const QList<QByteArray> &imageData);

    void onAddToServerStatusChanged(const QStringList &files, int status);
    void onIndexDeleted(const QStringList &files);
    void onLocalLLMStatusChanged(bool isExist);
    void onEmbeddingPluginsStatusChanged(bool isExist);

    void onScreenshotCustomDone(const QString &imagePath);
    void onScreenshotCallFinished(QDBusPendingCallWatcher *watcher);

    void onAccountUsageClaimed(bool ret, const QString &msg);

protected:
    //Private methods
    void loadAiFAQ();

    //Init audio record
    void initAudioRecord();

    //Init network monitor
    void initNetworkMonitor();

protected:
    QSharedPointer<Session> m_aiProxy;

    //Ref to the chat window
    ChatWindow *m_chatWindow { nullptr};

    using AiCallQueue = QMap<QString, QSharedPointer<EAiCallback>>;
    AiCallQueue m_callQueue;

    const int   m_limitAQCount {6};
    EAiFAQInit m_faqInitTool;
    QVector<QJsonObject> m_aiFAQ;
    QVector<QJsonObject> m_assistantFAQ;
    QVector<QJsonObject> m_persKnowledgeBaseFAQ;
    QVector<QJsonObject> m_aiWritingFAQ;
    QVector<QJsonObject> m_aiTextProcessingFAQ;
    QVector<QJsonObject> m_aiTranslationFAQ;
    QString m_faqId = "";   //生成问题的ID

    QLocale m_sysLang;

    //Ai Chat history set
    struct AiConversation {
        int errCode;
        QString errInfo;
        QString reqId;
        QStringList answer;
        //LLM Info
        QString llmIcon;
        QString llmName;
    };

    struct AiChatRecord {
        QString question;
        QString questHash;
        //Conversation type
        //Ref:serverdefs.h/ChatAction
        ChatAction actType;
        QString douSummaryParam;

        QVector<AiConversation> replys;
    };

    QMap<QString, QJsonDocument> m_convsHistory;
    QVector<uos_ai::Conversations> m_currnetConvs;
    QVector<uos_ai::Conversations> m_currnetPrivateConvs;
    int m_answerActiveIndex; // 当前应答侧展示的第几次回答

    friend QDebug &operator<<(QDebug &debug, const AiChatRecord &r);

    static bool makeJsonFromChatRecord(const AiChatRecord &rec /*in*/, QJsonArray &historyArray /*out*/);

    QMap<QString, QVector<AiChatRecord>> m_assistantChatHistories;

    //Temporary directory to save TTP
    QTemporaryDir m_ttpDir;

    bool m_isAnswering = false;
    bool m_knowledgeBaseExist = false;
    bool m_embeddingPluginsExist = false;
    bool m_isFAQGenerating = false;

    // 划词助手继续对话缓存，可能会有多个继续对话
    QQueue<uos_ai::Conversations> m_wordWizardConvs;
    // 划词助手问一问缓存，可能会有多个问题
    QQueue<QString> m_wordWizardQuestions;

    ConversionMode m_conversionMode = ConversionMode::Normal;

    int m_inputFileSize = 0;

    UosFreeAccountActivity m_hasActivity;
    UosFreeModelActivity m_hasModelActivity;
    bool m_isShwoFreeAccountGuide = false;  // 是否显示免费额度领取引导界面
private:
    //Make ai request context
    QString makeAiReqContext(QSharedPointer<EAiPrompt> prompt, bool isFAQGeneration = false);
    QString makeContextReq(QSharedPointer<EAiPrompt> prompt, bool isRetry);
    QString makeReq(const QString &prompt);
    void processAiRecord(const AiChatRecord &rec /*in*/, QJsonArray &ctxArray/*out*/);
    void processConversations(const uos_ai::Conversations &convs /*in*/, QJsonArray &ctxArray/*out*/);

    QString requestChatText(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback, EAiPrompt::RequstType type); // general
    QString requestFunction(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback, const QJsonArray &funcs); // functionCall
    QString chatRequest(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback);  // text plain
    QString requestGenImage(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback);
    QString searchRequest(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback);
    QString requestMcpAgent(const QString &llmId, const QString &aiCtx, const QVariantHash &params, QSharedPointer<EAiCallback> callback);
    
    QString wordWizardRequest(const QString &llmId, const QString &aiCtx, QSharedPointer<EAiCallback> callback);

    QSharedPointer<EAiPrompt> initPrompt(const uos_ai::ChatChunk &chatChunk);
    QSharedPointer<EAiPrompt> initDefaultPrompt(const QString &userQuestion, const QJsonArray &extetion);

    QSharedPointer<EAiCallback> initCallback(QObject *caller, const QString &notifier, QSharedPointer<EAiPrompt> prompt);
    QSharedPointer<EAiCallback> initDefaultCallback(QObject *caller, const QString &notifier);

    QVariantHash initAiWritingParams(const uos_ai::ChatChunk &chatChunk);

    void parseReceivedFaq(const QString &faqContent);
    void updateFaqList(const QStringList &faqList, const QString &lang, const QString &currentLang);

    bool isKnowAssistantType();

    QString appendDocContent(const QJsonArray &exts, const QString &userData);

    QString processRequest(QSharedPointer<EAiPrompt> prompt, QSharedPointer<EAiCallback> callback, const QString &llmId, const uos_ai::ChatChunk &chatChunk);

    bool isAppendConv(const uos_ai::Conversations &convs);

    void controlAssisConvsLog(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QVector<uos_ai::Conversations> &convs);
    void createPPTHistory(const QString &id, const QString &displayContent);
    void createPosterHistory(const QString &idList, const QString &displayContent);

    void writeAssistantChatTypeEvent(ConversionType type);

    void JudgeIsShowFreeAccountGuide();
};

#define EAiExec() (ESingleton<EAiExecutor>::getInstance())
#endif // EAIEXECUTOR_H
