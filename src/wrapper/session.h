#ifndef SESSION_H
#define SESSION_H

#include "serverdefs.h"
#include "networkdefs.h"

#include <QJsonArray>
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
    QPair<AIServer::ErrorType, QStringList> requestChatText(const QString &llmId, const QString &conversation, const QVariantHash &params = {});
    QPair<AIServer::ErrorType, QStringList> requestFunction(const QString &llmId, const QString &conversation, const QJsonArray &funcs, const QVariantHash &params = {});
    QPair<AIServer::ErrorType, QStringList> requestMcpAgent(const QString &llmId, const QString &conversation, const QVariantHash &params = {});
    QPair<AIServer::ErrorType, QStringList> requestRag(const QString &llmId, const QString &conversation, const QVariantHash &params = {});
    QString chatRequest(const QString &llmId, const QString &ctx, const QVariantHash &params = {});
    QString searchRequest(const QString &llmId, const QString &ctx);
    QString requestGenImage(const QString &llmId, const QString &imageDesc);

    void claimUsageRequest(const QString &llmId);

    /**
     * @brief Set the current model ID.
     * @param id: LLM model ID
     */
    bool setCurrentLLMAccountId(const QString &id);
    bool setUosAiLLMAccountId(const QString &id);

    /**
     * @brief Get the current LLM model ID.
     * @return
     */
    QString currentLLMAccountId();
    QString uosAiLLMAccountId();
    LLMChatModel currentLLMModel();
    ModelType currentModelType();

    bool setCurrentAssistantId(const QString &id);
    QString currentAssistantId();
    QString currentAssistantDisplayName();
    QString currentAssistantDisplayNameEn(); // 埋点专用
    AssistantType currentAssistantType();
    QString currentAssistantInstList();

    /**
     * @brief Get the list of all LLM model accounts.
     * @return JsonArray: [{\"id\":\"xxx\",\"name\":\"xxx\",\"type\":0}]
     * type -> 0:GPT3.5, 1:GPT_3_5_16, 2:GPT_4, 3:GPT_4_32K, 10:SPARKDESK
     */
    QString queryLLMAccountList(const QList<LLMChatModel> &excludes = {});
    QString queryLLMAccountListWithRole(const QList<LLMChatModel> &excludes = {});
    QString queryUosAiLLMAccountList();

    QString queryAssistantList();
    QString queryAssistantIdByType(AssistantType type);
    QString queryDisplayNameByType(AssistantType type);
    AssistantType queryAssistantTypeById(const QString &id);
    QString queryIconById(const QString &assistantId, const QString &modelId);
    QString queryDisplayNameById(const QString &assistantId);

    /**
     * @brief Launch LLM UI page.
     */
    void launchLLMUiPage(bool showAddllmPage, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString &locateTitle = "");
    void launchGetFreeAccountDlg();

    void launchKnowledgeBaseUiPage(const QString & locateTitle = "");

    void launchAboutWindow();

    QVariant getFAQ();

    LLMServerProxy queryValidServerAccount(const QString &llmId);

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
     * @brief Emitted with the full agent context (tool call chain) after each request.
     *        Always emitted before chatTextReceived so handlers can store context first.
     * @param id      Request ID
     * @param context OAI-format message array: assistant tool_calls + tool results + final assistant
     */
    void chatContextReceived(const QString &id, const QJsonArray &context);

    /**
     * @brief Emitted for each incremental chunk when stream=true.
     *        Consumers can use this to progressively render output.
     * @param id        Request ID (same as chatRequest return value)
     * @param deltaText The new incremental text fragment
     */
    void chatTextChunkReceived(const QString &id, const QString &deltaText);

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
    void uosAiLlmAccountLstChanged();

    /**
     * @brief Interrupt the task.
     */
    void executionAborted();

    void assistantListChanged();

    void previewReference(const QString &reference);

    void sigClaimAccountUsageFinished(bool ret, const QString &msg);
    void sigClaimAgain(bool claimAgain);

private:
    QScopedPointer<SessionPrivate> m_private;
    QString m_uosaiTopAccountId;
};

#endif // SESSION_H
