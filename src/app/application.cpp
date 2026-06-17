#include "application.h"
#include "serverwrapper.h"
#include "localmodelserver.h"
#include "wordwizard/gui/aiquickdialog.h"
#include "wordwizard/gui/aiwriterdialog.h"
#include "wordwizard/wordwizard.h"
#include "private/welcomedialog.h"
#include "esystemcontext.h"
#include "dbus/dbusinterface.h"

#ifdef QT_DEBUG
#include "tas/uosaccountencoder.h"
#include "model/builtinprovider.h"
#endif

#include "mcp/mcpconfigsyncer.h"
#include "global_define.h"
#include "dbus/shortcutmanager.h"

#include <report/chatwindowpoint.h>
#include <report/eventlogutil.h>

// 3.0
#include "gui/window/windowmanager.h"
#include "gui/web/webcontext.h"
#include "gui/web/taskchannel.h"
#include "chatbot/chatbotservice.h"

#include <QtDBus>
#include <QCommandLineParser>
#include <QLoggingCategory>

#include <iostream>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logMain)

using namespace uos_ai;

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
#ifdef QT_DEBUG
    ,
    QCommandLineOption("enc-model", "Encrypt external model json file", "file")
#endif
};

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    initApplicationIconName();
    qCInfo(logMain) << "Initializing UOS AI Application" << DApplication::buildVersion(APP_VERSION);
    setApplicationName(APP_NAME_KEY);
    setObjectName(APP_NAME_KEY);
    setOrganizationName("deepin");
    loadTranslator();
    setApplicationDisplayName(QCoreApplication::translate("uos_ai::AssistantManager", "UOS AI"));
    setApplicationVersion(DApplication::buildVersion(APP_VERSION));
    setProductName(QCoreApplication::translate("uos_ai::AssistantManager", "UOS AI"));
    setProductIcon(QIcon::fromTheme(getApplicationIconName()));
    setApplicationDescription(tr("UOS AI is a desktop smart assistant, your personal assistant! You can communicate with it using text or voice, and it can help answer questions, provide information, and generate images based on your descriptions."));
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setQuitOnLastWindowClosed(false);
    setWindowIcon(QIcon::fromTheme(getApplicationIconName()));
}

Application::~Application()
{
    if (m_mgmtWindow)
        m_mgmtWindow->deleteLater();
}

void Application::initialization()
{
    qCDebug(logMain) << "Starting application initialization";
    ServerWrapper::instance()->initialization();

    setAttribute(Qt::AA_UseHighDpiPixmaps);
    initShortcut();
    handleArgumentsParser(arguments());

    McpConfigSyncer::instance()->fetchConfigFromServerAsync();

#ifdef ENABLE_CHATBOT
    chatbot::ChatBotService::instance()->initialize();
#endif
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
    int isUpdate = 1;

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

void Application::initMgmtWindow()
{
    if (!m_mgmtWindow) {
        qCInfo(logMain) << "Creating new management window";
        m_mgmtWindow = new MgmtWindow();
        if (m_wordWizard) {
            bool isHidden = m_wordWizard->queryHiddenStatus();
            qCInfo(logMain) << "wordWizard hiddenStatus" << isHidden;
            m_mgmtWindow->onWordWizardHiddenStatus(isHidden);
            connect(m_wordWizard, &WordWizard::signalHiddenwidget, m_mgmtWindow, &MgmtWindow::onWordWizardHiddenStatus, Qt::UniqueConnection);
            connect(m_wordWizard, &WordWizard::signalAddDisabledApp, m_mgmtWindow, &MgmtWindow::onAddDisabledApp, Qt::UniqueConnection);
            connect(m_mgmtWindow, &MgmtWindow::signalWordWizardStatusChanged, m_wordWizard, &WordWizard::onChangeHiddenStatus, Qt::UniqueConnection);
            connect(m_mgmtWindow, &MgmtWindow::signalDisabledAppsUpdated, m_wordWizard, &WordWizard::updateDisabledApps, Qt::UniqueConnection);
        }
        if (WmIns() && WmIns()->context() && WmIns()->context()->taskCh) {
            WmIns()->context()->taskCh->setMgmtWindow(m_mgmtWindow);
            qCInfo(logMain) << "Set MgmtWindow pointer to TaskChannel";
        }
    }
}

void Application::showConfig(int page)
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "User agreement not accepted, cannot launch chat window";
        return;
    }

    initMgmtWindow();
    m_mgmtWindow->showPage(page);
}

void Application::showChatWindow()
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "User agreement not accepted, cannot launch chat window";
        return;
    }

    WmIns()->showWindow(WmIns()->currentMode());
}

void Application::launchChatWindowWithToken(int index, const QString &token)
{
    if (!token.isEmpty()) {
        qCInfo(logMain) << "Applying activation token before launching chat window";
        qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }

    launchChatWindow(index);
}

