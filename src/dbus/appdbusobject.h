#ifndef APPDBUSOBJECT_H
#define APPDBUSOBJECT_H

#include <QDBusContext>
#include <QSharedPointer>

namespace uos_ai {

class SessionManager;
class ConversationRecord;
class AppDbusObject : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.copilot.app")

public:
    explicit AppDbusObject(const QString &appId);
    virtual ~AppDbusObject();

    /**
     * @brief Interrupt the task.
     */
    void executionAborted();

public Q_SLOTS:
    /**
     * @brief Cancel the task ID that is currently being requested.
     * @param id: Session ID for the dialogue interface during the request.
     */
    Q_SCRIPTABLE void cancelRequestTask(const QString &id);

    /**
     * @brief requestChatText: Responding to user conversations
     * @param conversation
     * @param stream: Conversation flow switch.
     * @param temperature: A parameter that returns randomness, where a higher value indicates higher randomness.
     */
    Q_SCRIPTABLE QStringList requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream = false);

    /**
     * @brief Set the current model ID.
     * @param id: LLM model ID
     */
    Q_SCRIPTABLE bool setCurrentLLMAccountId(const QString &id);

    /**
     * @brief Get the current LLM model ID.
     * @return
     */
    Q_SCRIPTABLE QString currentLLMAccountId();

    /**
     * @brief Get the list of all LLM model accounts.
     * @return JsonArray: [{\"id\":\"xxx\",\"name\":\"xxx\",\"type\":0}]
     * type -> 0:GPT3.5, 1:GPT_3_5_16, 2:GPT_4, 3:GPT_4_32K, 10:SPARKDESK
     */
    Q_SCRIPTABLE QString queryLLMAccountList();

signals:
    /**
     * @brief Return the error message for the request interface.
     * @param id: Request ID
     * @param code: Error Code
     * @param errorString: Error Message
     */
    Q_SCRIPTABLE void error(const QString &id, int code, const QString &errorString);

    /**
     * @brief Return the received chat content from the request.
     * @param id: Request ID
     * @param chatText: Response Text
     */
    Q_SCRIPTABLE void chatTextReceived(const QString &id, const QString &chatText);

    /**
     * @brief llmAccountLstChanged
     * @param currentAccountId: Current LLM Model Id
     * @param accountLst => JsonArray: [{\"id\":\"xxx\",\"name\":\"xxx\",\"type\":0}]
     * type -> 0:GPT3.5, 1:GPT_3_5_16, 2:GPT_4, 3:GPT_4_32K, 10:SPARKDESK
     */
    Q_SCRIPTABLE void llmAccountLstChanged(const QString &currentAccountId, const QString &accountLst);

//to be removed in the future
public Q_SLOTS:
    /**
     * @brief Launch LLM UI page.
     * This function was added in v1.0 but replaced by others in v1.1
     */
    Q_SCRIPTABLE void launchLLMUiPage(bool showAddllmPage);
protected Q_SLOTS:
    void onEvent(int event, const QString &id, const QString &json);
protected:
    struct ChatTask {
        QString id;
        bool stream = false;
        QSharedPointer<ConversationRecord> record;
    };
    bool checkAgreement();
    QSharedPointer<ConversationRecord> convertRequestion(const QString &id, const QString &json);
protected:
    QMap<QString, ChatTask> chatTask;
    QString app;
    QSharedPointer<SessionManager> chatSession;
};

}

#endif // APPDBUSOBJECT_H
