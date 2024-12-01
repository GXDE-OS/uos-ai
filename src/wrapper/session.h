#ifndef SESSION_H
#define SESSION_H

#include "serverdefs.h"
#include "networkdefs.h"

#include <QScopedPointer>

class SessionPrivate;
class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(const QString &appId, QObject *parent = nullptr);
    ~Session();

    /**
     * @brief The LLM account list has been updated
     * including additions, edits, and deletions of LLM accounts.
     */
    void updateLLMAccount();

    /**
     * @brief appId
     * @return
     */
    QString appId() const;

    /**
     * @brief llmServerProxy
     * @return
     */
    LLMServerProxy llmServerProxy() const;

public Q_SLOTS:
    /**
     * @brief Cancel the task ID that is currently being requested.
     * @param id: Session ID for the dialogue interface during the request.
     */
    void cancelRequestTask(const QString &id);

    /**
     * @brief requestChatText: Responding to user conversations
     * @param conversation
     * @param stream: Conversation flow switch.
     * @param temperature: A parameter that returns randomness, where a higher value indicates higher randomness.
     */
    QPair<AIServer::ErrorType, QStringList> requestChatText(const QString &llmId, const QString &conversation, qreal temperature, bool stream = false, bool isFAQGeneration = false);

    /**
     * @brief Set the current model ID.
     * @param id: LLM model ID
     */
    bool setCurrentLLMAccountId(const QString &id);

    /**
     * @brief Get the current LLM model ID.
     * @return
     */
    QString currentLLMAccountId();
    LLMChatModel currentLLMModel();
    ModelType currentModelType();

    bool setCurrentAssistantId(const QString &id);
    QString currentAssistantId();
    QString currentAssistantDisplayName();
    AssistantType currentAssistantType();

    /**
     * @brief Get the list of all LLM model accounts.
     * @return JsonArray: [{\"id\":\"xxx\",\"name\":\"xxx\",\"type\":0}]
     * type -> 0:GPT3.5, 1:GPT_3_5_16, 2:GPT_4, 3:GPT_4_32K, 10:SPARKDESK
     */
    QString queryLLMAccountList(const QList<LLMChatModel> &excludes = {});
    QString queryLLMAccountListWithRole(const QList<LLMChatModel> &excludes = {});

    QString queryAssistantList();
    QString queryAssistantIdByType(AssistantType type);

    /**
     * @brief Launch LLM UI page.
     */
    void launchLLMUiPage(bool showAddllmPage, bool onlyUseAgreement = false);

    void launchAboutWindow();

    QVariant getFAQ();

signals:
    /**
     * @brief Return the error message for the request interface.
     * @param id: Request ID
     * @param code: Error Code
     * @param errorString: Error Message
     */
    void error(const QString &id, int code, const QString &errorString);

    /**
     * @brief chatAction
     */
    void chatAction(const QString &id, int action);

    /**
     * @brief Return the received chat content from the request.
     * @param id: Request ID
     * @param chatText: Response Text
     */
    void chatTextReceived(const QString &id, const QString &chatText);

    /**
     * @brief text2ImageReceived
     * @param id
     * @param imageData
     */
    void text2ImageReceived(const QString &id, const QList<QByteArray> &imageData);

    /**
     * @brief llmAccountLstChanged
     * @param currentAccountId: Current LLM Model Id
     * @param accountLst => JsonArray: [{\"id\":\"xxx\",\"name\":\"xxx\",\"type\":0}]
     * type -> 0:GPT3.5, 1:GPT_3_5_16, 2:GPT_4, 3:GPT_4_32K, 10:SPARKDESK
     */
    void llmAccountLstChanged(const QString &currentAccountId, const QString &accountLst);

    /**
     * @brief Interrupt the task.
     */
    void executionAborted();

private:
    QScopedPointer<SessionPrivate> m_private;
};

#endif // SESSION_H