void Application::launchChatWindow(int index)
{
    if (!checkAgreement()) {
        qCWarning(logMain) << "User agreement not accepted, cannot launch chat window";
        return;
    }
    qCInfo(logMain) << "Launching chat window with index:" << index;

    if (index == 1) {
        if (!WmIns()->isWindowExist() || !WmIns()->isActiveWindow()) {
            qCInfo(logMain) << "Launching chat window for DigitalMode";
            WmIns()->showWindow(WmIns()->currentMode());
        }
        if (WmIns()->context() && WmIns()->context()->taskCh)
            WmIns()->context()->taskCh->changeToDigitalMode();
    } else {
        if (WmIns()->isWindowExist() && WmIns()->isActiveWindow()) {
            WmIns()->hideWindow();
        } else {
            WmIns()->showWindow(WmIns()->currentMode());
        }
    }

    initMgmtWindow();
    if (!m_mgmtWindow->isVisible())
        m_mgmtWindow->checkUpdateStatus();
    // tid:1001600006 event:chatwindow
    ReportIns()->writeEvent(report::ChatwindowPoint().assemblingData());
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

void Application::launchAiQuick(int type, QString query, QPoint pos, bool isCustom, QString imagePath)
{
    if (!WelcomeDialog::isAgreed()) {
        qCInfo(logMain) << "Showing welcome dialog for input prompt";
        WelcomeDialog::instance(false)->exec();
        return;
    }

    qCInfo(logMain) << "Launching AI quick dialog with type:" << type << "query size:" << query.size();
    AiQuickDialog *aiQuickDlg = new AiQuickDialog(m_wordWizard);
    aiQuickDlg->setQuery(type, query, pos, isCustom, imagePath);

    connect(m_wordWizard, &WordWizard::sigToCloseAiQuick, aiQuickDlg, &AiQuickDialog::deleteLater);

    aiQuickDlg->show();
}

void Application::initWordWizard(WordWizard *wizard)
{
    qCDebug(logMain) << "Initializing word wizard";
    m_wordWizard = wizard;
    if (m_wordWizard) {
        connect(m_wordWizard, &WordWizard::sigToLaunchAiQuick, this, &Application::launchAiQuick, Qt::UniqueConnection);
        connect(m_wordWizard, &WordWizard::sigToLaunchMgmt, this, &Application::showConfig, Qt::UniqueConnection);
    }
}

void Application::handleAboutAction()
{
    WmIns()->showAboutWindow();
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
        QString error = notification.call(QDBus::Block, "launchChatPage", 0).errorMessage();
        if (!error.isEmpty()) {
            qCCritical(logMain) << "Failed to launch chat page via D-Bus:" << error;
            return -1;
        }
    } else if (parser.isSet("talk")) {
        qInfo() << "Detects that an application already exists and will notify it of its launch chat window for talk";
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage", 1).errorMessage();
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

void Application::inputPrompt(const QString &question, const QMap<QString, QString> &params)
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
    if (!WmIns()->isWindowExist() || !WmIns()->isActiveWindow()) {
        qCInfo(logMain) << "Launching chat window for input prompt";
        WmIns()->showWindow(WmIns()->currentMode());
    }

    qCDebug(logMain) << "Processing input prompt size:" << question.size();
    WmIns()->context()->taskCh->overrideQuestion(question, params);
}

void Application::appendPrompt(const QString &question, bool isSend)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Append prompt called from non-main thread";
        return;
    }


    // 避免：窗口模式下，连续点击划词icon使得主窗口隐藏
    if (!WmIns()->isWindowExist() || !WmIns()->isActiveWindow()) {
         WmIns()->showWindow(WmIns()->currentMode());
    }

    qCDebug(logMain) << "Appending prompt size:" << question.size();
    WmIns()->context()->taskCh->appendPrompt(question, isSend);
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
    }

    if (parser.isSet("chat")) {
        qCInfo(logMain) << "Launching chat window from command line";
        launchChatWindow(0);
    } else if (parser.isSet("talk")) {
        qCInfo(logMain) << "Launching talk window from command line";
        launchChatWindow(1);
    } else if (parser.isSet("screenshot")) {
        qCInfo(logMain) << "Starting screenshot from command line";
        startScreenshot();
    }
#ifdef QT_DEBUG
    else if (parser.isSet("enc-model")) {
        QString path = parser.value("enc-model");
        QFile f(path);
        if (!f.open(QFile::ReadOnly)) {
            std::cerr << "Failed to open file:" << path.toStdString() << std::endl;
            _Exit(1);
            return;
        }

        UosAccountEncoder encoder(UOS_EXTERNAL_MODEL_PASSWD);
        QString value = encoder.encrypt(QString::fromUtf8(f.readAll()), "1", 0);
        std::cout << value.toStdString() << std::endl;
        _Exit(0);
    }
#endif
}

void Application::addKnowledgeBasefile(const QStringList &knowledgeBasefile)
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

    if (!WmIns()->isWindowExist() || !WmIns()->isActiveWindow()) {
         WmIns()->showWindow(WmIns()->currentMode());
    }

    qCInfo(logMain) << "Adding knowledge base files to TaskChannel queue:" << knowledgeBasefile;
    initMgmtWindow();
    WmIns()->context()->taskCh->addKnowledgeBase(knowledgeBasefile);
}

void Application::showTranslate()
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

void Application::startScreenshot()
{
    if (!WmIns()->isWindowExist()) {
        qCWarning(logMain) << "Chat window is not excited";
        return;
    }
    qCInfo(logMain) << "Executing screenshot via shortcut";
    // 调用WmIns()的startScreenshot方法
    if (WmIns()->isWindowExist()) {
        WmIns()->context()->fileCh->startScreenshot();
    }
}

void Application::uploadImage(const QString &imagePath)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logMain) << "Upload image called from non-main thread";
        return;
    }

    //外部传入图片时，ai窗口在上面展示，方便用户查看
    if (!WmIns()->isWindowExist() || !WmIns()->isActiveWindow()) {
         WmIns()->showWindow(WmIns()->currentMode());
    }
    WmIns()->context()->taskCh->appendImage(imagePath);
}
