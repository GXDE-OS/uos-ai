#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include "serverdefs.h"
#include "audioaiassistant.h"

#include <QSharedPointer>
#include <QJsonArray>

class Session;
class DBusInterface;

namespace uos_ai { namespace chatbot { class ChatBotService; } }

UOSAI_BEGIN_NAMESPACE
class ChatDBusInterface;
UOSAI_END_NAMESPACE

class ServerWrapper : public QObject
{
    Q_OBJECT
public:
    ServerWrapper();
    static ServerWrapper *instance();

signals:
    void sigToLaunchMgmt(bool showAddllmPage, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString & locateTitle = "");

    void sigToLaunchChat(int);

    void sigToLaunchAbout();

    void sigToLaunchWordWizard();

    void sigInputPrompt(const QString &question, const QMap<QString, QString> &params);

    void sigAppendPrompt(const QString &question);

    void sigAddKnowledgeBase(const QStringList &file);

    void sigToTranslate();

    void sigToStartScreenshot();

    void sigToLaunchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath);

    void sigToUploadImage(const QString &imagePath);

    void sigToLaunchGetFreeAccountDlg();

public:
    /**
     * @brief registerService
     * @return
     */
    static bool registerService();

    /**
     * @brief initialization Register for dubs services and objects
     */
    bool initialization();

    /**
     * @brief createChatSession
     * @return
     */
    QSharedPointer<Session> createChatSession();

    /**
     * @brief The LLM account list has been updated
     * including additions, edits, and deletions of LLM accounts.
     */
    void updateLLMAccount();

    /**
     * @brief Update User experience plan State.
     */
    void updateUserExpState(int state);

    /**
     * @brief addAppFunction
     * @param appId
     * @param funciton
     */
    void addAppFunction(const QString &appId, const QJsonObject &funciton);

    void updateVisibleState(bool visible);
    void updateActiveState(bool active);

    uos_ai::chatbot::ChatBotService *chatBotService() const { return m_chatBotService; }

public:
    /**
     * @brief LLM model account validity verification.
     * @return
     */
    static QPair<int, QString> verify(const LLMServerProxy &serverProxy);
private:
    QSharedPointer<DBusInterface> m_copilotDbusObject;
    QSharedPointer<UOSAI_NAMESPACE::ChatDBusInterface> m_chatDbusObject;
    QSharedPointer<Session> m_copilotSeesion;
    UOSAI_NAMESPACE::AudioAiassistant *m_audioAiassistant = nullptr;
    uos_ai::chatbot::ChatBotService *m_chatBotService = nullptr;
};

#endif // SERVERWRAPPER_H
