#ifndef APPLICATION_H
#define APPLICATION_H
#include "uosai_global.h"
#include "dbus/shortcutmanager.h"

#include <DApplication>

#include "mgmtwindow.h"

class ChatManager;
class ChatWindow;

UOSAI_BEGIN_NAMESPACE
class WordWizard;
UOSAI_END_NAMESPACE

class Application : public DApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();

    static int handleExistingArgument(int argc, char *argv[]);
    static bool handleWordWizardArgument(int argc, char *argv[]);

public:
    void initialization();

    void launchMgmtWindow(bool, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString &locateTitle = "");

    void launchChatWindow(int index);

    void handleArgumentsParser(const QStringList &arguments);

    void launchAboutWindow();

    void launchMgmtWindowNoShow();

    void launchWordWizard();

    void launchAiQuickDialog(int type, QString query, QPoint pos, bool isCustom);

    void launchAiQuickOCRDialog(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath);

    void launchAiWriterDialog();

    void initWordWizard(UOSAI_NAMESPACE::WordWizard *wizard);

protected:
    void handleAboutAction() override { launchAboutWindow(); }

public slots:
    void onSignalShowMgmtWindowAfterChatInitFinished();

private slots:
    void onLaunchMgmt(bool, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString &locateTitle = "");

    void onLaunchGetFreeAccountDlg();

    void onLaunchChat(int index);

    void onLaunchAbout();

    void onLaunchMgmtNoShow();

    void onLaunchWordWizard();

    void onLaunchAiQuick(int type, QString query, QPoint pos, bool isCustom);

    void onLaunchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath);

    void onLaunchAiWriter();

    void onInputPrompt(const QString &question, const QMap<QString, QString> &params);

    void onAppendPrompt(const QString &question);

    void onAddKnowledgeBasefile(const QStringList &knowledgeBasefile);

    void onTranslate();

    void onStartScreenshot();

    void onUploadImage(const QString &imagePath);

private:
    bool checkAgreement();

    void initShortcut();

signals:
    void sigGenPersonalFAQ();

private:
    MgmtWindow *m_mgmtWindow = nullptr;
    ChatWindow *m_chatWindow = nullptr;
    UOSAI_NAMESPACE::WordWizard *m_wordWizard = nullptr;
};

#endif // APPLICATION_H
