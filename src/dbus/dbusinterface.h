#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include "appdefs.h"

#include <QThread>
#include <QVariantMap>
#include <QSharedPointer>
#include <QDBusContext>

class AppDbusObject;
class DBusInterface : public QThread, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.copilot")

    enum TaskType {
        TASK_UPDATE_LLM_ACCOUNT = 0,
    };

public:
    explicit DBusInterface(QObject *parent = nullptr);
    ~DBusInterface();

signals:
    /**
     * @brief User experience plan state Changed
     */
    Q_SCRIPTABLE void userExpStateChanged(bool);

    Q_SCRIPTABLE void windowVisibleChanged(bool);

    Q_SCRIPTABLE void windowActiveChanged(bool);

public Q_SLOTS:
    /**
     * @brief specify the version of the current interface, because the LLM is updated very quickly, and a number of old interfaces will be eliminated in the future;
     * therefore, the access application needs to be updated in a timely manner, or use this method to be compatible with different versions of uos ai
     * This function was added in v1.0
     * @return current version
     */
    Q_SCRIPTABLE QString version();

    /**
     * @brief register dbus service, get an exclusive path.
     * This function was added in v1.1
     * @return dbus path
     */
    Q_SCRIPTABLE QString registerApp();

    /**
     * @brief anti-registration, removal of exclusive dbus services.
     * This function was added in v1.1
     */
    Q_SCRIPTABLE void unregisterApp();

    /**
     * @brief check the status of whether the user agreement is agreed or not.
     * This function was added in v1.0
     * @return yes or not
     */
    Q_SCRIPTABLE bool queryUserExpState();

    /**
     * @brief A list of functions to be performed by the called program.
     * This method needs to be called after the application recognises the --functioncall parameter to get the tasks that need to be performed.
     * This function was added in v1.1
     * @return A list of functions in json format
     */
    Q_SCRIPTABLE QString cachedFunctions();

    /**
     * @brief raise the chat window.
     * This function was added in v1.1
     */
    Q_SCRIPTABLE void launchChatPage(int index = 0);

    /**
     * @brief LLM Configuration dialog box pops up.
     * This function was added in v1.0
     * @param openAddAccountDialog Add llm dialog opens immediately after launch
     */
    Q_SCRIPTABLE void launchLLMUiPage(bool openAddAccountDialog);


//to be removed in the future
public Q_SLOTS:
    /**
     * @brief registration app instruction prompt phrase
     * This function was added in v1.0 but replaced by registerApp in v1.1
     * @param cmdPrompts: Instruction prompt words.
     * @return The first in the list is the dbus path.
     */
    Q_SCRIPTABLE QStringList registerAppCmdPrompts(const QVariantMap &cmdPrompts);

    /**
     * @brief Cancel registration of the application
     * This function was added in v1.0 but replaced by unregisterApp in v1.1
     */
    Q_SCRIPTABLE void unregisterAppCmdPrompts();

public:
    /**
     * @brief Asynchronously update the model account information.
     */
    void asyncUpdateLLMAccount();

    /**
     * @brief Update User experience plan.
     */
    void updateUserExpState(int state);

    void updateVisibleState(bool visible);
    void updateActiveState(bool active);
    /**
     * @brief addAppFunction
     * @param appId
     * @param funciton
     */
    void addAppFunction(const QString &appId, const QJsonObject &funciton);

    QJsonArray appFunctions();

signals:
    void sigTask(TaskType type);

    void sigToLaunchMgmt(bool showAddllmPage, bool onlyUseAgreement = false);

    void sigToLaunchChat(int index);

private slots:
    /**
     * @brief Execute the task received from the signal sigTask.
     * @param type
     */
    void onProcessTask(TaskType type);

    /**
     * @brief Request task completion.
     */
    void onRequestTaskFinished();

private:
    /**
     * @brief Update LLM account information.
     */
    void updateLLMAccount();

private:
    QMap<QString, AppDbusPathObject> m_appDbusObjects;
    QMap<QString, QJsonArray> m_appFunctions;
};

#endif // DBUSINTERFACE_H
