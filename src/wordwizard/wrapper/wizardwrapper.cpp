#include "wizardwrapper.h"
#include "inputplaceholderwidget.h"
#include "wordwizard.h"
#include "dconfigmanager.h"
#include "dbwrapper.h"
#include "private/eaiexecutor.h"
#include "private/welcomedialog.h"
#include "private/wizarddpushbutton.h"
#include "esystemcontext.h"
#include <report/followalongpoint.h>
#include <report/eventlogutil.h>
#ifdef COMPILE_ON_V25
#include "ddeshellwayland.h"
#endif
#include <QHBoxLayout>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QDBusInterface>
#include <QDataStream>
#include <QCryptographicHash>
#include <QTimer>

#include <DStyle>
#include <DDialog>
#include <DFontSizeManager>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

#include <DFloatingMessage>
#include <DIconButton>
#include <DGuiApplicationHelper>
#include <QLabel>

#define  INIT_MODE_HEIGHT 24

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

WizardWrapper::WizardWrapper(QWidget *parent) : DBlurEffectWidget(parent)
{
    initUI();
    initConnect();
    initAnimation();

    m_customFunctionListHash = calculateCustomFunctionListHash();
}

WizardWrapper &WizardWrapper::instance() {
    static WizardWrapper instance;
    return instance;
}

void WizardWrapper::initUI()
{
    this->setFixedSize(INIT_MODE_HEIGHT, INIT_MODE_HEIGHT);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland())
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
#endif
    this->setAttribute(Qt::WA_AlwaysShowToolTips);
    this->setAttribute(Qt::WA_TranslucentBackground);
    DPlatformWindowHandle handle(this, this);
    handle.setWindowRadius(8);
    m_effectScene = handle.windowEffect();
    this->setObjectName("selectionwidget");
    this->setBlurEnabled(true);
    this->setBlurRectXRadius(8);
    this->setBlurRectYRadius(8);
    this->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    installEventFilter(this);

    // 创建输入区域容器
    m_inputArea = new InputPlaceholderWidget(this);
    m_inputArea->installEventFilter(this);
    
    // 创建图标按钮
    m_iconBtn = new DIconButton(this);
    m_iconBtn->setFixedSize(QSize(20, 20));
    m_iconBtn->setIcon(QIcon::fromTheme("UosAiAssistant"));
    m_iconBtn->setIconSize(QSize(20, 20));
    m_iconBtn->installEventFilter(this);
    
    m_lineSep = new DWidget(this);
    m_lineSep->setFixedSize(1, 15);
    m_lineSep->setAutoFillBackground(true);

    m_twoLineSep = new QLabel(this);
    m_twoLineSep->setFixedSize(5, 12);

    m_moreBtn = new WizardDPushButton(this);
    m_moreBtn->setFlat(true);
    m_moreBtn->setIcon(QIcon::fromTheme("uos-ai-assistant_downarrow"));
    m_moreBtn->setIconSize(QSize(12, 12));
    m_moreBtn->setFixedSize(26,26);
    m_moreBtn->installEventFilter(this);

    m_closeBtn = new WizardDPushButton(this);
    m_closeBtn->setFlat(true);
    m_closeBtn->setIcon(QIcon::fromTheme("uos-ai-assistant_close"));
    m_closeBtn->setIconSize(QSize(20, 20));
    m_closeBtn->setFixedHeight(24);
    m_closeBtn->installEventFilter(this);

    // Create  menu for disable actions
    m_disableMenu = new CustomDMenu(this);
    m_disableInProcessAction = createCustomMenuAction(tr("Hide until restarting this app"), "uos-ai-assistant_hide");
    m_disableInAppAction = createCustomMenuAction(tr("Disable in This Application"), "uos-ai-assistant_disabled");
    m_disableGlobalAction = createCustomMenuAction(tr("Disable Globally"), "uos-ai-assistant_disabled");
    if (ESystemContext::isWayland()){
        m_disableInProcessAction->setVisible(false);
        m_disableInAppAction->setVisible(false);
    }
    m_disableMenu->addAction(m_disableInProcessAction);
    m_disableMenu->addAction(m_disableInAppAction);
    m_disableMenu->addAction(m_disableGlobalAction);
    m_disableMenu->installEventFilter(this);
    m_disableMenu->addSeparator();
    m_customSettings = new QAction(tr("Custom Settings"));
    
    // Add clickable "Go to UOS AI Assistant Settings" text
    m_settingAction = new QWidgetAction(this);
    // 获取活动色
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor activeColor = palette.color(DPalette::Normal, DPalette::Highlight);
    QString colorName = activeColor.name(QColor::HexRgb);
    
    m_settingLabel = new QLabel(QString(tr("Go to <a href=\"#\" style=\"color:%1;text-decoration:none;\">Settings</a> to re-enable it.")).arg(colorName));
    m_settingLabel->setTextFormat(Qt::RichText);
    m_settingLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_settingLabel->setFixedHeight(34);
    if (ESystemContext::isWayland())
        m_settingLabel->installEventFilter(this);
    DFontSizeManager::instance()->bind(m_settingLabel, DFontSizeManager::T9, QFont::Normal);

    QWidget *m_settingWidget = new QWidget(this);
    QHBoxLayout* settingLayout = new QHBoxLayout(m_settingWidget);
    settingLayout->setContentsMargins(31, 0, 0, 0);
    settingLayout->setSpacing(0);
    settingLayout->addWidget(m_settingLabel);
    m_settingAction->setDefaultWidget(m_settingWidget);
    m_disableMenu->addAction(m_settingAction);
    connect(m_settingLabel, &QLabel::linkActivated, this, [this] {
        emit signalFunctionTriggered(WordWizard::WIZARD_TYPE_CUSTOM, m_cursorPos + QPoint(this->rect().width() / 2, 0), false);
    });

    m_moreMenu = new CustomDMenu(this);
    m_moreMenu->setMinimumWidth(92);
    m_moreMenu->installEventFilter(this);

    QHBoxLayout *scribeWordsLayout = new QHBoxLayout;
    scribeWordsLayout->setSpacing(2);
    scribeWordsLayout->setAlignment(Qt::AlignCenter);
    scribeWordsLayout->setContentsMargins(6, 0, 6, 0);
    this->setLayout(scribeWordsLayout);

    updateCustomFunctions();
    
    switchToInitMode();

    onUpdateSystemTheme(DGuiApplicationHelper::instance()->themeType());
}

