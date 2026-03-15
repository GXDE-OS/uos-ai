#include "application.h"
#include "serverwrapper.h"
#include "llmutils.h"
#include "functionhandler.h"
#include "chatwindow.h"
#include "localmodelserver.h"
#include "wordwizard/gui/aiquickdialog.h"
#include "wordwizard/gui/aiwriterdialog.h"
#include "wordwizard/wordwizard.h"
#include "dbwrapper.h"
#include "private/welcomedialog.h"
#include "esystemcontext.h"
#include "gui/upgrade/deepseekinfo.h"
#include "mcpconfigsyncer.h"
#include "global_define.h"
#include <report/chatwindowpoint.h>
#include <report/eventlogutil.h>

#include <QtDBus>
#include <QCommandLineParser>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logMain)

constexpr char APP_NAME_KEY[]   =   "uos-ai-assistant";

static const QList<QCommandLineOption> options = {
    {
        {"c", "chat"},
        "ChatWindow Auto Display"
    }, {
        {"f", "functions"},
        "print function calling list"
    },
    {
        {"t", "talk"},
        "ChatWindow for talk Auto Display"
    },
    {
        {"w", "wordwizard"},
        "Display Scratch toolbar when hidden"
    },
    {
        {"s", "screenshot"},
        "Start screenshot for AI analysis"
    }
};

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    qCDebug(logMain) << "Initializing UOS AI Application";
    setApplicationName(APP_NAME_KEY);
    setObjectName(APP_NAME_KEY);
    setOrganizationName("deepin");
    loadTranslator();
    setApplicationDisplayName(tr("UOS AI"));
    setApplicationVersion(DApplication::buildVersion(APP_VERSION));
    setProductName(tr("UOS AI"));
    setProductIcon(QIcon::fromTheme(kApplicationIconName));
    setApplicationDescription(tr("UOS AI is a desktop smart assistant, your personal assistant! You can communicate with it using text or voice, and it can help answer questions, provide information, and generate images based on your descriptions."));
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setQuitOnLastWindowClosed(false);
    setWindowIcon(QIcon::fromTheme(kApplicationIconName));

    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchMgmt, this, &Application::onLaunchMgmt);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchGetFreeAccountDlg, this, &Application::onLaunchGetFreeAccountDlg);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchChat, this, &Application::onLaunchChat);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchAbout, this, &Application::onLaunchAbout);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchWordWizard, this, &Application::onLaunchWordWizard);
    connect(ServerWrapper::instance(), &ServerWrapper::sigInputPrompt, this, &Application::onInputPrompt);
    connect(ServerWrapper::instance(), &ServerWrapper::sigAppendPrompt, this, &Application::onAppendPrompt);
    connect(ServerWrapper::instance(), &ServerWrapper::sigAddKnowledgeBase, this, &Application::onAddKnowledgeBasefile);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToTranslate, this, &Application::onTranslate);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToStartScreenshot, this, &Application::onStartScreenshot);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchAiQuickOCR, this, &Application::onLaunchAiQuickOCR);
    connect(ServerWrapper::instance(), &ServerWrapper::sigToUploadImage, this, &Application::onUploadImage);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::sigToLaunchMgmtNoShow, this, &Application::onLaunchMgmtNoShow);

    connect(this, &DApplication::iconThemeChanged, this, &Application::onIconThemeChanged);
}

Application::~Application()
{
    if (m_mgmtWindow)
        m_mgmtWindow->deleteLater();

    if (m_chatWindow)
        m_chatWindow->deleteLater();
}

void Application::initialization()
{
    qCDebug(logMain) << "Starting application initialization";
    ServerWrapper::instance()->initialization();

    setAttribute(Qt::AA_UseHighDpiPixmaps);
    initShortcut();
    handleArgumentsParser(arguments());

    McpConfigSyncer::instance()->fetchConfigFromServerAsync();
}

