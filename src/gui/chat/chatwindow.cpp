#include "chatwindow.h"
#include "private/esinglewebview.h"
#include "private/eaiexecutor.h"
#include "private/echatwndmanager.h"
#include "dbwrapper.h"
#include "private/echatbutton.h"
#include "serverwrapper.h"
//#include "utils/util.h"

#include <QDesktopWidget>
#include <QBoxLayout>
#include <QDBusInterface>
#include <QPalette>
#include <QTimer>
#include <QScreen>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QDir>
#include <QAction>
#include <QStackedLayout>
#include <QApplication>
#include <QMenu>
#include <QResizeEvent>

#include <DLabel>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DAboutDialog>
#include <DWindowOptionButton>
#include <DWidget>
#include <DWidgetUtil>
#include <DWindowManagerHelper>

#include <KGlobalAccel>

#ifdef COMPILE_ON_V23
#include "dbus-private/dbusdisplay1.h"
#include "dbus-private/dbusmonitor1.h"
#include "dbus-private/dbusdock1.h"
#else
#include <com_deepin_daemon_display_monitor.h>
#include <com_deepin_daemon_display.h>
#include "ddedockobject.h"
#include <com_deepin_dde_daemon_dock.h>
#endif

#include <com_deepin_daemon_appearance.h>

DWIDGET_USE_NAMESPACE

#ifdef COMPILE_ON_V23
const QString AppearanceService = QStringLiteral("org.deepin.dde.Appearance1");
const QString AppearancePath = QStringLiteral("/org/deepin/dde/Appearance1");
const QString AppearanceInterface = QStringLiteral("org.deepin.dde.Appearance1");
#else
const QString AppearanceService = QStringLiteral("com.deepin.daemon.Appearance");
const QString AppearancePath = QStringLiteral("/com/deepin/daemon/Appearance");
const QString AppearanceInterface = QStringLiteral("com.deepin.daemon.Appearance");
#endif

static constexpr char appIcon[] = "uos-ai-assistant";

ChatWindow::ChatWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_background(new QWidget(this))
    , m_webView(new ESingleWebView(this))
    , m_gloablShortcut(new QAction(this))
    , m_appearance(new __Appearance("org.deepin.dde.Appearance1", "/org/deepin/dde/Appearance1", QDBusConnection::sessionBus(), this))
    , m_appearanceInter(new QDBusInterface(AppearanceService, AppearancePath, AppearanceInterface, QDBusConnection::sessionBus(), this))
{
    m_wWindowFlags = windowFlags();
    m_sWindowFlags = /*Qt::FramelessWindowHint |*/ Qt::Tool | Qt::MSWindowsFixedSizeDialogHint;

    if (windowHandle())
        windowHandle()->setProperty("_d_dwayland_window-type", "standAlone");

#ifdef COMPILE_ON_V23
    m_display = new DBusDisplay();
    m_dock = new DBusDock(this);
#else
    m_display = new __Display("com.deepin.daemon.Display", "/com/deepin/daemon/Display", QDBusConnection::sessionBus(), this);
    m_dock = new DDeDockObject(this);
#endif

    m_hasBlurWindow = DWindowManagerHelper::instance()->hasBlurWindow();

    if (DbWrapper::localDbWrapper().getDisplayMode() == 1) {
        m_displayMode = WINDOW_MODE;
        m_webView->setWindowMode(true);
    }
    QStringList sizeList = DbWrapper::localDbWrapper().getWindowSize().split("#");
    if (sizeList.size() == 2) {
        int width = sizeList[0].toInt();
        int height = sizeList[1].toInt();
        if (width > 0 && height > 0)
            m_windowSize = QSize(width, height);
    }

    initTitlebar();

    QFontInfo fontInfo = this->fontInfo();
    m_webView->updateFont(fontInfo.family(), fontInfo.pixelSize());
    m_webView->setAccessibleName("webview");
    m_webView->page()->setBackgroundColor(Qt::transparent);
    m_webView->setUrl(QUrl("qrc:/assets/web/front/dist/index.html"));

    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(m_webView);
    m_stackedWidget->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *backLayout = new QVBoxLayout;
    backLayout->setMargin(0);
    backLayout->addWidget(m_stackedWidget);
    m_background->setLayout(backLayout);

    setCentralWidget(m_background);

    m_appearanceInter->setTimeout(100);
    m_alpha = qvariant_cast<double>(m_appearanceInter->property("Opacity"));

    //All shortcut should be register after the cleanup.
    m_gloablShortcut->setObjectName(QString::fromUtf8("uos-ai-record"));
    connect(m_gloablShortcut, &QAction::triggered, this, &ChatWindow::onAudioRecShortcutPressed);
    KGlobalAccel::self()->setShortcut(m_gloablShortcut, QList<QKeySequence>() << QKeySequence("Meta+R"));

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ChatWindow::onSystemThemeChanged);

