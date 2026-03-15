#ifndef EAIPROXY_H
#define EAIPROXY_H

#include "eaicallbck.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QScopedPointer>
#include <util.h>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

#define DEF_ACT_NOTIFIER(actname) \
    void sig##actname(int act, QString reply, int err);\
    void sig##actname##Stream(int act, QString reply, int err)

#define GET_NOTIFIER_NAME(actname) ("sig"#actname)
#define GET_SNOTIFIER_NAME(actname) ("sig"#actname"Stream")

class EAiProxy : public QObject
{
    Q_OBJECT
    /**
     *  enum ColorType {
     *   UnknownType,
     *   LightType = 1,
     *   DarkType  = 2
     *  };
     */
    Q_PROPERTY(int themeType READ getThemeType NOTIFY sigThemeChanged)
    Q_PROPERTY(QString activeColor READ getActiveColor NOTIFY sigActiveColorChanged)
public:
    explicit EAiProxy(QObject *parent = nullptr);

    enum AiAction {
        None,
        Conversation = 1, //对话模式
        DocumentSummary, //文档总结
        MaxAiAction,
    };

    enum AiToast {
        //Keep the index sync with it add order.
        clearHistory = 1,
        copyDone
    };

    using Mode = EAiCallback::DataMode;

    QString getNotifierName(AiAction act, int mode);

    void initAsyncWorkerConn();

signals:
    /**
     *sigAiReply
     *sigAiReplyStream(int act, QString reply, int err)
     *  int act AiAction,
     *  QString reply Ai reply or error message if err isn't 0,
     *  int err 0 for normal, 200 for stream end, other for error
     */
    DEF_ACT_NOTIFIER(AiReply);

    //Audio common singals
    void sigAudioInputDevChange(bool valid);
    void sigAudioOutputDevChanged(bool valid);
    void sigAudioSampleLevel(int level);
    //Audio ASR signals
    void sigAudioASRStream(const QString &text, bool isEnd);
    void sigAudioASRError(int err, const QString &text);
    void sigAudioCountDown(int sec);

    //TTS signals
    void sigPlayTTSFinished(const QString &id);
    void sigPlayTTSError(int code, const QString &errorString);

    //TTP
    void sigText2PicFinish(const QString &id, const QStringList &paths);

    //Ai Conversation type
    void sigChatConversationType(const QString &id, int action);

    //Account change signal
    void llmAccountLstChanged(const QString &currentAccountId,
                              const QString &accountLst);
    void sigThemeChanged(int value);
    void sigActiveColorChanged(QString color);
    void sigNetStateChanged(bool isOnline);
    void sigFontChanged(const QString &fontFamily, int pixelSize);
    void sigKnowledgeBaseStatusChanged( bool exist);
    void sigKnowledgeBaseFAQGenFinished();

    void sigLocalLLMStatusChanged(bool exist);
    void sigEmbeddingPluginsStatusChanged(bool exist);

    void sigDocSummaryParsingStart(const QString &docPath, const QString &iconData, const QString &defaultPrompt, int error);
    void sigDocSummaryForOffice(const QString filesDataJson, int error, int category);
    void sigDocSummaryParserResult(const QString &id, int error, const QString &docPath, const QString &docContent);
    void sigOpenFileFromPathResult(bool ok);

    void sigOCRResult(int error, const QString &OCRContent);

    void sigAppendWordWizardConv(int type);

    void sigPreviewReference(const QString &reference);
    void sigPPTCreateSuccess(const QString &id, const QStringList &paths);
    void sigPPTChangeSuccess(const QString &id, const QStringList &paths);
    void sigPosterCreateSuccess(const QStringList &id, const QStringList &paths);

    void sigOverrideQues(const QString &question);

    void sigAssistantListChanged();

    void sigAsyncWorker(int type);

    void sigMainContentBackgroundColor(QString color);

    void sigGetNewHistoryList(QString currentConversationId);

    void sigHideHistoryList();

    void sigToShowPromptWindow(int titleBarBtnWidth);

    void sigToChangeFreeAccountGuide(bool isShowFreeAccountGuide, bool isPreShow);

    void sigInputFocus();

    void sigShowTip(const QString &msg);