void Application::initShortcut()
{
    if (ESystemContext::isTreeland()) {
        qCDebug(logMain) << "Skip shortcut registration in treeland environment";
        return;
    }

    qCInfo(logMain) << "Initializing application shortcuts";
    ShortcutManager& shortcutMgr = ShortcutManager::getInstance();
    
    if (!shortcutMgr.isValid()) {
        qCWarning(logMain) << "ShortcutManager D-Bus interface is not valid";
        return;
    }

    // 使用枚举获取快捷键控制位
    int isUpdate = DbWrapper::localDbWrapper().getUpdatePromptBits(UpdatePromptBitType::SHORTCUT_UPDATE);

    QString wordwizardName = tr("UOS AI FollowAlong/Write");

    // 查询现有快捷键
    QList<ShortcutInfo> shortcuts = shortcutMgr.searchShortcuts("UOS AI");
    
    ShortcutInfo wordWizardShortcut;
    ShortcutInfo uosAiShortcut;
    ShortcutInfo uosAiTalkShortcut;

    for (const ShortcutInfo &shortcut : shortcuts) {
        if (shortcut.id == "UOS AI") {
            uosAiShortcut = shortcut;
        } else if (shortcut.id == "UOS AI Talk") {
            uosAiTalkShortcut = shortcut;
        } else if (shortcut.id.contains("/")) {
            wordWizardShortcut = shortcut;
        }
    }

    //首次更新至v2.8,替换快捷键
    if (isUpdate == 0) {
        if (!wordWizardShortcut.id.isEmpty() && wordWizardShortcut.id == wordWizardShortcut.name && wordWizardShortcut.accel == "<Super>space") {
            qCInfo(logMain) << "Modifying Wordwizard shortcut from" << wordWizardShortcut.accel << "to <Super>R";
            shortcutMgr.modifyCustomShortcut(wordWizardShortcut.id, wordWizardShortcut.name, wordWizardShortcut.exec, "<Super>R");
        }

        if (!uosAiTalkShortcut.id.isEmpty() && uosAiTalkShortcut.id == uosAiTalkShortcut.name) {
            if (uosAiTalkShortcut.accel == "<Control><Super>C" || uosAiTalkShortcut.accel == "<Control><Super>A") {
                qCInfo(logMain) << "Modifying UOS AI Talk shortcut from" << uosAiTalkShortcut.accel << "to <Control><Super>space";
                shortcutMgr.modifyCustomShortcut(uosAiTalkShortcut.id, uosAiTalkShortcut.name, uosAiTalkShortcut.exec, "<Control><Super>space");
            }
        }

        if (!uosAiShortcut.id.isEmpty() && uosAiShortcut.id == uosAiShortcut.name) {
#ifdef COMPILE_ON_V20
            if (!ESystemContext::isWayland()) {
#endif
                if (uosAiShortcut.accel == "<Super>C" || uosAiShortcut.accel == "<Super>A") {
                    //延迟执行，等待随航快捷键修改完成
                    QTimer::singleShot(0, this, [uosAiShortcut]{
                        qCInfo(logMain) << "Modifying UOS AI shortcut from" << uosAiShortcut.accel << "to <Super>space";
                        ShortcutManager::getInstance().modifyCustomShortcut(uosAiShortcut.id, uosAiShortcut.name, uosAiShortcut.exec, "<Super>space");
                    });
                }
#ifdef COMPILE_ON_V20
            }
#endif
        }
    }

    //随航快捷键跟随语言切换名称
    if (!wordWizardShortcut.id.isEmpty() && wordWizardShortcut.id == wordWizardShortcut.name &&  wordWizardShortcut.id !=wordwizardName)
        shortcutMgr.deleteCustomShortcut(wordWizardShortcut.id);

    shortcutMgr.addCustomShortcut(wordwizardName, "/usr/bin/uos-ai-assistant --wordwizard", "<Super>R");
    qCInfo(logMain) << "Add wordwizard shortcut <Super>R";
    shortcutMgr.addCustomShortcut("UOS AI Talk", "/usr/bin/uos-ai-assistant --talk", "<Control><Super>space");
    qCInfo(logMain) << "Add UOS AI Talk shortcut <Control><Super>space";
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        shortcutMgr.addCustomShortcut("UOS AI", "/usr/bin/uos-ai-assistant --chat", "<Super>C");
        qCInfo(logMain) << "Add UOS AI shortcut <Super>C";
        return;
    }
#endif
    shortcutMgr.addCustomShortcut("UOS AI", "/usr/bin/uos-ai-assistant --chat", "<Super>space");
    qCInfo(logMain) << "Add UOS AI shortcut <Super>space";
    
#ifdef COMPILE_ON_V25
    // 添加截图快捷键 Ctrl+Alt+Q (目前仅在v25版本下)
    shortcutMgr.addCustomShortcut("UOS AI Screenshot", "/usr/bin/uos-ai-assistant --screenshot", "<Control><Alt>Q");
    qCInfo(logMain) << "Add UOS AI Screenshot shortcut <Control><Alt>Q";
#endif
}

void Application::launchMgmtWindow(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString &locateTitle)
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "User agreement not accepted, cannot launch management window";
        return;
    }

    qCInfo(logMain) << "Launching management window with parameters:"
                   << "showAddllmPage:" << showAddllmPage
                   << "onlyUseAgreement:" << onlyUseAgreement
                   << "isFromAiQuick:" << isFromAiQuick;
    launchMgmtWindowNoShow();
    m_mgmtWindow->showEx(showAddllmPage, onlyUseAgreement, isFromAiQuick, locateTitle);
}

