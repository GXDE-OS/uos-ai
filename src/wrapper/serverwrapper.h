#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include "serverdefs.h"
#include "compliance/aiassistantsubstitute.h"

#include <QSharedPointer>
#include <QJsonArray>

class Session;
class DBusInterface;
class ServerWrapper : public QObject
{
    Q_OBJECT
public:
    ServerWrapper();
    static ServerWrapper *instance();

signals:
    void sigToLaunchMgmt(bool showAddllmPage, bool onlyUseAgreement = false);

    void sigToLaunchChat(int);

    void sigToLaunchAbout();
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

public:
    /**
     * @brief LLM model account validity verification.
     * @return
     */
    static QPair<int, QString> verify(const LLMServerProxy &serverProxy);
private:
    QSharedPointer<DBusInterface> m_copilotDbusObject;
    QSharedPointer<Session> m_copilotSeesion;
    UOSAI_NAMESPACE::AiassistantSubstitute *m_aiassistant = nullptr;
};

#endif // SERVERWRAPPER_H
