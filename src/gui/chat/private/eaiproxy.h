#ifndef EAIPROXY_H
#define EAIPROXY_H

#include "eaicallbck.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QScopedPointer>

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
        MaxAiAction,
    };

    enum AiToast {
        //Keep the index sync with it add order.
        clearHistory = 1,
        copyDone
    };

    using Mode = EAiCallback::DataMode;

    QString getNotifierName(AiAction act, int mode);

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
    void sigAssistantChanged(int id);
    void sigKnowledgeBaseStatusChanged( bool exist);
    void sigKnowledgeBaseFAQGenFinished();

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
    QString sendAiRequest(const QString &llmId,
                          int llmType,
                          int act,
                          const QString &param,
                          int mode);
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
     * @brief launchLLMConfigWindow Show ai config window
     * @param showAddllmPage Show account add dialog if true
     */
    void launchLLMConfigWindow(bool showAddllmPage = false);
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
    bool playTextAudio(const QString &id, const QString &text, bool isEnd);
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
    void logAiChatRecord(const QString &reqId,
                         const QString &question,
                         const QString &anwser,
                         bool isRetry,
                         int err, const QString &errorInfo,
                         int actionType,
                         const QString &llmIcon,
                         const QString &llmName);

    void logAiChatRecord(const QString &reqId,
                         const QString &question,
                         const QStringList &anwser,
                         bool isRetry,
                         int err, const QString &errorInfo,
                         int actionType,
                         const QString &llmIcon,
                         const QString &llmName);
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
    /**
     * @brief saveImageAsFile
     * @param filePath
     * @return
     */
    bool saveImageAs(const QString &filePath);
    /**
     * @brief previewImage
     * @param filePath
     * @return
     */
    bool previewImage(const QString &filePath);
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

    void configureKnowledgeBase();

protected:
    //Property Get methods
    int getThemeType() { return m_themeType;}
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

    bool m_isWindowMode = false;
};

#endif // EAIPROXY_H