void WizardWrapper::initConnect()
{
    connect(m_iconBtn, &DIconButton::clicked, this, &WizardWrapper::onIconBtnClicked);
    connect(m_inputArea, &InputPlaceholderWidget::clicked, this, &WizardWrapper::onInputAreaClicked);
    connect(m_moreMenu, &QMenu::triggered, this, &WizardWrapper::onMoreMenuTriggered);
    connect(m_disableMenu, &QMenu::triggered, this, &WizardWrapper::onDisableMenuTriggered);
    connect(m_moreBtn, &WizardDPushButton::clicked, this, &WizardWrapper::showMenu);

    //避免随航检测到mouseRelease事件后又弹出
    connect(m_closeBtn, &WizardDPushButton::clicked, this, [&] {
        emit signalRequestServer();
        adjustSize();
        m_disableMenu->exec(m_closeBtn->mapToGlobal(m_closeBtn->rect().bottomLeft() + QPoint(0, 5)));
        m_closeBtn->update();
    });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WizardWrapper::onUpdateSystemTheme);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, [this]() {
        if (m_isExtend) {
            QTimer::singleShot(0, this, [this]() {
                int calculatedWidth = this->calculateWidth();
                this->setFixedWidth(calculatedWidth);
            });
        }
    });

}

bool WizardWrapper::eventFilter(QObject *watched, QEvent *event)
{
    // 显示工具栏逻辑
    if (event->type() == QEvent::Enter) {
        m_mouseInside = true;
        if (watched == m_iconBtn && !m_isExtend) {
            QTimer::singleShot(500, this, [ = ]{
                expandWrapper();
            });
        }
    } else if (event->type() == QEvent::Leave && watched != m_iconBtn && watched != m_inputArea && watched != m_settingLabel) {
        m_mouseInside = false;
    }

    // 处理自定义菜单项的hover事件
    if (m_customMenuWidgets.contains(static_cast<QWidget*>(watched))) {
        if (event->type() == QEvent::HoverEnter) {
            updateHoverStyle(static_cast<QWidget*>(watched), true);
            return true;
        } else if (event->type() == QEvent::HoverLeave) {
            updateHoverStyle(static_cast<QWidget*>(watched), false);
            return true;
        }
    }

    // 拖动逻辑
    bool isDraggableWidget = false;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        if (watched == m_iconBtn || watched == m_moreBtn || watched == m_closeBtn) {
            isDraggableWidget = true;
        } else {
            for (WizardDPushButton *btn : m_functionButtons) {
                if (watched == btn) {
                    isDraggableWidget = true;
                    break;
                }
            }
        }

        if (isDraggableWidget) {
            if (event->type() == QEvent::Enter) {
                this->setCursor(Qt::CursorShape::ArrowCursor);
            } else if (event->type() == QEvent::Leave) {
                this->setCursor(Qt::CursorShape::OpenHandCursor);
            }
        }
    }

    return DBlurEffectWidget::eventFilter(watched, event);
}

void WizardWrapper::mousePressEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        if (event->button() == Qt::LeftButton) {
            this->setCursor(Qt::CursorShape::ClosedHandCursor);
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            return;
        }
    }

    DBlurEffectWidget::mousePressEvent(event);
}

void WizardWrapper::mouseMoveEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        if (m_dragging) {
            this->setCursor(Qt::CursorShape::ClosedHandCursor);
            this->move(event->globalPos() - m_dragStartPos);
            return;
        }
    }

    DBlurEffectWidget::mouseMoveEvent(event);
}

void WizardWrapper::mouseReleaseEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            this->setCursor(Qt::CursorShape::OpenHandCursor);
            return;
        }
    }

    DBlurEffectWidget::mouseReleaseEvent(event);
}