    void sigWordWizardAsk(const QString &askQuestion);

    void sigIsGotFreeCredits(bool isGotFreeCredits);
    void sigGetFreeCreditsResult(bool success, const QString &msg);  // 免费额度获取结果

    void sigIconThemeChanged();

    void sigDownloadFileFinished(const QString &id, bool result);

    // System event signal: triggered when shutdown/restart/lock/quit occurs
    void sigSystemEvent(const QString &eventType);

    void sigActiveChatFromDigitalImage();

public slots:
    //Js Call this method to notify C++
    //error
    /**
     * @brief sendAiRequest
     * @param llmId LLM id
     * @param llmType LLM type
     * @param act Operaion type
     * @param param chat history.
     *   [
     *     {"role": "user", "content": "你好"},
     *     {"role": "assistant", "content": "我是人工智能助手"},
     *     {"role": "user", "content": "你叫什么名字"},
     *     {"role": "assistant", "content": "我叫chatGLM"},
     *     {"role":"user", "content":"你都可以做些什么事"},
     *   ]
     * @param mode
     *      Mode::CacheMode Return complete result in notifier
     *      Mode::StreamMode Return char squence by stream notifier
     */
    Q_DECL_DEPRECATED QString sendAiRequest(const QString &llmId,
                          int llmType,
                          int act,
                          const QString &param,
                          int mode);

    QString sendRequest(const QString &llmId, const QString &chatChunkData);
    /**
     * @brief cancelAiRequest
     * @param id Call id return from sendAiRequest
     */
    void cancelAiRequest(const QString &id);
    //Account manager interface
    /**
     * @brief currentLLMAccountId
     * @return Account String id
     */
    QString currentLLMAccountId();
    /**
     * @brief queryLLMAccountList
     * @return Accounts Json info
     * '[{"id":"xxx-xxx-xxx","name":"xxx","type":0}]
     */
    QString queryLLMAccountList();
    /**
     * @brief setCurrentLLMAccountId
     * @param id account id
     * @return Return true if ok, else false
     */
    bool setCurrentLLMAccountId(const QString &id);

    QString currentAssistantId();
    QString queryAssistantList();
    bool setCurrentAssistantId(const QString &id);

    /**
     * @brief getAiFAQ
     * @return Return limited count ai FAQ
     */
    QString getAiFAQ();
    /**
     * @brief getAiFAQByFunction
     * @return Return limited count ai FAQ by function
     */
    QString getAiFAQByFunction(int type, const QString &function);
    /**
     * @brief getAssistantFunctions
     * @return Return functions by assistantType
     */
    QString getAssistantFunctions(int type);
    /**
     * @brief getFunctionTemplate
     * @return Return template by function and contain
     */
    QString getFunctionTemplate(int type, const QString &function, const QString &contain);
    /**
     * @brief launchLLMConfigWindow Show ai config window
     * @param showAddllmPage Show account add dialog if true
     */
    void launchLLMConfigWindow(bool showAddllmPage = false);

    /**
     * @brief launchKnowledgeBaseConfigWindow Show ai config window and locate the knowledgeBase
     */
    void launchKnowledgeBaseConfigWindow();

    void launchMcpConfigWindow();

    /**
     * @brief showToast
     * @param type AiToast
     *
     */
    void showToast(int type);
    /**
     * @brief closeChatWindow
     */
    void closeChatWindow();

    /**
     * @brief isAudioInputAvailable
     * @return Return true if any input device available
     */
    bool isAudioInputAvailable();
    /**
     * @brief isAudioOutputAvailable
     * @return Return true if any output device available
     */
    bool isAudioOutputAvailable();
    /**
     * @brief startRecorder
     *     Start audio record
     * @param 0:chat  1:digital
     */
    bool startRecorder(int mode);
    /**
     * @brief stopRecorder
     *
     *     Stop audio record
     */
    bool stopRecorder();
    /**
     * @brief isRecording
     * @return Return true if doing audio record
     */
    //bool isRecording();
    /**
     * @brief playTextAudio play TTS
     * @param id Request id
     * @param text  Text to make speech
     * @param isEnd is all the text transfered.
     *      true if the end.
     */
    bool playTextAudio(const QString &id, const QString &text, bool isEnd, bool isPlayOutline);
    /**
     * @brief stopPlayTextAudio
     *      Stop to play TTS.
     */
    bool stopPlayTextAudio();
    /**
     * @brief playSystemSound
     * @param effId sound effect id
     *     Ref:src/audio/audiocontroler.h
     *     enum AudioSystemEffect {
     *      Active = 0,
     *      Sleep  = 1
     *   }
     */
    void playSystemSound(int effId);
    /**
     * @brief copyReplyText
     *      Copy the text of the reply.
     * @param reply text
     */
    void copyReplyText(const QString &reply);