void Application::launchMgmtWindowNoShow()
{
    if (!m_mgmtWindow) {
        qCInfo(logMain) << "Creating new management window";
        m_mgmtWindow = new MgmtWindow();
        connect(m_mgmtWindow, &MgmtWindow::sigGenPersonalFAQ, this, &Application::sigGenPersonalFAQ, Qt::UniqueConnection);
        connect(m_mgmtWindow, &MgmtWindow::sigSetRedPointVisible, m_chatWindow, &ChatWindow::onRedPointVisible);
        if (m_wordWizard) {
            bool isHidden = m_wordWizard->queryHiddenStatus();
            qCInfo(logMain) << "wordWizard hiddenStatus" << isHidden;
            m_mgmtWindow->onWordWizardHiddenStatus(isHidden);
            connect(m_wordWizard, &WordWizard::signalHiddenwidget, m_mgmtWindow, &MgmtWindow::onWordWizardHiddenStatus, Qt::UniqueConnection);
            connect(m_wordWizard, &WordWizard::signalAddDisabledApp, m_mgmtWindow, &MgmtWindow::onAddDisabledApp, Qt::UniqueConnection);
            connect(m_mgmtWindow, &MgmtWindow::signalWordWizardStatusChanged, m_wordWizard, &WordWizard::onChangeHiddenStatus, Qt::UniqueConnection);
            connect(m_mgmtWindow, &MgmtWindow::signalDisabledAppsUpdated, m_wordWizard, &WordWizard::updateDisabledApps, Qt::UniqueConnection);
        }
        connect(m_chatWindow, &ChatWindow::sigThirdPartyMcpAgree, m_mgmtWindow, &MgmtWindow::sigThirdPartyMcpAgree);
    }
}

void Application::launchChatWindow(int index)
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "User agreement not accepted, cannot launch chat window";
        return;
    }

    qCInfo(logMain) << "Launching chat window with index:" << index;
    if (!m_chatWindow) {
        if (!DeepSeekInfo::checkAndShow()) {
            qCWarning(logMain) << "checkAndShow block launching chat window";
            return;
        }
        m_chatWindow = ESystemContext::createWebWindow<ChatWindow>();
        connect(this, &Application::sigGenPersonalFAQ, m_chatWindow, &ChatWindow::onGenPersonalFAQ, Qt::UniqueConnection);
        connect(m_chatWindow, &ChatWindow::sigToAddKnowledgeBase, this, [this](const QStringList &knowledgeBasefile){
            this->launchMgmtWindowNoShow();
            this->m_mgmtWindow->showEx(false, false, false, tr("Knowledge Base Management"));
            this->m_mgmtWindow->onAddKnowledgeBase(knowledgeBasefile);
        });
    }

    m_chatWindow->showWindow(static_cast<ChatIndex>(index));

    QTimer::singleShot(3000, this, [this](){
        qCInfo(logMain) << "Delayed launching mgmt window.";
        launchMgmtWindowNoShow();
        if (!m_mgmtWindow->isVisible())
            m_mgmtWindow->checkUpdateStatus();
    });

    // tid:1001600006 event:chatwindow
    ReportIns()->writeEvent(report::ChatwindowPoint().assemblingData());
}

void Application::launchAboutWindow()
{
    if (!m_chatWindow) {
        qWarning(logMain) << "Cannot launch about window - chat window not available";
        return;
    }
    qCDebug(logMain) << "Showing about window";
    m_chatWindow->showAboutWindow();
}

void Application::launchWordWizard()
{
    if (!m_wordWizard) {
        qCWarning(logMain) << "Cannot launch word wizard - word wizard not initialized";
        return;
    }
    qCInfo(logMain) << "Launching word wizard via shortcut";
    m_wordWizard->onShortcutPressed();
}

void Application::launchAiQuickDialog(int type, QString query, QPoint pos, bool isCustom)
{
    qCInfo(logMain) << "Launching AI quick dialog with type:" << type << "query size:" << query.size();
    AiQuickDialog *aiQuickDlg = new AiQuickDialog(m_wordWizard);
    aiQuickDlg->setQuery(type, query, pos, isCustom);

    connect(m_wordWizard, &WordWizard::sigToCloseAiQuick, aiQuickDlg, [ aiQuickDlg ] {
        if (aiQuickDlg)
            aiQuickDlg->close();
    });

    aiQuickDlg->show();
}