#ifdef COMPILE_ON_QT6
void WizardWrapper::enterEvent(QEnterEvent *event)
#else
void WizardWrapper::enterEvent(QEvent *event)
#endif
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        this->setCursor(Qt::CursorShape::OpenHandCursor);
    }
    DBlurEffectWidget::enterEvent(event);
}

void WizardWrapper::leaveEvent(QEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland() && m_isExtend) {
        this->setCursor(Qt::CursorShape::ArrowCursor);
    }
    DBlurEffectWidget::leaveEvent(event);
}

void WizardWrapper::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &themeType)
{
    // update two line sep.
    {
        qreal ratio = this->devicePixelRatioF();
        int w = static_cast<int>(std::ceil(5 * ratio + 0.5) - 1); // for 1.5 scaled
        int h = static_cast<int>(12 * ratio);
        auto pix = QIcon::fromTheme("uos-ai-twolines").pixmap(QSize(5, 12));
        if (pix.width() != w)
            pix = pix.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_twoLineSep->setPixmap(pix);
    }

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    if (themeType == DGuiApplicationHelper::LightType) {
        if(m_lineSep) {
            DPalette sepPalette = m_lineSep->palette();
            sepPalette.setColor(DPalette::Window, QColor(0, 0, 0, int(255 * 0.2)));
            m_lineSep->setPalette(sepPalette);
        }
        QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);
        setMaskColor(backgroundColor);
        setMaskAlpha(255);
    } else {
        if(m_lineSep) {
            DPalette sepPalette = m_lineSep->palette();
            sepPalette.setColor(DPalette::Window, QColor(255, 255, 255, int(255 * 0.2)));
            m_lineSep->setPalette(sepPalette);
        }
        QColor backgroundColor(32, 32, 32);
        setMaskColor(backgroundColor);
        setMaskAlpha(0.8 * 255);
    }
    // Update setting label link color to use active palette color
    if (m_settingLabel) {
        DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
        QColor activeColor = palette.color(DPalette::Normal, DPalette::Highlight);
        QString colorName = activeColor.name(QColor::HexRgb);
        m_settingLabel->setText(QString(tr("Go to <a href=\"#\" style=\"color:%1;text-decoration:none;\">Settings</a> to re-enable it.")).arg(colorName));
    }

    for (QWidget* widget : m_customMenuWidgets) {
        if (widget) {
            // 检查widget是否处于hover状态
            bool isHovered = widget->underMouse();
            updateHoverStyle(widget, isHovered);
        }
    }

    update();
}

void WizardWrapper::showScribeWordsAtCursorPosition(QRect screenRect, QPoint& point, bool isMouseRelease, bool isShortcut)
{
    checkAndUpdateLayout();

    m_screenRect = screenRect;
    m_cursorPos = point;
    if(isMouseRelease || isShortcut){
        if (isShortcut) {
            QList<QScreen *> screens = QGuiApplication::screens();
            for(QScreen *screen : screens) {
                if (screen->geometry().contains(m_cursorPos)) {
                    m_screenRect = screen->geometry();
                }
            }
            m_cursorPos = m_screenRect.center() - QPoint(this->rect().width() / 2, 0);
        }
        else {
            adjustShowPos(m_screenRect, m_cursorPos);
        }
        move(m_cursorPos);
        close();
        m_isVisible = false;
        showWizardWrapperWithAnimation();
    }
}

void WizardWrapper::initAnimation()
{
    m_animation = new QPropertyAnimation(this, "windowOpacity");
    m_animation->setDuration(300);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutInQuad);

    // 初始化收起动画
    m_collapseAnimation = new QPropertyAnimation(this, "windowOpacity");
    m_collapseAnimation->setDuration(250);
    m_collapseAnimation->setEasingCurve(QEasingCurve::InQuad);

    m_scaleAnim = new QPropertyAnimation(this, "geometry");
    m_scaleAnim->setDuration(250);
    // 关键：设置运动节奏为ease-in（先慢后快）
    m_scaleAnim->setEasingCurve(QEasingCurve::InQuad);

    return;
}

void WizardWrapper::showWizardWrapperWithAnimation()
{
    this->setWindowOpacity(0.0);
    this->show();
    m_isVisible = true;
    m_mouseInside = false;
#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        DDEShellWayland::get(windowHandle())->setAutoPlacement(1);
        DDEShellWayland::get(windowHandle())->setSkipDockPreview(true);
        DDEShellWayland::get(windowHandle())->setSkipMutiTaskView(true);
        DDEShellWayland::get(windowHandle())->setSkipSwitcher(true);
        DDEShellWayland::get(windowHandle())->setAcceptKeyboardFocus(false);
    }
#endif
    update();
    adjustSize();
    m_animation->start();
}

