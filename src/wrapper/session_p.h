#ifndef SEESIONPRIVATE_H
#define SEESIONPRIVATE_H

#include "serverdefs.h"
#include "networkdefs.h"

class LLM;
class Session;
class SessionPrivate : public QObject
{
    Q_OBJECT
public:
    explicit SessionPrivate(Session *session, const QString &appId);
    ~SessionPrivate();

    /**
     * @brief Cancel the task ID that is currently being requested.
     * @param id: Session ID for the dialogue interface during the request.
     */
    void cancelRequestTask(const QString &id);

private:
    /**
     * @brief Request the chat session function interface.
     * @param llmId: LLM Account Id
     * @param conversation: A description of a paragraph.
     * @param functionName: Request Function Name
     * @param stream: Conversation flow switch.
     * @param temperature: A parameter that returns randomness, where a higher value indicates higher randomness.
     */
    QPair<AIServer::ErrorType, QStringList> requestChatFunctionText(const QString &llmId, const QString &conversation
                                                                    , bool stream = false
                                                                    , qreal temperature = 1.0);

    /**
     * @brief checkUpdateLLmAccount
     * @param llmId
     */
    void checkUpdateLLmAccount(const QString &llmId);

    /**
     * @brief handleRequestError
     * @param copilot
     * @param uuid
     * @return
     */
    bool handleRequestError(QSharedPointer<LLM> copilot, const QString &uuid);

    /**
     * @brief handleAiServerRequest
     * @param copilot
     * @param uuid
     * @param response
     * @param conversation
     * @param functions
     * @param temperature
     * @param model
     * @return
     */
    bool handleAiServerRequest(QSharedPointer<LLM> copilot, const QString &uuid
                               , QJsonObject &response, const QString &conversation
                               , const QJsonArray &functions, qreal temperature, LLMChatModel model, const QString &userName);

    /**
     * @brief queryValidServerAccount
     * @return
     */
    LLMServerProxy queryValidServerAccount(const QString &llmId);

    /**
     * @brief checkLLMAccountStatus
     * @param llmAccount
     * @return
     */
    bool checkLLMAccountStatus(const QString &uuid, const LLMServerProxy &llmAccount);

    /**
     * @brief ChatSeesionPrivate::handleLocalModel
     * @param copilot
     * @param uuid
     * @param conversation
     * @return
     */
    QString handleLocalModel(QSharedPointer<LLM> copilot, const QString &uuid, const QString &conversation, const LLMServerProxy &llmAccount);

private slots:
    /**
     * @brief Task request has ended.
     */
    void onRequestTaskFinished();

private:
    bool m_needQueryFunctions = true;

    QString m_appId;
    LLMServerProxy m_appServerProxy;

    QList<QString> m_runTaskIds;

private:
    Session *m_q;
    friend class Session;
};

#endif // SEESIONPRIVATE_H
