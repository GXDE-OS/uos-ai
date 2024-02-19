#include "chatwindow.h"
#include "private/esinglewebview.h"
#include "private/eaiexecutor.h"
#include "ddedockobject.h"
#include "private/echatwndmanager.h"

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

#include <DLabel>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DAboutDialog>

#include <KGlobalAccel>
#include <com_deepin_daemon_appearance.h>
#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_daemon_display_monitor.h>
#include <com_deepin_daemon_display.h>

DWIDGET_USE_NAMESPACE

static constexpr char appIcon[] = "uos-ai-assistant";

ChatWindow::ChatWindow(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_background(new QWidget(this))
    , m_fakeView(new QLabel(this))
    , m_webView(new ESingleWebView(this))
    , m_gloablShortcut(new QAction(this))
    , m_xAnimation(new QPropertyAnimation(this, "x"))
    , m_wAnimation(new QPropertyAnimation(this, "width"))
    , m_animationGroup(new QSequentialAnimationGroup(this))
    , m_appearance(new __Appearance("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", QDBusConnection::sessionBus(), this))
    , m_display(new __Display("com.deepin.daemon.Display", "/com/deepin/daemon/Display", QDBusConnection::sessionBus(), this))
    , m_dock(new DDeDockObject(this))
{
    m_webView->setAccessibleName("webview");
    m_webView->page()->setBackgroundColor(Qt::transparent);
    m_webView->setUrl(QUrl("qrc:/assets/web/front/dist/index.html"));

    m_wAnimation->setEasingCurve(QEasingCurve::Linear);
    m_wAnimation->setDuration(AnimationTime);

    m_xAnimation->setEasingCurve(QEasingCurve::Linear);
    m_xAnimation->setDuration(AnimationTime / 2);

    m_animationGroup->addAnimation(m_wAnimation);
    m_animationGroup->addAnimation(m_xAnimation);
    connect(m_animationGroup, &QSequentialAnimationGroup::stateChanged, this, &ChatWindow::onAnimationStateChanged);

    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(m_webView);
    m_stackedWidget->addWidget(m_fakeView);
    m_stackedWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_stackedWidget->hide();

    QVBoxLayout *backLayout = new QVBoxLayout;
    backLayout->setMargin(0);
    backLayout->addWidget(m_stackedWidget);
    m_background->setLayout(backLayout);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::MSWindowsFixedSizeDialogHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(0);
    mainLayout->addWidget(m_background);
    setLayout(mainLayout);

    //All shortcut should be register after the cleanup.
    m_gloablShortcut->setObjectName(QString::fromUtf8("uos-ai-record"));
    connect(m_gloablShortcut, &QAction::triggered, this, &ChatWindow::onAudioRecShortcutPressed);
    KGlobalAccel::self()->setShortcut(m_gloablShortcut, QList<QKeySequence>() << QKeySequence("Meta+R"));

    setMaskAlpha(static_cast<quint8>(m_appearance->opacity() * 255));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ChatWindow::onSystemThemeChanged);
    connect(m_dock, &DDeDockObject::FrontendWindowRectChanged, this, &ChatWindow::onScreenOrDockChanged);

    updateSystemTheme();
    EAiExec()->setChatWindow(this);

    installEventFilter(this);

    connect(EWndManager(), &EChatWndManager::modalStateChanged, this, &ChatWindow::onModalStateChanged);
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
        if (isMinimized()) {
            showNormal();
        } else
            show();

        activateWindow();
    });
}

void ChatWindow::onAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
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
            m_fakeView->setPixmap(pixmap);
            m_fakeView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            m_fakeView->show();
        }
    }

    //the start and end of the animation need to be executed
    if (m_stackedWidget->isHidden()) {
        m_stackedWidget->show();
    }
    m_stackedWidget->setCurrentIndex(newState == QAbstractAnimation::Running);
}

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