#ifdef COMPILE_ON_V23
    connect(m_dock, &DBusDock::FrontendWindowRectChanged, this, &ChatWindow::onScreenOrDockChanged);
#else
    connect(m_dock, &DDeDockObject::FrontendWindowRectChanged, this, &ChatWindow::onScreenOrDockChanged);
#endif

    updateSystemTheme();
    EAiExec()->setChatWindow(this);

    installEventFilter(this);

    connect(EWndManager(), &EChatWndManager::modalStateChanged, this, &ChatWindow::onModalStateChanged);

    connect(m_webView, &ESingleWebView::voiceConversationStatusChanged, this, &ChatWindow::onVoiceConversationStatusChanged);

    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasBlurWindowChanged, this, [=](){
        // qDebug() << "hasBlurWindowChanged: " << DWindowManagerHelper::instance()->hasBlurWindow();
        m_hasBlurWindow = DWindowManagerHelper::instance()->hasBlurWindow();
        updateSystemTheme();
    });

    QDBusConnection session = QDBusConnection::sessionBus();
    session.connect(AppearanceService, AppearancePath, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    this, SLOT(appearancePropertiesChanged(QString, QVariantMap, QStringList)));
}

void ChatWindow::appearancePropertiesChanged(QString interface, QVariantMap changedProperties, QStringList)
{
    if (interface != AppearanceInterface)
        return;

    for (auto iter = changedProperties.begin(); iter != changedProperties.end(); iter++) {
        if (iter.key() == "Opacity") {
            m_alpha = qdbus_cast<double>(iter.value());
            updateSystemTheme();
        }
    }
}

void ChatWindow::initTitlebar()
{
    QMenu *titleMenu = new QMenu(this);
    QMenu *m_displayModeMenu = new QMenu(tr("Mode"));
    QActionGroup *m_displayModeActionGroup = new QActionGroup(m_displayModeMenu);
    m_windowModeAction = new QAction(tr("Window Mode"));
    m_windowModeAction->setActionGroup(m_displayModeActionGroup);
    m_displayModeMenu->addAction(m_windowModeAction);
    m_windowModeAction->setCheckable(true);

    m_sidebarModeAction = new QAction(tr("Sidebar Mode"));
    m_sidebarModeAction->setActionGroup(m_displayModeActionGroup);
    m_displayModeMenu->addAction(m_sidebarModeAction);
    m_sidebarModeAction->setCheckable(true);

    m_settingsAction = new QAction(tr("Settings"));
    titleMenu->addMenu(m_displayModeMenu);
    titleMenu->addAction(m_settingsAction);
    titleMenu->addSeparator();

    if (m_displayMode == WINDOW_MODE) {
        m_windowModeAction->setChecked(true);
    } else {
        m_sidebarModeAction->setChecked(true);
    }

    titlebar()->setIcon(QIcon::fromTheme("uos-ai-assistant"));
    titlebar()->setMenu(titleMenu);
    titlebar()->setSwitchThemeMenuVisible(false);
    titlebar()->setQuitMenuVisible(false);
    titlebar()->setBackgroundTransparent(false);

    m_chatBtn = new EChatButton(this);
    m_chatBtn->setToolTip(tr("Voice conversation"));
    //m_chatBtn->setIcon(QIcon(QString(":/icons/deepin/builtin/dark/icons/chat.svg")));
    m_chatBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_chatBtn->setIconSize(QSize(titlebar()->height(), titlebar()->height()));
    titlebar()->addWidget(m_chatBtn, Qt::AlignRight);

    connect(m_chatBtn, &DIconButton::clicked, this, &ChatWindow::onChatBtnClicked);
    connect(titleMenu, &QMenu::triggered, this, &ChatWindow::onMenuTriggered);
#ifdef DTKWIDGET_CLASS_DSizeMode
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [=](DGuiApplicationHelper::SizeMode sizeMode){
        m_chatBtn->setIconSize(QSize(titlebar()->height(), titlebar()->height()));
    });