void WizardWrapper::onMoreMenuTriggered(const QAction *action)
{
    if (!WelcomeDialog::isAgreed()) {
        qCWarning(logWordWizard) << "Welcome dialog not agreed, closing wizard";
        this->close();
        m_isVisible = false;
        WelcomeDialog::instance(false)->exec();
        return;
    }

    int wizardtype = -1;
    bool isCustom = false;

    if (action == m_customSettings) {
        qCDebug(logWordWizard) << "Custom settings action triggered";
        wizardtype = WordWizard::WIZARD_TYPE_CUSTOM;
        isCustom = false;
    } else {
        bool found = false;
        for (QAction *customAction : m_customActions) {
            if (customAction == action) {
                int functionIndex = action->property("functionIndex").toInt();
                const QList<CustomFunction> &customFunctions = WordWizard::kCustomFunctionList;
                if (functionIndex >= 0 && functionIndex < customFunctions.size()) {
                    const CustomFunction &func = customFunctions[functionIndex];
                    isCustom = func.isCustom;
                    if (isCustom) {
                        wizardtype = functionIndex;
                    } else {
                        wizardtype = func.defaultFunctionType;
                    }
                }
                found = true;
                break;
            }
        }

        if (!found) {
            qCWarning(logWordWizard) << "Unknown action triggered";
            return;
        }
    }

    emit signalFunctionTriggered(wizardtype, m_cursorPos + QPoint(this->rect().width() / 2, 0), isCustom);
}

void WizardWrapper::onDisableMenuTriggered(const QAction *action)
{
    if (!WelcomeDialog::isAgreed()) {
        qCWarning(logWordWizard) << "Welcome dialog not agreed, closing wizard";
        this->close();
        m_isVisible = false;
        WelcomeDialog::instance(false)->exec();
        return;
    }

    int wizardtype = -1;
    bool isCustom = false;

    if (action == m_disableGlobalAction) {
        qCDebug(logWordWizard) << "Disable globally action triggered";
        wizardtype = WordWizard::WIZARD_TYPE_HIDDEN;
        isCustom = false;
    } else if (!ESystemContext::isWayland() && action == m_disableInAppAction) {
        qCDebug(logWordWizard) << "Disable in app action triggered";
        emit signalDisableInApp();
        return;
    } else if (!ESystemContext::isWayland() && action == m_disableInProcessAction) {
        qCDebug(logWordWizard) << "Disable in process action triggered";
        emit signalDisableInProcess();
        return;
    }

    emit signalFunctionTriggered(wizardtype, m_cursorPos + QPoint(this->rect().width() / 2, 0), isCustom);
}

void WizardWrapper::onButtonClicked()
{
    int wizardtype = -1;
    bool isCustom = false;
    int defaultFunctionType = -1;

    for (WizardDPushButton *btn : m_functionButtons) {
        if (sender() == btn) {
            int functionIndex = btn->property("functionIndex").toInt();
            isCustom = btn->property("isCustom").toBool();
            defaultFunctionType = btn->property("defaultFunctionType").toInt();
            if (isCustom) {
                wizardtype = functionIndex;
            } else {
                wizardtype = defaultFunctionType;
            }
            break;
        }
    }

    if (wizardtype == -1) {
        qCWarning(logWordWizard) << "Unknown button clicked";
        return;
    }

    // 检查是否需要同意协议（只有非自定义的搜索功能不需要协议）
    bool needAgreement = true;
    if (!isCustom && defaultFunctionType == WordWizard::WIZARD_TYPE_SEARCH) {
        needAgreement = false;
    }

    if (needAgreement && !WelcomeDialog::isAgreed()) {
        qCWarning(logWordWizard) << "Welcome dialog not agreed, closing wizard";
        this->close();
        m_isVisible = false;
        WelcomeDialog::instance(false)->exec();
        return;
    }

    emit signalRequestServer();
    emit signalFunctionTriggered(wizardtype, m_cursorPos + QPoint(this->rect().width() / 2, 0), isCustom);
}

void WizardWrapper::showMenu()
{
    emit signalRequestServer();
    adjustSize();
    m_moreMenu->exec(m_moreBtn->mapToGlobal(m_moreBtn->rect().bottomLeft() + QPoint(0, 5)));
    m_moreBtn->update();
}

void WizardWrapper::onCloseWidget()
{
    // Keep this as fallback if needed elsewhere
    qCDebug(logWordWizard) << "Closing widget";
    this->close();
    m_isVisible = false;
    emit signalCloseBtnClicked();
}

void WizardWrapper::onIconBtnClicked()
{
    if (m_isExtend) {
        this->close();
        m_isVisible = false;
        emit signalIconBtnClicked();
    } else {
        expandWrapper();
    }
}

void WizardWrapper::onInputAreaClicked()
{

    int wizardWidth = this->width();
    int wizardHeight = 36;

    // 获取当前窗口的位置
    QPoint currentPos = QPoint(this->x()+(this->width()-wizardWidth)/2, this->y()+(this->height()-wizardHeight)/2);

    // 开始向左收起动画
    collapseWithAnimation();

    // 发送信号给WordWizard来显示InputWindow
    emit signalShowInputWindow(currentPos, m_screenRect, wizardWidth, wizardHeight);
}