    //Chat history apis
    /**
    * @brief logAiChatRecord
    * @param reqId Request Id
    * @param question Question asked to Ai
    * @param anwsers  Ai reply answer
    * @param isRetry  true for retry conversation
    * @param err Error code
    * @param errorInfo Error string message
    * @param actionType
    * @param llmIcon LLM account icon
    * @param llmName LLM account name
    *   Ref:src/serverdefs.h
    *       enum ChatAction {
    *           ChatTextPlain     = 0,      // 纯文本聊天
    *           ChatFunctionCall  = 1,      // FunctionCall
    *           ChatText2Image    = 2       // 文生图
    *       };
    */
    Q_DECL_DEPRECATED void logAiChatRecord(const QString &reqId,
                         const QString &question,
                         const QString &anwser,
                         bool isRetry,
                         int err, const QString &errorInfo,
                         int actionType,
                         const QString &llmIcon,
                         const QString &llmName,
                         const QString &docSummaryParam);

    Q_DECL_DEPRECATED void logAiChatRecord(const QString &reqId,
                         const QString &question,
                         const QStringList &anwser,
                         bool isRetry,
                         int err, const QString &errorInfo,
                         int actionType,
                         const QString &llmIcon,
                         const QString &llmName,
                         const QString &docSummaryParam);

    /**
     * @brief getAiRecord
     * @param lastRec
     *      true for get last record
     *      false for get full record
     * @return
     */
    QString getAiChatRecords(bool lastRec);
    /**
     * @brief clearAiChatRecord
     */
    void clearAiChatRecords();

    void logCurrentConversations(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, const QString &conversationsData);
    QString getConversations();
    QString createNewConversation();
    void removeConversation(const QString &assistantId, const QString &conversationId);
    void removeAllConversation();

    QString getConversationHistoryList();
    bool setCurrentConversationId(const QString &assistantId, const QString &conversationId);
    QString getLastConversation(const QString &assistantId);
    /**
     * @brief saveImageAsFile
     * @param filePath
     * @return
     */
    QString saveImageAs(const QString &imageData, bool saveAs = true);
    /**
     * @brief previewImage
     * @param filePath
     * @return
     */
    bool previewImage(const QString &imageData);
    bool previewImageForPath(const QString &imagePath);
    /**
     * @brief copyImage2Clipboard
     * @param filePath
     */
    void copyImage2Clipboard(const QString &filePath);
    /**
     * @brief isNetworkAvailable
     * @return Return true if network available.
     */
    bool isNetworkAvailable();

    /**
     * @brief loadTranslations
     * @return
     */
    QJsonObject loadTranslations();

    void launchAboutWindow();

    void setFontInfo(const QString &fontFamily, int pixelSize);
    QString fontInfo();

    void setWindowMode(bool isWindowMode);
    bool isWindowMode();

    void onUpdateSystemFont(const QFont &);

    bool isKnowledgeBaseExist();
    bool isEmbeddingPluginsExist();

    void configureKnowledgeBase();
    void installEmbeddingPlugins();

    void onDocSummarySelect();
    void onDocSummaryForOfficeSelect(int category);
    QString processClipboardData();
    void onDocSummaryParsing(const QString &id, const QString &docPath);
    void openFile(const QString &filePath);
    void openUrl(const QString &url);

    void appendWordWizardConv(int type);
    void appendWordWizardQuestion(int type);
    void webViewLoadFinished();

    void previewRefDoc(const QString &docPath, const QStringList &docContents);
    int getThemeType() { return m_themeType;}

    // Instruction
    QString getInstList();

