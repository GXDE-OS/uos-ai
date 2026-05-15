#ifndef APPLICATION_H
#define APPLICATION_H

#include "mgmtwindow.h"

#include <DApplication>

#define aiApp (static_cast<uos_ai::Application *>(QCoreApplication::instance()))

namespace uos_ai {

class WordWizard;
class Application : public DTK_WIDGET_NAMESPACE::DApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application() override;

    static int handleExistingArgument(int argc, char *argv[]);
    static bool handleWordWizardArgument(int argc, char *argv[]);
// 3.0
public slots:
    void initMgmtWindow();
    void showConfig(int page);
    void showChatWindow();
    void showTranslate();
    void startScreenshot();
    void launchChatWindow(int index);
    void launchWordWizard();
    void launchAiQuick(int type, QString query, QPoint pos, bool isCustom, QString imagePath = "");
    void uploadImage(const QString &imagePath);
    void addKnowledgeBasefile(const QStringList &knowledgeBasefile);
    void inputPrompt(const QString &question, const QMap<QString, QString> &params);
    void appendPrompt(const QString &question, bool isSend);
//
public:
    void initialization();
    void handleArgumentsParser(const QStringList &arguments);
    void initWordWizard(WordWizard *wizard);
protected:
    void handleAboutAction() override;
private:
    bool checkAgreement();
    void initShortcut();
private:
    MgmtWindow *m_mgmtWindow = nullptr;
    WordWizard *m_wordWizard = nullptr;
};
}

#endif // APPLICATION_H