void WizardWrapper::adjustShowPos(const QRect &screenRect, QPoint &pos)
{
    adjustSize();
    pos = pos - QPoint(this->rect().width() / 2, 0);

    QRect widgetRect(pos.x(), pos.y(), this->rect().width(), this->rect().height());
    if (screenRect.contains(widgetRect))
        return;
    if (screenRect.left() > widgetRect.left())
        pos.setX(screenRect.left());
    if (screenRect.top() > widgetRect.top())
        pos.setY(screenRect.top());
    if (screenRect.right() < widgetRect.right())
        pos.setX(screenRect.right() - widgetRect.width());
    if (screenRect.bottom() < widgetRect.bottom())
        pos.setY(widgetRect.top() - 2 * widgetRect.height());
}

void WizardWrapper::isEnabledAction(bool isEnabled)
{
    if (!ESystemContext::isWayland()) {
        m_disableInAppAction->setEnabled(isEnabled);
        m_disableInProcessAction->setEnabled(isEnabled);
    }
    m_disableGlobalAction->setEnabled(isEnabled);
}

bool WizardWrapper::isFirstClose()
{
    return DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLOSE).toBool();
}

void WizardWrapper::expandWrapper()
{
    if (!m_isExtend && this->geometry().contains(QCursor::pos())) {
        this->hide();
        switchToExpandedMode();
        adjustShowPos(m_screenRect, m_cursorPos);
        move(m_cursorPos);
        this->show();
        this->adjustSize();
    }
}

void WizardWrapper::clearLayout()
{
    QHBoxLayout *layout = this->findChild<QHBoxLayout *>();
    if (layout) {
        while (layout->count() > 0) {
            QLayoutItem *item = layout->takeAt(0);
            if (item->widget()) {
                layout->removeWidget(item->widget());
            }
            delete item;
        }
    }
}

void WizardWrapper::switchToInitMode()
{
    // 初始化模式：仅显示图标按钮
    m_isExtend = false;
    this->setFixedSize(INIT_MODE_HEIGHT, INIT_MODE_HEIGHT);

    // 设置窗口为完全透明和无阴影
    this->setBlurEnabled(false);
    DPlatformWindowHandle handle(this, this);
    handle.setWindowEffect(m_effectScene | DPlatformWindowHandle::EffectScene::EffectNoShadow);
    // 隐藏所有功能按钮和输入相关组件
    for (WizardDPushButton *btn : m_functionButtons) {
        btn->hide();
    }
    m_moreBtn->hide();
    m_closeBtn->hide();
    m_inputArea->hide();
    m_lineSep->hide();
    m_twoLineSep->hide();
    
    // 显示并设置图标按钮为透明背景
    m_iconBtn->setFixedSize(QSize(INIT_MODE_HEIGHT, INIT_MODE_HEIGHT));
    m_iconBtn->setIcon(QIcon::fromTheme("uos-ai-assistant-wordwizard"));
    m_iconBtn->setIconSize(QSize(INIT_MODE_HEIGHT, INIT_MODE_HEIGHT));
    m_iconBtn->setFlat(true);
    m_iconBtn->setBackgroundRole(QPalette::NoRole);
    m_iconBtn->setAutoFillBackground(false);
    
    // 重新布局以确保图标居中
    clearLayout();
    QHBoxLayout *layout = this->findChild<QHBoxLayout *>();
    if (layout) {
        layout->addStretch();
        layout->addWidget(m_iconBtn);
        layout->addStretch();
        layout->setContentsMargins(0, 0, 0, 0);
    }
    
    adjustSize();
}

void WizardWrapper::switchToExpandedMode()
{
    // 展开模式：显示功能按钮和输入区域示意
    m_isExtend = true;
    
    // 恢复背景透明度和模糊效果，去掉noshadow
    this->setBlurEnabled(true);
    DPlatformWindowHandle handle(this, this);
    handle.setWindowEffect(m_effectScene);
    onUpdateSystemTheme(DGuiApplicationHelper::instance()->themeType());
    
    // 显示所有功能按钮
    for (WizardDPushButton *btn : m_functionButtons) {
        btn->show();
        btn->setHoverStatus(false);
    }
    
    // 显示其他UI组件
    m_moreBtn->show();
    m_closeBtn->show();
    m_inputArea->show();
    m_lineSep->show();
    m_twoLineSep->show();
    
    // 重新构建展开模式的布局
    clearLayout();
    QHBoxLayout *layout = this->findChild<QHBoxLayout *>();
    if (layout) {
        // 添加展开模式的所有组件
        layout->addWidget(m_twoLineSep, Qt::AlignVCenter);
        layout->addSpacing(5);
        m_iconBtn->setFixedSize(QSize(20, 20));
        m_iconBtn->setIcon(QIcon::fromTheme("UosAiAssistant"));
        m_iconBtn->setIconSize(QSize(20, 20));
        layout->addWidget(m_iconBtn, Qt::AlignVCenter);
        
        for (WizardDPushButton *btn : m_functionButtons) {
            layout->addWidget(btn, Qt::AlignVCenter);
        }
        
        layout->addWidget(m_lineSep, Qt::AlignVCenter);
        layout->addWidget(m_moreBtn, Qt::AlignVCenter);
        layout->addWidget(m_inputArea, Qt::AlignVCenter);
        layout->addWidget(m_closeBtn, Qt::AlignVCenter);
        layout->addStretch();
        
        // 设置展开模式的布局参数
        layout->setContentsMargins(9, 0, 6, 0);
    }
    
    // 设置窗口大小
    if (!m_functionButtons.isEmpty() && (m_functionButtons.first()->height() + 6) > 36) {
        this->setFixedHeight(m_functionButtons.first()->height() + 6);
    } else {
        this->setFixedHeight(36);
    }
    this->setFixedWidth(calculateWidth());
    this->setMaximumWidth(QWIDGETSIZE_MAX);
    
    // tid:1001600001 event:followalong
    ReportIns()->writeEvent(report::FollowalongPoint().assemblingData());
    
    adjustSize();
}

