#ifndef SEESIONPRIVATE_H
#define SEESIONPRIVATE_H

#include "serverdefs.h"
#include "networkdefs.h"

#include "llmservicevendor.h"

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
     * @param tmpLLMAccount: LLM Account
     * @param conversation: A description of a paragraph.
     * @param functionName: Request Function Name
     * @param stream: Conversation flow switch.
     * @param temperature: A parameter that returns randomness, where a higher value indicates higher randomness.
     */
    QPair<AIServer::ErrorType, QStringList> requestChatFunctionText(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params);
    QPair<AIServer::ErrorType, QStringList> requestInstFunction(LLMServerProxy tmpLLMAccount, const QString &conversation, const QJsonArray &funcs, const QVariantHash &params);
    QPair<AIServer::ErrorType, QStringList> requestPlugin(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params);
    QPair<AIServer::ErrorType, QStringList> requestAgent(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params);
    QPair<AIServer::ErrorType, QStringList> requestRag(LLMServerProxy tmpLLMAccount, const QString &conversation, const QVariantHash &params);

    // 只返回reqID，error统一通过信号发送
    QString chatRequest(LLMServerProxy llmAccount, const QString &ctx, const QVariantHash &params);
    QString genImageRequest(LLMServerProxy tmpLLMAccount, const QString &imageDesc);
    QString searchRequest(LLMServerProxy llmAccount, const QString &ctx);

    void claimUsageRequest(LLMServerProxy llmAccount);

    void checkUpdateAssistantAccount(const QString &assistantId);

    /**
     * @brief handleRequestError
     * @param copilot
     * @param uuid
     * @return
     */
    bool handleRequestError(QSharedPointer<LLM> copilot, const QString &uuid);

    bool supportFunctionCall(const LLMServerProxy &account);

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
                               , const QJsonArray &functions, const LLMServerProxy &llmAccount);

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
    bool checkLLMAccountStatus(const QString &uuid, LLMServerProxy &llmAccount, int &errorType);

    /**
     * @brief ChatSeesionPrivate::handleLocalModel
     * @param copilot
     * @param uuid
     * @param conversation
     * @return
     */
    QString handleLocalModel(QSharedPointer<LLM> copilot, const QString &uuid, const QString &conversation, const LLMServerProxy &llmAccount);

    QVariant getFAQ();

    void increaseAccountUsage(const LLMServerProxy &llmAccount, int chatAction) const;

    QString ragPromptBuild(const QString &conversation);

private slots:
    /**
     * @brief Task request has ended.
     */
    void onRequestTaskFinished();

private:
    void onError(const QString &uuid, int errCode, const QString &errInfo);

    bool m_needQueryFunctions = true;

    QString m_appId;
    LLMServerProxy m_appServerProxy;
    AssistantProxy m_assistantProxy;

    QList<QString> m_runTaskIds;



private:
    Session *m_q;
    friend class Session;
};

#endif // SEESIONPRIVATE_H