#endif
}

void ChatWindow::showWindowMode()
{
    setWindowIconVisible(true);
    if (m_hasChatHistory) {
        setWindowTitleVisible(true);
    } else {
        setWindowTitleVisible(false);
    }
    setTitlebarShadowEnabled(true);
    //setAttribute(Qt::WA_TranslucentBackground, false);
    //setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(m_wWindowFlags);
    setFixedHeight(m_windowSize.height());
    resize(m_windowSize);
    setMinimumSize(680, 300);
    setMaximumSize(4096, 4096);
    activeShowWindow();
    Dtk::Widget::moveToCenter(this);
    updateSystemTheme();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    DbWrapper::localDbWrapper().updateDisplayMode(WINDOW_MODE);
}

void ChatWindow::showSidebarMode()
{
    if (m_hasChatHistory) {
        setWindowIconVisible(true);
        setWindowTitleVisible(true);
    } else {
        setWindowIconVisible(false);
        setWindowTitleVisible(false);
    }
    setTitlebarShadowEnabled(false);
    setWindowFlags(m_sWindowFlags);
    setEnableBlurWindow(true);
    setAttribute(Qt::WA_TranslucentBackground);
    showNormal();
    updateSidebarGeometry();
    updateSystemTheme();
    activeShowWindow();

    DbWrapper::localDbWrapper().updateDisplayMode(SIDEBAR_MODE);
}

void ChatWindow::setW(int w)
{
    setFixedWidth(w);
    move(m_geometry.x() + DefaultMargin + m_geometry.width() - w, m_geometry.y());
}

void ChatWindow::setX(int x)
{
    move(x, m_geometry.y());
}

void ChatWindow::activeShowWindow()
{
    QTimer::singleShot(0, [this]() {
        if (m_isMaximized && m_displayMode == WINDOW_MODE)
            showMaximized();
        else
            showNormal();

        activateWindow();
    });
}

/*void ChatWindow::onAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState);

    //when the animation is finished
    if (newState == QAbstractAnimation::Stopped && oldState == QAbstractAnimation::Running) {
        if (m_animationGroup->direction() == QAbstractAnimation::Backward) {
            //when the return animation is finished
            //hides itself,is used to determine whether the window is displayed or not
            this->hide();
            return;
        } else {
            //when the forward animation is finished
            //Remove fixedsize effect, allow drag and drop
            setMaximumWidth(m_displayRect.width() / 2 - DefaultMargin);
        }
    }

    //when the animation starts
    if (newState == QAbstractAnimation::Running && oldState == QAbstractAnimation::Stopped) {
        if (m_animationGroup->direction() == QAbstractAnimation::Forward) {
            //flickering problem: hide web content when the window comes out to solve the flickering problem
            m_stackedWidget->hide();
            return;
        } else if (m_animationGroup->direction() == QAbstractAnimation::Backward) {
            //flickering problem: capture the current interface to display as web content to solve the flickering problem
            QPixmap pixmap(size());
            render(&pixmap);
        }
    }

    //the start and end of the animation need to be executed
    if (m_stackedWidget->isHidden()) {
        m_stackedWidget->show();
    }
    m_stackedWidget->setCurrentIndex(newState == QAbstractAnimation::Running);
}*/

void ChatWindow::onModalStateChanged(bool modal)
{
    qInfo() << "ChatWindow::onModalStateChanged->" << modal;

    //Only need notify WebView when chat
    //window is visible, beacuase dialog
    //may popup seperately.
    if (isVisible()) {
        m_webView->setModalState(modal);
    }
}

void ChatWindow::onMenuTriggered(QAction *action)
{
    if (action == m_windowModeAction) {
        m_windowModeAction->setChecked(true);
        m_displayMode = WINDOW_MODE;

        m_webView->setWindowMode(true);
        ServerWrapper::instance()->updateVisibleState(false);
        showWindowMode();
    } else if (action == m_sidebarModeAction) {
        m_sidebarModeAction->setChecked(true);
        m_displayMode = SIDEBAR_MODE;

        m_webView->setWindowMode(false);
        ServerWrapper::instance()->updateVisibleState(true);
        showSidebarMode();
    } else if(action == m_settingsAction) {
        EAiExec()->launchLLMConfigWindow(false, AssistantType::PLUGIN_ASSISTANT == EAiExec()->currentAssistantType());
    }
}