void Application::launchAiQuickOCRDialog(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath)
{
    if (!WelcomeDialog::isAgreed()) {
        qCInfo(logMain) << "Showing welcome dialog for input prompt";
        WelcomeDialog::instance(false)->exec();
        return;
    }
    qCInfo(logMain) << "Launching AI quick dialog with type:" << type << "query size:" << query.size();
    AiQuickDialog *aiQuickDlg = new AiQuickDialog(m_wordWizard);
    aiQuickDlg->setQuery(type, query, pos, isCustom, imagePath);

    connect(m_wordWizard, &WordWizard::sigToCloseAiQuick, aiQuickDlg, [ aiQuickDlg ] {
        if (aiQuickDlg)
            aiQuickDlg->close();
    });

    aiQuickDlg->show();
}

void Application::launchAiWriterDialog()
{
    qCInfo(logMain) << "Launching AI writer dialog";
    AiWriterDialog::instance().show();
}

void Application::initWordWizard(WordWizard *wizard)
{
    qCDebug(logMain) << "Initializing word wizard";
    m_wordWizard = wizard;
    if (m_wordWizard) {
        connect(m_wordWizard, &WordWizard::sigToLaunchAiQuick, this, &Application::onLaunchAiQuick, Qt::UniqueConnection);
        connect(m_wordWizard, &WordWizard::sigToLaunchMgmt, this, &Application::onLaunchMgmt, Qt::UniqueConnection);
    }
}

int Application::handleExistingArgument(int argc, char *argv[])
{
    qCDebug(logMain) << "Handling existing arguments";
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << argv[i];
    }

    QCommandLineParser parser;
    parser.addOptions(options);
    parser.parse(arguments);

    if (parser.isSet("chat")) {
        qCInfo(logMain) << "Processing --chat argument";
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage", ChatIndex::Text).errorMessage();
        if (!error.isEmpty()) {
            qCCritical(logMain) << "Failed to launch chat page via D-Bus:" << error;
            return -1;
        }
    } else if (parser.isSet("talk")) {
        qInfo() << "Detects that an application already exists and will notify it of its launch chat window for talk";
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage", ChatIndex::Talk).errorMessage();
        if (!error.isEmpty()) {
            qCritical() << error;
            return -1;
        }
    } else if (parser.isSet("wordwizard")) {
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchWordWizard").errorMessage();
        if (!error.isEmpty()) {
            qCritical() << error;
            return -1;
        }
    } else if (parser.isSet("screenshot")) {
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        qCInfo(logMain) << "launch chat window for screenshot";
        QString error = notification.call(QDBus::Block, "startScreenshot").errorMessage();
        if (!error.isEmpty()) {
            qCritical() << error;
            return -1;
        }
    }

    return 0;
}

bool Application::handleWordWizardArgument(int argc, char *argv[])
{
    qCDebug(logMain) << "Checking for word wizard argument";
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << argv[i];
    }

    QCommandLineParser parser;
    parser.addOptions(options);
    parser.parse(arguments);

    bool result = parser.isSet("wordwizard");
    qCDebug(logMain) << "Word wizard argument present:" << result;
    return result;
}

void Application::onLaunchMgmt(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString &locateTitle)
{
    launchMgmtWindow(showAddllmPage, onlyUseAgreement, isFromAiQuick, locateTitle);
}

void Application::onLaunchGetFreeAccountDlg()
{
    launchMgmtWindow(false);
    m_mgmtWindow->showGetFreeAccountDlg();
}

void Application::onLaunchMgmtNoShow()
{
    launchMgmtWindowNoShow();
}

void Application::onLaunchChat(int index)
{
    launchChatWindow(index);
}

void Application::onLaunchAbout()
{
    launchAboutWindow();
}

void Application::onLaunchWordWizard()
{
    launchWordWizard();
}

void Application::onLaunchAiQuick(int type, QString query, QPoint pos, bool isCustom)
{
    launchAiQuickDialog(type, query, pos, isCustom);
}

void Application::onLaunchAiQuickOCR(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath)
{
    launchAiQuickOCRDialog(type, query, pos, isCustom, imagePath);
}

void Application::onLaunchAiWriter()
{
    launchAiWriterDialog();
}

