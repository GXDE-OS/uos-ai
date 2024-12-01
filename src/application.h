#ifndef APPLICATION_H
#define APPLICATION_H

#include <DApplication>

#include "mgmtwindow.h"

DWIDGET_USE_NAMESPACE
class ChatManager;
class ChatWindow;
class Application : public DApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();

    static int handleExistingArgument(int argc, char *argv[]);

public:
    void initialization();

    void launchMgmtWindow(bool, bool onlyUseAgreement = false);

    void launchChatWindow(int index);

    void handleArgumentsParser(const QStringList &arguments);

    void launchAboutWindow();

private slots:
    void onLaunchMgmt(bool, bool onlyUseAgreement = false);

    void onLaunchChat(int index);

    void onLaunchAbout();

signals:
    void sigGenPersonalFAQ();

private:
    MgmtWindow *m_mgmtWindow = nullptr;
    ChatWindow *m_chatWindow = nullptr;
};

#endif // APPLICATION_H