void ChatWindow::onChatBtnClicked()
{
    const QList<LLMServerProxy> &llmAccountLst = DbWrapper::localDbWrapper().queryLlmList();
    if (llmAccountLst.isEmpty()) {
        EAiExec()->launchLLMConfigWindow(true, AssistantType::PLUGIN_ASSISTANT == EAiExec()->currentAssistantType());
    } else {
        if (m_isDigitalMode) {
            m_webView->setToChatMode();
            if (m_displayMode == WINDOW_MODE)
                setWindowIconVisible(true);

            if (m_hasChatHistory) {
                setWindowIconVisible(true);
                setWindowTitleVisible(true);
            }

        } else {
            m_webView->setToDigitalMode();
            if (m_displayMode == SIDEBAR_MODE)
                setWindowIconVisible(false);
            setWindowTitleVisible(false);
        }

        m_isDigitalMode = !m_isDigitalMode;
        m_chatBtn->updateActiveStatus(m_isDigitalMode);
    }
}

void ChatWindow::onVoiceConversationStatusChanged(int status)
{
//    if (m_voiceConversationStatus == status)
//        return;

    m_voiceConversationStatus = status;

    switch (m_voiceConversationStatus) {
    case 2:
        titlebar()->setMenuDisabled(true);
        break;
    case 3:
    case 4:
        m_chatBtn->setDisabled(true);
        break;
    case 1:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    default:
        m_chatBtn->setDisabled(false);
        titlebar()->setMenuDisabled(false);
        break;

    }

}

void ChatWindow::onGenPersonalFAQ()
{
    EAiExec()->personalFAQGenerate();
}

void ChatWindow::updateSidebarGeometry(int currentWidth)
{
#ifdef COMPILE_ON_V23
    DockPosition dockPosition = static_cast<DockPosition>(m_dock->position());
    int mode = 0;
#else
    DockPosition dockPosition = static_cast<DockPosition>(m_dock->position());
    int mode = m_dock->displayMode();
#endif
    int dockMargin = 10;
    qreal ratio = qApp->primaryScreen()->devicePixelRatio();

#ifdef COMPILE_ON_V23
    QList<QDBusObjectPath> screenList = m_display->monitors();
#else
    QList<QDBusObjectPath> screenList = m_display->monitors();
#endif

#ifdef COMPILE_ON_V23
    MyDockRect dockrect1 = m_dock->frontendWindowRect();
    QRect frontendWindowRect = dockrect1.operator QRect();
    if (ratio != 1.0)
        frontendWindowRect = QRect(frontendWindowRect.x(), frontendWindowRect.y()
                        , static_cast<int>(frontendWindowRect.width() / ratio)
                        , static_cast<int>(frontendWindowRect.height() / ratio));
#else
    QRect frontendWindowRect = m_dock->frontendWindowRect();
#endif


    m_displayRect = m_display->primaryRect();
    for (const QDBusObjectPath &screen : screenList) {
#ifdef COMPILE_ON_V23
        DBusMonitor monitor(screen.path());
#else
        __Monitor monitor("com.deepin.daemon.Display", screen.path(), QDBusConnection::sessionBus());
#endif
        QRect monitorRect(monitor.x(), monitor.y(), monitor.width(), monitor.height());
        if (monitor.enabled() && monitorRect.contains(frontendWindowRect.center())) {
            m_displayRect = QRect(monitorRect.x(), monitorRect.y(),
                                  monitorRect.width() / ratio, monitorRect.height() / ratio);
            break;
        }
    }

    int width = currentWidth;
    int height = m_displayRect.height() - DefaultMargin * 2;

    if (dockPosition == DockPosition::Top || dockPosition == DockPosition::Bottom) {
        if (mode == DockModel::Fashion) {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio - dockMargin;
        } else {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio;
        }
    }

    int x = m_displayRect.x() + m_displayRect.width() - currentWidth - DefaultMargin;
    if (dockPosition == DockPosition::Right) {
        if (mode == DockModel::Fashion) {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + dockMargin + DefaultMargin);
        } else {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + DefaultMargin);
        }
    }

    int y = m_displayRect.y() + DefaultMargin;
    if (dockPosition == DockPosition::Top) {
        if (mode == DockModel::Fashion) {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio + dockMargin;
        } else {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio;
        }
    }

    m_geometry = QRect(x, y, width, height);
    setFixedHeight(height);
    setMaximumWidth(m_displayRect.width() / 2 - DefaultMargin);
    setMinimumWidth(400);
    setGeometry(m_geometry);
}

void ChatWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_displayMode == SIDEBAR_MODE) {
        Q_UNUSED(event);
        return;
    } else {
        return DMainWindow::mouseMoveEvent(event);
    }
}

void ChatWindow::hideEvent(QHideEvent *event)
{
    m_webView->setTohiddenState();

    if (m_displayMode == SIDEBAR_MODE)
        ServerWrapper::instance()->updateVisibleState(false);

    return DMainWindow::hideEvent(event);
}

void ChatWindow::showEvent(QShowEvent *event)
{
    m_webView->setToShowedState();

    if (m_displayMode == SIDEBAR_MODE)
        ServerWrapper::instance()->updateVisibleState(true);

    return DMainWindow::showEvent(event);
}

void ChatWindow::resizeEvent(QResizeEvent *event)
{
    if (m_displayMode == WINDOW_MODE) {
        m_isMaximized = isMaximized();
        if (!isMaximized()) {
            m_windowSize = event->size();
            QString sizeStr = QString::number(m_windowSize.width()) + "#" + QString::number(m_windowSize.height());
            DbWrapper::localDbWrapper().updateWindowSize(sizeStr);
        }
    }
//    qDebug() << "resize event: " << event->size();
    return DMainWindow::resizeEvent(event);
}

bool ChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    switch (event->type()) {
    case QEvent::WindowActivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(true);
        }
        if (m_displayMode == SIDEBAR_MODE)
            ServerWrapper::instance()->updateActiveState(true);
    } break;
    case QEvent::WindowDeactivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(false);
        }
        if (m_displayMode == SIDEBAR_MODE)
            ServerWrapper::instance()->updateActiveState(false);
    } break;
    default:
        break;
    };
    return DMainWindow::eventFilter(obj, event);
}

void ChatWindow::updateSystemTheme()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    QPalette pa = titlebar()->palette();
    QColor background(248, 248, 248, 255);
    QColor background_dark(37, 37, 37, 255);
    QString styleSheet = QString("background-color:rgba(248, 248, 248, 255)");
    QString styleSheet_dark = QString("background-color:rgba(37, 37, 37, 255)");

    if (m_displayMode == SIDEBAR_MODE && m_hasBlurWindow) {
        int alpha = m_alpha * 255;
        if (alpha < 0 || alpha > 255 )
            alpha = 160;

        background = QColor(255, 255, 255, alpha);
        background_dark = QColor(0, 0, 0, alpha);
        styleSheet = QString("background-color:rgba(255, 255, 255, %1)").arg(alpha);
        styleSheet_dark = QString("background-color:rgba(0, 0, 0, %1)").arg(alpha);
    }

    if (themeType == DGuiApplicationHelper::DarkType) {
        m_background->setStyleSheet(styleSheet_dark);
        pa.setColor(QPalette::Base, background_dark);
    } else {
        m_background->setStyleSheet(styleSheet);
        pa.setColor(QPalette::Base, background);

    }
    titlebar()->setPalette(pa);
}

void ChatWindow::showToast(const QString &message)
{
    DFloatingMessage *floatMessage = new DFloatingMessage(DFloatingMessage::TransientType, this);
    floatMessage->setMessage(message);

    QRect geometry(QPoint(10, 10), floatMessage->sizeHint());
    geometry.moveCenter(rect().center());

    //Show the message at 1/3 from window's bottom
    geometry.moveBottom(rect().bottom() - (rect().height() / 3));
    floatMessage->setGeometry(geometry);
    floatMessage->show();
}

void ChatWindow::onSystemThemeChanged()
{
    updateSystemTheme();
}

void ChatWindow::onAudioRecShortcutPressed()
{
    //TODO:
    //Modified 2023-12-6
    // V1.1.3 原有的Super + R快捷键已取消
    // So just shadow the function code, maybe
    //need revert.

    //onlyShowWidget();
    //m_webView->changeAudioRecState();
}

