#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QThread>
#include <QVariantMap>
#include <QSharedPointer>
#include <QDBusContext>


#define DBUS_SERVER             "com.deepin.copilot"
#define DBUS_SERVER_PATH        "/com/deepin/copilot"
#define DBUS_SERVER_INTERFACE   "com.deepin.copilot"

namespace uos_ai {
class AppDbusObject;
class DBusInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.copilot")

    struct AppDbusPathObject {
        QString appId;
        QString path;
        QString curLLMId;
        QVariantMap cmdPrompts;
        QSharedPointer<AppDbusObject> object;
    };
public:
    explicit DBusInterface(QObject *parent = nullptr);
    ~DBusInterface();

signals:
    /**
     * @brief User experience plan state Changed
     */
    Q_DECL_DEPRECATED Q_SCRIPTABLE void userExpStateChanged(bool);

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
    Q_DECL_DEPRECATED Q_SCRIPTABLE bool queryUserExpState();

    /**
     * @brief A list of functions to be performed by the called program.
     * This method needs to be called after the application recognises the --functioncall parameter to get the tasks that need to be performed.
     * This function was added in v1.1
     * @return A list of functions in json format
     */
     Q_DECL_DEPRECATED Q_SCRIPTABLE QString cachedFunctions();

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

    /**
     * @brief raise the wordwizard widget
     * This function was added in v1.4
     */
    Q_SCRIPTABLE void launchWordWizard();

    /**
     * @brief ctrl + Alt + U Translate the selected text
     * This function was added in v2.7
     */
    Q_SCRIPTABLE void textTranslation();

    /**
     * @brief ctrl + Alt + Q Start screenshot for AI analysis
     * This function was added in v2.10
     */
    Q_SCRIPTABLE void startScreenshot();

    Q_SCRIPTABLE bool isCopilotEnabled();

    /**
     * @brief for screenshot to upload picture
     * This function was added in v2.12
     */
    Q_SCRIPTABLE void launchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath);
    Q_SCRIPTABLE void launchChatUploadImage(const QString &imagePath);
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
    void updateVisibleState(bool visible);
    void updateActiveState(bool active);
    static QString adjustDbusPath(QString appId);

private:
    QMap<QString, AppDbusPathObject> m_appDbusObjects;
};

}

#endif // DBUSINTERFACE_H
