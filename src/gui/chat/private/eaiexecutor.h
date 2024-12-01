#ifndef EAIEXECUTOR_H
#define EAIEXECUTOR_H

#include "esingleton.h"
#include "eaicallbck.h"
#include "session.h"
#include "chatwindow.h"
#include "serverdefs.h"

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QMap>
#include <QVariant>
#include <QDBusServiceWatcher>
#include <QVector>
#include <QJsonObject>
#include <QLocale>
#include <QTemporaryDir>

class EAiExecutor : public QObject
{
    Q_OBJECT

    SINGLETONIZE_CLASS(EAiExecutor)

    explicit EAiExecutor(QObject *parent = nullptr);

public:
    bool initAiSession();

    QString sendAiRequst(const QString &llmId, const QString &prompt, QSharedPointer<EAiCallback> callback, bool isFAQGeneration = false);
    void clearAiRequest(const QObject *aiProxy);
    void cancelAiRequst(const QString &id);

    //Account manager interface
    QString currentLLMAccountId();
    QString queryLLMAccountList();
    bool setCurrentLLMAccountId(const QString &id);

    QString currentAssistantId();
    QString queryAssistantList();
    bool setCurrentAssistantId(const QString &id);
    QString currentAssistantName();
    AssistantType currentAssistantType();

    //Config ai account
    void launchLLMConfigWindow(bool showAddllmPage = false, bool onlyUseAgreement = false);

    void launchAboutWindow();

    //Get Ai FAQ
    QString getRandomAiFAQ();

    //Set ChatWindow
    void setChatWindow(ChatWindow *window);
    void showToast(const QString &messge);

    //Close chat window
    void closeChatWindow();

    //Record api
    bool startRecorder(int mode);
    bool stopRecorder();
    bool isRecording();
    bool isAudioInputAvailable();
    bool isAudioOutputAvailable();

    //TTS api
    bool playTextAudio(const QString &id, const QString &text, bool isEnd);
    bool stopPlayTextAudio();

    //Sound effect
    void playSystemSound(int effId);
    //Chat history apis
    void logChatRecord(const QString &reqId,
                       const QString &question,
                       const QStringList &answer,
                       bool isRetry,
                       int err, const QString &errorInfo,
                       int actionType,
                       const QString &llmIcon,
                       const QString &llmName);
    QString getChatRecords(bool lastRec);
    void clearChatRecords();

    //Image operations
    bool saveImageAs(const QString &filePath);
    bool previewImage(const QString &filePath);
    void copyImg2Clipboard(const QString &filePath);

    //Network state check
    bool isNetworkAvailable();
    bool isKnowledgeBaseExist() { return m_knowledgeBaseExist; }

    void personalFAQGenerate();

signals:
    void llmAccountLstChanged(const QString &currentAccountId,
                              const QString &accountLst);
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

    void assistantChanged(int id);
    void knowledgeBaseExistChanged( bool exist);
    void knowledgeBaseFAQGenFinished();

protected slots:
    void onCommandsReceived(const QVariantMap &commands);
    void onChatTextReceived(const QString &callId, const QString &chatText);
    void onChatError(const QString &id, int code, const QString &errorString);
    void onTextToPictureData(const QString &id, const QList<QByteArray> &imageData);

    void onAddToServerStatusChanged(const QStringList &files, int status);
    void onIndexDeleted(const QStringList &files);

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

    //Ai temperature
    qreal m_temperature {1.0};

    const int   m_limitAQCount {6};
    QVector<QJsonObject> m_aiFAQ;
    QVector<QJsonObject> m_assistantFAQ;
    QVector<QJsonObject> m_persKnowledgeBaseFAQ;
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

        QVector<AiConversation> replys;
    };

    friend QDebug &operator<<(QDebug &debug, const AiChatRecord &r);

    static bool makeJsonFromChatRecord(const AiChatRecord &rec /*in*/, QJsonArray &historyArray /*out*/);

    QMap<QString, QVector<AiChatRecord>> m_assistantChatHistories;

    //Temporary directory to save TTP
    QTemporaryDir m_ttpDir;

    bool m_isAnswering = false;
    bool m_knowledgeBaseExist = false;
    bool m_isFAQGenerating = false;

private:
    //Make ai request context
    QString makeAiReqContext(const QString &question, bool isFAQGeneration = false);
    void processAiRecord(const AiChatRecord &rec /*in*/, QJsonArray &ctxArray/*out*/);

    bool isKnowAssistantType();
};

#define EAiExec() (ESingleton<EAiExecutor>::getInstance())
#endif // EAIEXECUTOR_H