void ChatWindow::onScreenOrDockChanged()
{
    //keep the width
    if(m_displayMode == SIDEBAR_MODE)
        updateSidebarGeometry(this->width());
}

void ChatWindow::showWindow(ChatIndex index)
{
    if (ChatIndex::Talk == index) {
        //Requirement: if there is a modal window, do not do the switch and display the talk window
        if (QApplication::activeModalWidget()) {
            return;
        }

        const QList<LLMServerProxy> &llmAccountLst = DbWrapper::localDbWrapper().queryLlmList();
        if (llmAccountLst.isEmpty()) {
            EAiExec()->launchLLMConfigWindow(true, AssistantType::PLUGIN_ASSISTANT == EAiExec()->currentAssistantType());
        } else {
            if (m_voiceConversationDisabled)
                return;

            m_webView->setToDigitalMode();
            m_isDigitalMode = true;
            m_chatBtn->updateActiveStatus(m_isDigitalMode);
            titlebar()->setMenuDisabled(true);
            if (m_displayMode == SIDEBAR_MODE)
                setWindowIconVisible(false);
            setWindowTitleVisible(false);
        }

    }

    if (isHidden()) {
        if (m_displayMode == SIDEBAR_MODE) {
            showSidebarMode();
            return;
        } else {
            showWindowMode();

            m_stackedWidget->show();
            m_stackedWidget->setCurrentIndex(0);
            return;
        }
    } else if (qApp->applicationState() != Qt::ApplicationActive) {
        onlyShowWidget();
        return;
    }

    if (ChatIndex::Talk != index) {
        if (m_displayMode == SIDEBAR_MODE)
            closeWindow();
        else
            showMinimized();
    }
}

void ChatWindow::closeWindow()
{
    if (!isHidden()) {
        if (m_displayMode == SIDEBAR_MODE) {
            updateSidebarGeometry(this->width());
            hide();
        } else {
            this->hide();
        }
    }
}

void ChatWindow::onlyShowWindow()
{
    if (isHidden()) {
        updateSidebarGeometry();
        activeShowWindow();
        return;
    } else if (!m_webView->isActiveWindow()) {
        onlyShowWidget();
        return;
    }
}

void ChatWindow::onlyShowWidget()
{
    if (isHidden()) {
        //Resume on redisplay
        updateSidebarGeometry();
    }

    activeShowWindow();
}

void ChatWindow::showAboutWindow()
{
    auto aboutDialog = this->findChild<DAboutDialog *>();
    if (aboutDialog) {
        aboutDialog->show();
        aboutDialog->activateWindow();
        return;
    }

    aboutDialog = new DAboutDialog(this);
    aboutDialog->setProductName("UOS AI");
    aboutDialog->setProductIcon(QIcon::fromTheme(appIcon));
    aboutDialog->setVersion(QApplication::applicationVersion());
    aboutDialog->setDescription(tr("UOS AI is a desktop smart assistant, your personal assistant! You can communicate with it using text or voice, and it can help answer questions, provide information, and generate images based on your descriptions."));

    aboutDialog->show();
}

void ChatWindow::setMenuDisabled(bool disabled)
{
    titlebar()->setMenuDisabled(disabled);
}
void ChatWindow::setVoiceConversationDisabled(bool disabled)
{
    m_chatBtn->setDisabled(disabled);
    m_voiceConversationDisabled = disabled;
}

void ChatWindow::setWindowTitleVisible(bool visible)
{
    if (visible)
        titlebar()->setTitle(EAiExec()->currentAssistantName());
    else
        titlebar()->setTitle("");
}

void ChatWindow::setWindowIconVisible(bool visible)
{
    if (visible)
        titlebar()->setIcon(QIcon::fromTheme("uos-ai-assistant"));
    else
        titlebar()->setIcon(QIcon());
}

void ChatWindow::setHasChatHistory(bool hasChatHistory)
{
    m_hasChatHistory = hasChatHistory;
    if(m_hasChatHistory) {
        setWindowTitleVisible(true);
        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(true);
    } else {
        setWindowTitleVisible(false);
        if (m_displayMode == SIDEBAR_MODE)
            setWindowIconVisible(false);
    }
}
