#include "application.h"
#include "serverwrapper.h"
#include "llmutils.h"
#include "functionhandler.h"
#include "chatwindow.h"

#include <QtDBus>
#include <QCommandLineParser>

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
    }
};

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    setApplicationName(APP_NAME_KEY);
    setObjectName(APP_NAME_KEY);
    setOrganizationName("deepin");
    loadTranslator();
    setApplicationVersion(DApplication::buildVersion(APP_VERSION));
    setProductName(tr("UOS AI"));
    setProductIcon(QIcon::fromTheme(APP_NAME_KEY));
    setApplicationDescription(QCoreApplication::translate("CopilotApplication", "An intelligent UOS AI"));
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setQuitOnLastWindowClosed(false);
    setWindowIcon(QIcon::fromTheme(APP_NAME_KEY));

    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchMgmt, this, &Application::onLaunchMgmt) ;
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchChat, this, &Application::onLaunchChat) ;
    connect(ServerWrapper::instance(), &ServerWrapper::sigToLaunchAbout, this, &Application::onLaunchAbout) ;
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
    ServerWrapper::instance()->initialization();

    setAttribute(Qt::AA_UseHighDpiPixmaps);

    QDBusInterface notification("com.deepin.daemon.Keybinding", "/com/deepin/daemon/Keybinding", "com.deepin.daemon.Keybinding", QDBusConnection::sessionBus());
    notification.call("AddCustomShortcut", "UOS AI", "/usr/bin/uos-ai-assistant --chat", "<Super>C");
    notification.call("AddCustomShortcut", "UOS AI Talk", "/usr/bin/uos-ai-assistant --talk", "<Control><Super>C");
    handleArgumentsParser(arguments());
}

void Application::launchMgmtWindow(bool showAddllmPage)
{
    if (!m_mgmtWindow) {
        m_mgmtWindow = new MgmtWindow();
    }

    m_mgmtWindow->showEx(showAddllmPage);
}

void Application::launchChatWindow(int index)
{
    if (!m_chatWindow) {
        m_chatWindow = new ChatWindow();
    }

    m_chatWindow->showWindow(static_cast<ChatIndex>(index));
}

void Application::launchAboutWindow()
{
    if (!m_chatWindow) return;

    m_chatWindow->showAboutWindow();
}

int Application::handleExistingArgument(int argc, char *argv[])
{
    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << argv[i];
    }

    QCommandLineParser parser;
    parser.addOptions(options);
    parser.parse(arguments);

    if (parser.isSet("chat")) {
        qInfo() << "Detects that an application already exists and will notify it of its launch chat window for text";
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER_INTERFACE, QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage", ChatIndex::Text).errorMessage();
        if (!error.isEmpty()) {
            qCritical() << error;
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
    }

    return 0;
}

void Application::onLaunchMgmt(bool showAddllmPage)
{
    launchMgmtWindow(showAddllmPage);
}

void Application::onLaunchChat(int index)
{
    launchChatWindow(index);
}

void Application::onLaunchAbout()
{
    launchAboutWindow();
}

void Application::handleArgumentsParser(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.addOptions(options);
    parser.parse(arguments);

    if (parser.isSet("functions")) {
        bool notYetQueried = true;
        qInfo() << FunctionHandler::queryAppFunctions(notYetQueried);
    }

    if (parser.isSet("chat")) {
        launchChatWindow(ChatIndex::Text);
    } else if (parser.isSet("talk")) {
        launchChatWindow(ChatIndex::Talk);
    }
}