void Application::onInputPrompt(const QString &question, const QMap<QString, QString> &params)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Input prompt called from non-main thread";
        return;
    }

    if (!WelcomeDialog::isAgreed()) {
        qCInfo(logMain) << "Showing welcome dialog for input prompt";
        WelcomeDialog::instance(false)->exec();
        return;
    }

    // 避免：窗口模式下，连续点击划词icon使得主窗口隐藏
    if (m_chatWindow == nullptr || !m_chatWindow->isActiveWindow()) {
        qCInfo(logMain) << "Launching chat window for input prompt";
        launchChatWindow(ChatIndex::Text);
        m_chatWindow->show();
    }
    qCDebug(logMain) << "Processing input prompt size:" << question.size();
    m_chatWindow->overrideQuestion(question, params);
}

void Application::onAppendPrompt(const QString &question)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Append prompt called from non-main thread";
        return;
    }

    // 避免：窗口模式下，连续点击划词icon使得主窗口隐藏
    if (m_chatWindow == nullptr || !m_chatWindow->isActiveWindow()) {
        qCInfo(logMain) << "Launching chat window for append prompt";
        launchChatWindow(ChatIndex::Text);
        if (m_chatWindow == nullptr) {
            qCWarning(logMain) << "Agreement is not accepted";
            return;
        }
        m_chatWindow->show();
    }
    qCDebug(logMain) << "Appending prompt size:" << question.size();
    m_chatWindow->appendQuestion(question);
}

bool Application::checkAgreement()
{
    if (WelcomeDialog::isAgreed())
        return true;

    if (QThread::currentThread() != qApp->thread()) {
        qWarning() << "called thread is not main thread. do not show WelcomeDialog.";
        return false;
    }

    WelcomeDialog::instance(false)->exec();
    return WelcomeDialog::isAgreed();
}

void Application::handleArgumentsParser(const QStringList &arguments)
{
    qCDebug(logMain) << "Parsing command line arguments";
    QCommandLineParser parser;
    parser.addOptions(options);
    parser.parse(arguments);

    if (parser.isSet("functions")) {
        qCInfo(logMain) << "Listing available functions";
        bool notYetQueried = true;
        qCInfo(logMain) << FunctionHandler::queryAppFunctions(notYetQueried);
    }

    if (parser.isSet("chat")) {
        qCInfo(logMain) << "Launching chat window from command line";
        launchChatWindow(ChatIndex::Text);
    } else if (parser.isSet("talk")) {
        qCInfo(logMain) << "Launching talk window from command line";
        launchChatWindow(ChatIndex::Talk);
    } else if (parser.isSet("screenshot")) {
        qCInfo(logMain) << "Starting screenshot from command line";
        onStartScreenshot();
    }
}

void Application::onAddKnowledgeBasefile(const QStringList &knowledgeBasefile)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Add knowledge base called from non-main thread";
        return;
    }

    if (!WelcomeDialog::isAgreed()) {
        qCInfo(logMain) << "Showing welcome dialog for knowledge base addition";
        WelcomeDialog::instance(false)->exec();
        return;
    }

    if (m_chatWindow == nullptr || !m_chatWindow->isVisible()) {
        qCInfo(logMain) << "Launching chat window for knowledge base addition";
        launchChatWindow(ChatIndex::Text);
        m_chatWindow->show();
    }
    qCInfo(logMain) << "Adding knowledge base files:" << knowledgeBasefile;
    m_chatWindow->addKnowledgeBase(knowledgeBasefile);
}

void Application::onTranslate()
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "Cannot translate - user agreement not accepted";
        return;
    }

    if (!m_wordWizard) {
        qCWarning(logMain) << "Cannot translate - word wizard not initialized";
        return;
    }
    qCInfo(logMain) << "Executing translation via shortcut";
    m_wordWizard->onShortcutTranslate();
}

void Application::onSignalShowMgmtWindowAfterChatInitFinished()
{
    ChatWindow::setNeedShowLLMConfigWindow(true);
}

void Application::onStartScreenshot()
{
    if (!m_chatWindow) {
        qCWarning(logMain) << "Chat window is not excited";
        return;
    }
    qCInfo(logMain) << "Executing screenshot via shortcut";
    // 调用EAiExecutor的startScreenshot方法
    if (m_chatWindow) {
        m_chatWindow->startScreenshot();
    }
}

void Application::onUploadImage(const QString &imagePath)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Upload image called from non-main thread";
        return;
    }

    if (m_chatWindow == nullptr) {
        qCInfo(logMain) << "Launching chat window for upload image";
        launchChatWindow(ChatIndex::Text);
        if (m_chatWindow == nullptr) {
            qCWarning(logMain) << "Agreement is not accepted";
            return;
        }
    } else {
        m_chatWindow->onlyShowWindow();
    }

    m_chatWindow->appendImage(imagePath);
}

void Application::onIconThemeChanged()
{
    if (m_chatWindow) {
        m_chatWindow->iconThemeChanged();
    }
}