void ChatWindow::updateGeometry(int currentWidth)
{
    DockPosition pos = static_cast<DockPosition>(m_dock->position());
    int mode = m_dock->displayMode();
    int dockMargin = 10;

    qreal ratio = qApp->primaryScreen()->devicePixelRatio();
    m_displayRect = m_display->primaryRect();
    QList<QDBusObjectPath> screenList = m_display->monitors();
    QRect frontendWindowRect = m_dock->frontendWindowRect();

    for (const QDBusObjectPath &screen : screenList) {
        __Monitor monitor("com.deepin.daemon.Display", screen.path(), QDBusConnection::sessionBus());
        QRect monitorRect(monitor.x(), monitor.y(), monitor.width(), monitor.height());
        if (monitor.enabled() && monitorRect.contains(frontendWindowRect.center())) {
            m_displayRect = QRect(monitorRect.x(), monitorRect.y(),
                                  monitorRect.width() / ratio, monitorRect.height() / ratio);
            break;
        }
    }

    int width = currentWidth;
    int height = m_displayRect.height() - DefaultMargin * 2;

    if (pos == DockPosition::Top || pos == DockPosition::Bottom) {
        if (mode == DockModel::Fashion) {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio - dockMargin;
        } else {
            height = m_displayRect.height() - DefaultMargin * 2 - frontendWindowRect.height() / ratio;
        }
    }

    int x = m_displayRect.x() + m_displayRect.width() - currentWidth - DefaultMargin;
    if (pos == DockPosition::Right) {
        if (mode == DockModel::Fashion) {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + dockMargin + DefaultMargin);
        } else {
            x =  m_displayRect.x() + m_displayRect.width() - (currentWidth + frontendWindowRect.width() / ratio + DefaultMargin);
        }
    }

    int y = m_displayRect.y() + DefaultMargin;
    if (pos == DockPosition::Top) {
        if (mode == DockModel::Fashion) {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio + dockMargin;
        } else {
            y = m_displayRect.y() + DefaultMargin + frontendWindowRect.height() / ratio;
        }
    }

    m_geometry = QRect(x, y, width, height);
    setFixedHeight(height);
    setMaximumWidth(m_displayRect.width() / 2 - DefaultMargin);
    setGeometry(m_geometry);

    m_wAnimation->setStartValue(0);
    m_wAnimation->setEndValue(width);

    m_xAnimation->setStartValue(x + DefaultMargin);
    m_xAnimation->setEndValue(x);
}

void ChatWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return;
}

void ChatWindow::hideEvent(QHideEvent *event)
{
    m_webView->setTohiddenState();
    return DBlurEffectWidget::hideEvent(event);
}

void ChatWindow::showEvent(QShowEvent *event)
{
    m_webView->setToShowedState();
    return DBlurEffectWidget::showEvent(event);
}

bool ChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    switch (event->type()) {
    case QEvent::WindowActivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(true);
        }
    } break;
    case QEvent::WindowDeactivate: {
        if (m_webView) {
            m_webView->setWindowActiveState(false);
        }
    } break;
    default:
        break;
    };
    return DBlurEffectWidget::eventFilter(obj, event);
}

void ChatWindow::updateSystemTheme()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::DarkType) {
        m_background->setStyleSheet("background-color:rgba(0, 0, 0, 128)");
    } else {
        m_background->setStyleSheet("background-color:rgba(255, 255, 255, 128)");
    }
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
    updateGeometry(this->width());
}

void ChatWindow::showWindow(ChatIndex index)
{
    if (ChatIndex::Talk == index) {
        //Requirement: if there is a modal window, do not do the switch and display the talk window
        if (QApplication::activeModalWidget()) {
            return;
        }

        m_webView->setToDigitalMode();
    }

    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        return;
    }

    if (isHidden()) {
        updateGeometry();
        activeShowWindow();
        m_animationGroup->setDirection(QAbstractAnimation::Forward);
        m_animationGroup->start();
        return;
    } else if (qApp->applicationState() != Qt::ApplicationActive) {
        onlyShowWidget();
        return;
    }

    if (ChatIndex::Talk != index) {
        closeWindow();
    }
}

void ChatWindow::closeWindow()
{
    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        return;
    }

    if (!isHidden()) {
        updateGeometry(this->width());
        m_animationGroup->setDirection(QAbstractAnimation::Backward);
        m_animationGroup->start();
        return;
    }

    onlyHideWidget();
}

void ChatWindow::onlyShowWindow()
{
    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        return;
    }

    if (isHidden()) {
        updateGeometry();
        activeShowWindow();
        m_animationGroup->setDirection(QAbstractAnimation::Forward);
        m_animationGroup->start();
        return;
    } else if (!m_webView->isActiveWindow()) {
        onlyShowWidget();
        return;
    }
}

void ChatWindow::onlyShowWidget()
{
    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        return;
    }

    if (isHidden()) {
        //Resume on redisplay
        updateGeometry();
    }

    activeShowWindow();
}

void ChatWindow::onlyHideWidget()
{
    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        return;
    }

    hide();
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