    void sendPPTOutline(const QString &content);
    void editPPT(const QString &content);
    void downloadPPT(const QString &id);

    void genePoster(const QString &content);
    void editPoster(const QString &content);
    void downloadPoster(const QString &id);

    bool isAgentSupported();
    void openAppstore(const QString &id);

    void onAsyncWorkerFinished(int type);

    void rateAnwser(const int questionIdx, const int answerIdx, int rate, const QString &extJson);

    QString getMainContentbackgroundColor(){return m_backgroundColor;}

    bool isChineseLanguage() {return uos_ai::Util::checkLanguage();}

    void setTitleBarStatus(bool status);

    bool showWarningDialog(const QString assistantId, const QString conversationId, const QString msg, bool isDelete, bool isLlmDelete, bool isAllConvDelete);
    bool showRemoveFileDialog(const QString &message);
    bool showAllowUploadFilesAlert(int modelType, const QString &modelDisplayName, bool searchOnline);

    void updateUpdatePromptDB(bool isClicked);
    void updateUpdateFreeAccountGuideDB(bool isClicked);
    // 0:Debug 1:Info 2:Warning 3:Critical
    void writeVueLog(int level, const QString &msg);

    // Agent
    void installUosAiAgent();
    bool isEnableMcp();
    bool getThirdPartyMcpAgreement();
    bool isInstallUosAiAgent(const QString &agentName);// mcp环境检查

    // Screenshot
    void startScreenshot();
    int isEnableScreenshot();
    void setInputFileSize(int inputFileSize);

    //Private chat
    void setConversationMode(int mode);

    // 判断文件是否存在
    bool isFileExist(QString filePath);

    // 是否支持知识库
    bool isEnableKnowledgebase();

    // 知识库异常弹窗
    void showKnowledgeBaseErrorDialog(const int type);   // 0:插件异常 1:知识库异常

    // 重新编辑用户输入，文件重新走文档解析流程
    void editQuestionToFileSummary(const QStringList fileLists);
    bool showLostFileWarningDlg(const QString lostFileList);

    // mcp环境缺失弹窗
    bool showInstallUosAIAgentDlg();

    void mcpDataUpload(const QString &mcpToUse);

    void launchLLMConfigWindowAndGetFreeDialog();
    bool getFreeCredits(bool isShowDlg);  // 领取免费额度

    // 查询当前快捷键设置
    QString getCurrentShortcut();

    // 是否启用高级CSS特性（如backdrop-filter等）
    bool isEnableAdvancedCssFeatures();

    // 当前是否为简体中文
    bool isSimplifiedChinese();

    // 大纲删除确认框
    bool isDeleteOutlineTitle();

    // 或者下载列表图标
    QString getDownloadListIcon(const QString &fileSuffix);

    // 下载文件
    bool downloadFile(const QString &id, const QString &title, const QString &content, const QString &suffix);

    // 打印文档
    void printDocument(const QString &html, const QString &title);

    // 更新当前应答侧index
    void updateAnswresActiveIndex(int activeIndex);

    /**
     * @brief 前端确认数据已保存
     * 当系统准备关机/重启/休眠时，前端完成数据保存后调用此方法
     * 告知系统可以继续关机流程
     */
    Q_INVOKABLE void confirmDataSaved();

    // 禁止切换数字形象
    void setDigitalImageDisable(bool disable);

    // 是否从数字形象强制切换为聊天
    bool isActiveChatFromDigitalImage();

protected:
    //Property Get methods
//    int getThemeType() { return m_themeType;}
    QString getActiveColor() { return m_activeColor;}

protected:
    int m_themeType {DGuiApplicationHelper::ColorType::LightType};

    //Current activeColor
    QString m_activeColor {""};

    QVector<QString> m_aiToastMessage;

    enum {
        AUDIO_LENGTH = 60,
    };

    int m_audioLenLimit {AUDIO_LENGTH}; //60s
    QScopedPointer<QTimer> m_audioRecCounter;

    QString m_fontFamily = "Source Han Sans SC";
    int m_fontPixelSize = 14;

    bool m_isWindowMode = true;

    QString m_backgroundColor;
};

#endif // EAIPROXY_H