// 新增方法：向左收起动画
void WizardWrapper::collapseWithAnimation()
{
    if (!m_isVisible || !m_isExtend) {
        return;
    }

    m_collapseAnimation->setStartValue(1.0);
    m_collapseAnimation->setEndValue(0.0);

    // 连接动画完成信号，恢复固定尺寸设置
    connect(m_collapseAnimation, &QPropertyAnimation::finished, this, [this]() {
        // 断开连接，避免重复执行
        disconnect(m_collapseAnimation, &QPropertyAnimation::finished, this, nullptr);

        // 调用原有的关闭逻辑
        this->close();
        this->setWindowOpacity(1.0);
        m_isVisible = false;
    });

    // 记录原始位置和大小（假设旧面板在屏幕上的坐标是x,y，宽w高h）
    QRect originalRect = this->geometry();
    m_scaleAnim->setStartValue(originalRect);
    // 计算缩小后的位置（保持中心不变，大小缩为0.9倍）
    QRect scaledRect = QRect(
                originalRect.x() + originalRect.width()*0.05,  // 左移5%宽度（中心不变）
                originalRect.y() + originalRect.height()*0.05, // 上移5%高度（中心不变）
                originalRect.width()*0.9,
                originalRect.height()*0.9
                );
    m_scaleAnim->setEndValue(scaledRect);

    // 开始动画
    m_collapseAnimation->start();
    m_scaleAnim->start();
}

void WizardWrapper::setKnowledgeActionEnabled(bool enabled)
{
    const QList<CustomFunction> &customFunctions = WordWizard::kCustomFunctionList;
    QString tooltip = enabled ? "" : tr("The added content must be more than 10 words");

    // Update buttons
    for (WizardDPushButton *btn : m_functionButtons) {
        bool isCustom = btn->property("isCustom").toBool();
        int defaultFunctionType = btn->property("defaultFunctionType").toInt();

        if (!isCustom && defaultFunctionType == WordWizard::WIZARD_TYPE_KNOWLEDGE) {
            btn->setEnabled(enabled);
            btn->setToolTip(tooltip);
        }
    }

    // Update menu actions
    for (QAction *action : m_customActions) {
        int functionIndex = action->property("functionIndex").toInt();
        if (functionIndex >= 0 && functionIndex < customFunctions.size()) {
            const CustomFunction &func = customFunctions[functionIndex];
            if (!func.isCustom && func.defaultFunctionType == WordWizard::WIZARD_TYPE_KNOWLEDGE) {
                action->setEnabled(enabled);
                action->setToolTip(tooltip);
            }
        }
    }
}

void WizardWrapper::createDynamicButtons()
{
    for (WizardDPushButton *btn : m_functionButtons) {
        btn->deleteLater();
    }
    m_functionButtons.clear();

    const QList<CustomFunction> &customFunctions = WordWizard::kCustomFunctionList;
    int buttonCount = 0;
    for (int i = 0; i < customFunctions.size() && buttonCount < 4; ++i) {
        const CustomFunction &func = customFunctions[i];
        if (func.isHidden)
            continue;

        QString buttonText;
        QString iconName;

        if (func.isCustom) {
            buttonText = func.name;
            iconName = "uos-ai-assistant_custom";
        } else {
            switch (func.defaultFunctionType) {
                case WordWizard::WIZARD_TYPE_SEARCH:
                    buttonText = tr("Search");
                    iconName = "uos-ai-assistant_ai_search";
                    break;
                case WordWizard::WIZARD_TYPE_EXPLAIN:
                    buttonText = tr("Explain");
                    iconName = "uos-ai-assistant_explain";
                    break;
                case WordWizard::WIZARD_TYPE_SUMMARIZE:
                    buttonText = tr("Summary");
                    iconName = "uos-ai-assistant_summarize";
                    break;
                case WordWizard::WIZARD_TYPE_TRANSLATE:
                    buttonText = tr("Translate");
                    iconName = "uos-ai-assistant_translation";
                    break;
                case WordWizard::WIZARD_TYPE_RENEW:
                    buttonText = tr("Continue Writing");
                    iconName = "uos-ai-assistant_renew";
                    break;
                case WordWizard::WIZARD_TYPE_EXTEND:
                    buttonText = tr("Expand");
                    iconName = "uos-ai-assistant_extend";
                    break;
                case WordWizard::WIZARD_TYPE_CORRECT:
                    buttonText = tr("Correct");
                    iconName = "uos-ai-assistant_correct";
                    break;
                case WordWizard::WIZARD_TYPE_POLISH:
                    buttonText = tr("Polish");
                    iconName = "uos-ai-assistant_polish";
                    break;
                case WordWizard::WIZARD_TYPE_KNOWLEDGE:
                    buttonText = tr("Add to the AI knowledge base");
                    iconName = "uos-ai-assistant_knowledge";
                    break;
                default:
                    break;
            }
        }

        WizardDPushButton *btn = new WizardDPushButton(buttonText, this);
        btn->setIcon(QIcon::fromTheme(iconName));
        btn->setIconSize(QSize(20, 20));
        btn->setFlat(true);
        DFontSizeManager::instance()->bind(btn, DFontSizeManager::T7, QFont::Normal);
        btn->installEventFilter(this);

        btn->setProperty("functionIndex", i);
        btn->setProperty("isCustom", func.isCustom);
        btn->setProperty("defaultFunctionType", func.defaultFunctionType);

        m_functionButtons.append(btn);
        buttonCount++;
    }
}

void WizardWrapper::updateButtonsFromCustomFunctions()
{
    for (QAction *action : m_customActions) {
        m_moreMenu->removeAction(action);
        action->deleteLater();
    }
    m_customActions.clear();

    m_moreMenu->removeAction(m_customSettings);
    QList<QAction*> actions = m_moreMenu->actions();
    for (QAction* action : actions) {
        if (action->isSeparator()) {
            m_moreMenu->removeAction(action);
        }
    }

    const QList<CustomFunction> &customFunctions = WordWizard::kCustomFunctionList;

    int buttonCount = 0;
    for (int i = 0; i < customFunctions.size(); ++i) {
        const CustomFunction &func = customFunctions[i];
        if (func.isHidden) {
            continue;
        }

        if (buttonCount < 4) {
            buttonCount++;
            continue;
        }

        QString actionText;
        if (func.isCustom) {
            actionText = func.name;
        } else {
            actionText = WordWizard::getDefaultSkillName(func.defaultFunctionType);
        }

        QFontMetrics fm(font());
        QString truncatedText = actionText;
        if (fm.horizontalAdvance(truncatedText) > 140) {
            while (fm.horizontalAdvance(truncatedText) > 140 && !truncatedText.isEmpty() && truncatedText.length() > 10) {
                truncatedText.chop(1);
            }
            truncatedText += "...";
        }

        QAction *action = new QAction(truncatedText, this);
        action->setProperty("functionIndex", i);
        m_customActions.append(action);
        m_moreMenu->addAction(action);
    }

    if (!m_customActions.isEmpty()) {
        m_moreMenu->addSeparator();
    }
    m_moreMenu->addAction(m_customSettings);
}

void WizardWrapper::updateCustomFunctions()
{
    createDynamicButtons();
    for (WizardDPushButton *btn : m_functionButtons) {
        connect(btn, &WizardDPushButton::clicked, this, &WizardWrapper::onButtonClicked);
    }

    this->updateButtonsFromCustomFunctions();
}

uint WizardWrapper::calculateCustomFunctionListHash() const
{
    const QList<CustomFunction> &customFunctions = WordWizard::kCustomFunctionList;

    QCryptographicHash hash(QCryptographicHash::Sha256);

    for (const CustomFunction &func : customFunctions) {
        hash.addData(func.name.toUtf8());
        hash.addData(func.prompt.toUtf8());
        hash.addData(QByteArray::number(func.isCustom ? 1 : 0));
        hash.addData(QByteArray::number(func.isHidden ? 1 : 0));
        hash.addData(QByteArray::number(func.defaultFunctionType));
    }

    QByteArray result = hash.result();
    return qFromBigEndian<uint>(reinterpret_cast<const uchar*>(result.constData()));
}

void WizardWrapper::checkAndUpdateLayout()
{
    uint currentHash = calculateCustomFunctionListHash();

    if (currentHash != m_customFunctionListHash) {
        qCDebug(logWordWizard) << "CustomFunction list changed, updating layout";
        m_customFunctionListHash = currentHash;

        updateCustomFunctions();
        // 如果是快捷键调起的，需要是展开状态
        if (m_isExtend) {
            switchToExpandedMode();
        } else {
            switchToInitMode();
        }
    }
}

int WizardWrapper::calculateWidth()
{
    for (WizardDPushButton *btn : m_functionButtons) {
        if (btn && btn->isVisible()) {
            btn->adjustSize();
        }
    }

    if (m_moreBtn) {
        m_moreBtn->adjustSize();
    }
    if (m_closeBtn) {
        m_closeBtn->adjustSize();
    }

    int totalWidth = 0;

    // 布局margins: 展开状态是 (9, 0, 6, 0)
    totalWidth += 9 + 6;

    totalWidth += 5;  // m_twoLineSep
    totalWidth += 5;  // m_lineSepSpace
    totalWidth += 75; // m_inputArea

    int componentCount = 4;

    int visibleButtonCount = 0;
    for (int i = 0; i < m_functionButtons.size(); ++i) {
        WizardDPushButton *btn = m_functionButtons[i];
        if (!btn) {
            continue;
        }
        totalWidth += btn->width();
        componentCount++;
        visibleButtonCount++;
    }

    totalWidth += 2;  // m_lineSep
    componentCount++;

    totalWidth += m_moreBtn ? m_moreBtn->width() : 24;
    totalWidth += m_closeBtn ? m_closeBtn->width() : 24;
    componentCount += 2;

    totalWidth += (componentCount - 1) * 1;

    totalWidth += 6;

    return totalWidth;
}

void WizardWrapper::showToast(const QString &message)
{
    DFloatingMessage *floatMessage = new DFloatingMessage(DFloatingMessage::TransientType);
    floatMessage->setMessage(message);
    floatMessage->setAttribute(Qt::WA_DeleteOnClose);
    floatMessage->setIcon(QIcon(":/assets/images/ok_info.svg"));
    
    // 设置窗口为无边框
    floatMessage->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip);
    floatMessage->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(floatMessage);
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(2);
    shadowEffect->setYOffset(2);
    shadowEffect->setColor(QColor(0, 0, 0, int(255 * 0.2))); // (不透明度约20%)
    floatMessage->setGraphicsEffect(shadowEffect);

    // 设置文本居中和颜色
    QLabel *label = floatMessage->findChild<QLabel *>();
    if (label) {
        label->setAlignment(Qt::AlignCenter);
        DPalette labelPalette = label->palette();
        labelPalette.setColor(DPalette::WindowText, QColor(0, 0, 0, 178));
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            labelPalette.setColor(DPalette::WindowText, QColor(255, 255, 255, 178));
        }
        label->setPalette(labelPalette);
    }

    // 获取屏幕尺寸
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        screen = QGuiApplication::screenAt(this->pos());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QRect screenGeometry = screen->geometry();
    
    // 计算Toast位置 - 显示在屏幕正下方中央
    QRect geometry(QPoint(0, 0), floatMessage->sizeHint());
    
    // 水平居中
    geometry.moveCenter(QPoint(screenGeometry.center().x(), geometry.center().y()));
    
    // 垂直位置：距离屏幕底部50像素
    geometry.moveBottom(screenGeometry.bottom() - 100);
    
    floatMessage->setGeometry(geometry);
    floatMessage->show();
}

QAction* WizardWrapper::createCustomMenuAction(const QString& text, const QString& iconName)
{
    QWidgetAction* action = new QWidgetAction(this);

    // Create a custom widget for the menu item
    QWidget* widget = new QWidget();
    widget->setFixedHeight(34);
    widget->setObjectName("customMenuWidget");
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 6, 65, 6);
    layout->setSpacing(6);

    // Create icon label
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(16, 16));
    iconLabel->setFixedSize(16, 16);
    iconLabel->setObjectName("customMenuIcon");

    // Create text label
    QLabel* textLabel = new QLabel(text);
    textLabel->setObjectName("customMenuText");
    DFontSizeManager::instance()->bind(textLabel, DFontSizeManager::T6, QFont::Normal);

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();

    widget->setLayout(layout);
    
    // Enable mouse tracking for hover effects
    widget->setMouseTracking(true);
    widget->setAttribute(Qt::WA_Hover, true);
    
    // Install event filter to handle hover events
    widget->installEventFilter(this);
    
    // Store widget reference for hover handling
    m_customMenuWidgets.append(widget);
    m_customMenuWidgetIconNames.append(iconName);
    action->setDefaultWidget(widget);

    return action;
}

void WizardWrapper::updateHoverStyle(QWidget* widget, bool hovered)
{
    if (!widget) return;

    QString iconName = m_customMenuWidgetIconNames.value(m_customMenuWidgets.indexOf(widget));
    
    // Find the icon label within the widget
    QLabel* iconLabel = widget->findChild<QLabel*>("customMenuIcon");
    if (!iconLabel) return;
    
    if (hovered) {
        // Get current system palette colors
        DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
        QColor highlightColor = palette.color(DPalette::Normal, DPalette::Highlight);
        QColor highlightTextColor = palette.color(DPalette::Normal, DPalette::HighlightedText);
        
        QString hoverStyle = QString(
                    "#customMenuWidget {"
                    "    background-color: %1;"
                    "}"
                    "#customMenuWidget #customMenuText {"
                    "    color: %2;"
                    "}"
                    ).arg(
                    highlightColor.name(),
                    highlightTextColor.name()
                    );
        
        widget->setStyleSheet(hoverStyle);
        iconLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(16, 16, QIcon::Mode::Selected));
    } else {
        // Reset to default style
        widget->setStyleSheet("");
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            iconLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(16, 16));
        } else {
            iconLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(16, 16, QIcon::Mode::Selected));
        }
    }
}
