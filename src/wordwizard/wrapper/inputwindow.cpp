#include "inputwindow.h"
#include "dconfigmanager.h"
#include "wordwizard.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QTimer>
#include <QMenu>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <DFontSizeManager>
#include <DPlatformWindowHandle>
#include <DDialog>
#include "../utils/esystemcontext.h"
#include "private/sendbutton.h"
#include "private/wizarddpushbutton.h"
#include "private/welcomedialog.h"
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)


DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

InputWindow::InputWindow(QWidget *parent) : DBlurEffectWidget(parent)
{
    initUI();
    initConnect();
    initAnimation();
}

InputWindow &InputWindow::instance() {
    static InputWindow instance;
    return instance;
}

void InputWindow::initUI()
{
    this->setFixedHeight(32);  // 与WizardWrapper的默认高度保持一致
    this->setMinimumWidth(300);
    this->setMaximumWidth(600);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland())
        this->setWindowFlags(Qt::WindowStaysOnTopHint);
#endif
    this->setAttribute(Qt::WA_AlwaysShowToolTips);
    this->setAttribute(Qt::WA_TranslucentBackground);
    DPlatformWindowHandle handle(this, this);
    handle.setWindowRadius(8);
    this->setObjectName("inputwindow");
    this->setBlurEnabled(true);
    this->setBlurRectXRadius(8);
    this->setBlurRectYRadius(8);
    this->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    
    // 创建水平布局，与WizardWrapper保持一致
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(1);  // 与WizardWrapper保持一致
    layout->setContentsMargins(9, 0, 6, 0);  // 与WizardWrapper保持一致
    
    m_lineSep1 = new DWidget(this);
    m_lineSep1->setFixedSize(1, 12);
    m_lineSep1->setAutoFillBackground(true);

    m_lineSep2 = new DWidget(this);
    m_lineSep2->setFixedSize(1, 12);
    m_lineSep2->setAutoFillBackground(true);

    DPalette sepPalette = m_lineSep1->palette();
    sepPalette.setColor(DPalette::Window, QColor(0, 0, 0, int(255 * 0.2)));
    m_lineSep1->setPalette(sepPalette);
    m_lineSep2->setPalette(sepPalette);

    // 创建输入框
    m_inputEdit = new DLineEdit(this);
    m_inputEdit->setPlaceholderText(tr("What to ask about this?"));
    m_inputEdit->setFixedHeight(24);
    m_inputEdit->setMinimumWidth(75);
    DFontSizeManager::instance()->bind(m_inputEdit, DFontSizeManager::T7, QFont::Normal);
    
    // 创建发送按钮
    m_sendBtn = new SendButton(this);
    // 设置发送按钮的上下边距为2，通过调整按钮尺寸实现
    m_sendBtn->setFixedSize(QSize(16, 22));  // 高度从16增加到22，提供上下边距（上下边距+输入框边框宽度）
    m_sendBtn->setIcon(QIcon::fromTheme("uos-ai-assistant_aisend"));
    m_sendBtn->setIconSize(QSize(10, 10));
    m_sendBtn->setEnabled(false);

    // 创建QWidgetAction并将发送按钮添加到其中
    m_sendBtnAction = new QWidgetAction(this);
    m_sendBtnAction->setDefaultWidget(m_sendBtn);

    // 将发送按钮动作添加到输入框的右侧
    m_inputEdit->lineEdit()->addAction(m_sendBtnAction, QLineEdit::TrailingPosition);
    m_inputEdit->lineEdit()->setProperty("_d_dtk_lineeditIconMargin", 0);

    // 隐藏清除按钮
    m_inputEdit->lineEdit()->setClearButtonEnabled(false);

    m_closeBtn = new WizardDPushButton(this);
    m_closeBtn->setFlat(true);
    m_closeBtn->setIcon(QIcon::fromTheme("uos-ai-assistant_close"));
    m_closeBtn->setIconSize(QSize(20, 20));
    m_closeBtn->setFixedSize(24, 24);

    // Create menu for disable actions
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
    DFontSizeManager::instance()->bind(m_settingLabel, DFontSizeManager::T9, QFont::Normal);

    QWidget *m_settingWidget = new QWidget(this);
    QHBoxLayout* settingLayout = new QHBoxLayout(m_settingWidget);
    settingLayout->setContentsMargins(31, 0, 0, 0);
    settingLayout->setSpacing(0);
    settingLayout->addWidget(m_settingLabel);
    m_settingAction->setDefaultWidget(m_settingWidget);
    m_disableMenu->addAction(m_settingAction);
    connect(m_settingLabel, &QLabel::linkActivated, this, [this] {
        emit signalFunctionTriggered(WordWizard::WIZARD_TYPE_CUSTOM, QPoint(0, 0), false);
    });

    // 添加到布局，与WizardWrapper的布局结构保持一致
    layout->setSpacing(2);
    layout->addWidget(m_lineSep1);
    layout->addWidget(m_lineSep2);
    layout->addSpacing(6);
    layout->addWidget(m_inputEdit,1);  // 输入框占据大部分空间
    layout->addWidget(m_closeBtn);
    layout->addStretch();  // 添加弹性空间
    
    updateTheme(DGuiApplicationHelper::instance()->themeType());
}

void InputWindow::initConnect()
{
    connect(m_inputEdit, &DLineEdit::returnPressed, this, &InputWindow::onSendButtonClicked);
    connect(m_sendBtn, &DIconButton::clicked, this, &InputWindow::onSendButtonClicked);
    connect(m_inputEdit, &DLineEdit::textChanged, this, &InputWindow::onInputTextChanged);
    connect(m_disableMenu, &QMenu::triggered, this, &InputWindow::onDisableMenuTriggered);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, 
            this, &InputWindow::updateTheme);

    connect(m_closeBtn, &WizardDPushButton::clicked, this, [&] {
        emit signalRequestServer();
        adjustSize();
        m_disableMenu->activateWindow();
        m_disableMenu->exec(m_closeBtn->mapToGlobal(m_closeBtn->rect().bottomLeft() + QPoint(0, 5)));
        m_closeBtn->update();
    });

    // 安装事件过滤器
    this->installEventFilter(this);
    m_inputEdit->installEventFilter(this);
    m_sendBtn->installEventFilter(this);
    m_closeBtn->installEventFilter(this);
}

void InputWindow::initAnimation()
{  
    // 初始化展开动画
    m_expandAnimation = new QPropertyAnimation(this, "windowOpacity");
    m_expandAnimation->setDuration(250);
    m_expandAnimation->setStartValue(0.0);
    m_expandAnimation->setEndValue(1.0);
    m_expandAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnim = new QPropertyAnimation(this, "geometry");
    m_scaleAnim->setDuration(250);
    m_scaleAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_scaleAnim, &QPropertyAnimation::finished, this, [this]() {
        this->setFixedSize(m_wizardWidth, m_wizardHeight);
    });
    connect(m_expandAnimation, &QPropertyAnimation::finished, this, &InputWindow::onExpandAnimationFinished);
}

void InputWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    DBlurEffectWidget::keyPressEvent(event);
}

bool InputWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 显示工具栏逻辑
    if (event->type() == QEvent::Enter) {
        m_mouseInside = true;
    } else if (event->type() == QEvent::Leave && watched != m_closeBtn && watched != m_sendBtn && watched != m_inputEdit) {
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
    if (!ESystemContext::isTreeland()) {
        if  (watched == m_inputEdit || watched == m_sendBtn || watched == m_closeBtn) {
            isDraggableWidget = true;
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

void InputWindow::mousePressEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            this->setCursor(Qt::CursorShape::ClosedHandCursor);
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            return;
        }
    }

    DBlurEffectWidget::mousePressEvent(event);
}

void InputWindow::mouseMoveEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland()) {
        if (m_dragging) {
            this->setCursor(Qt::CursorShape::ClosedHandCursor);
            this->move(event->globalPos() - m_dragStartPos);
            return;
        }
    }

    DBlurEffectWidget::mouseMoveEvent(event);
}

void InputWindow::mouseReleaseEvent(QMouseEvent *event)
{
//    qDebug() << event;
    if (!ESystemContext::isTreeland()) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            this->setCursor(Qt::CursorShape::OpenHandCursor);
            return;
        }
    }

    DBlurEffectWidget::mouseReleaseEvent(event);
}

void InputWindow::onDisableMenuTriggered(const QAction *action)
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

    emit signalFunctionTriggered(wizardtype, QPoint(0, 0), isCustom);
}

void InputWindow::isEnabledAction(bool isEnabled)
{
    if (!ESystemContext::isWayland()) {
        m_disableInAppAction->setEnabled(isEnabled);
        m_disableInProcessAction->setEnabled(isEnabled);
    }
    m_disableGlobalAction->setEnabled(isEnabled);
}

void InputWindow::showAtWizardWrapperPosition(const QPoint &pos, const QRect &screenRect, int wizardWidth, int wizardHeight)
{
    m_inputEdit->clear();

    m_showPos = pos;
    m_screenRect = screenRect;
    m_wizardWidth = wizardWidth;
    m_wizardHeight = wizardHeight;
    
    // 不在这里设置固定尺寸，让动画能够正常工作
    // 动画完成后会恢复合适的尺寸
    
    // 确保输入框和按钮可见并调整布局
    m_inputEdit->show();
    m_sendBtn->show();

    // 调整布局以适应新的尺寸
    this->adjustSize();
    
    // 调整位置
    adjustPosition(screenRect);
    
    this->show();
    
    // 开始向右展开动画
    expandWithAnimation();
}

// 新增方法：向右展开动画
void InputWindow::expandWithAnimation()
{
    // 配置展开动画
    this->setWindowOpacity(0.0);
    QRect targetRect = QRect(this->x(),this->y(), m_wizardWidth, m_wizardHeight);

    this->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    this->setGeometry(
                targetRect.x() + targetRect.width()*0.05,  // 左移5%宽度（缩小状态）
                targetRect.y() + targetRect.height()*0.05, // 上移5%高度（缩小状态）
                targetRect.width()*0.9,
                targetRect.height()*0.9
                );
    m_expandAnimation->setStartValue(0.0);
    m_expandAnimation->setEndValue(1.0);
    
    // 连接动画完成信号，恢复固定尺寸设置
    connect(m_expandAnimation, &QPropertyAnimation::finished, this, [this]() {
        // 断开连接，避免重复执行
        disconnect(m_expandAnimation, &QPropertyAnimation::finished, this, nullptr);
        
        // 调用展开动画完成回调
        onExpandAnimationFinished();
    });

    // 2. 缩放动画（0.9→1）
    m_scaleAnim->setStartValue(this->geometry()); // 缩小的状态
    m_scaleAnim->setEndValue(targetRect); // 最终目标大小

    // 启动动画
    m_expandAnimation->start();
    m_scaleAnim->start();
}

void InputWindow::adjustPosition(const QRect &screenRect)
{
    // 计算窗口应该显示的位置
    QPoint pos = m_showPos;
    
    // 确保窗口完全在屏幕内
    QRect widgetRect(pos.x(), pos.y(), m_wizardWidth, m_wizardHeight);
    
    if (widgetRect.right() > screenRect.right()) {
        pos.setX(screenRect.right() - widgetRect.width());
    }
    if (widgetRect.bottom() > screenRect.bottom()) {
        pos.setY(screenRect.bottom() - widgetRect.height());
    }
    if (widgetRect.left() < screenRect.left()) {
        pos.setX(screenRect.left());
    }
    if (widgetRect.top() < screenRect.top()) {
        pos.setY(screenRect.top());
    }
    
    this->move(pos);
}

void InputWindow::updateTheme(const DGuiApplicationHelper::ColorType &themeType)
{
    // 主题更新逻辑可以在这里添加
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    if (themeType == DGuiApplicationHelper::LightType) {
        if(m_lineSep1) {
            DPalette sepPalette = m_lineSep1->palette();
            sepPalette.setColor(DPalette::Window, QColor(0, 0, 0, int(255 * 0.2)));
            m_lineSep1->setPalette(sepPalette);
            m_lineSep2->setPalette(sepPalette);
        }
        QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);
        setMaskColor(backgroundColor);
        setMaskAlpha(255);
    } else {
        if(m_lineSep1) {
            DPalette sepPalette = m_lineSep1->palette();
            sepPalette.setColor(DPalette::Window, QColor(255, 255, 255, int(255 * 0.2)));
            m_lineSep1->setPalette(sepPalette);
            m_lineSep2->setPalette(sepPalette);
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

void InputWindow::onSendButtonClicked()
{
    QString inputText = m_inputEdit->text().trimmed();
    if (!inputText.isEmpty()) {
        QString clipboardText = QApplication::clipboard()->text();
        emit signalInputTextTriggered(inputText, clipboardText);
    }
}

void InputWindow::onInputTextChanged()
{
    QString text = m_inputEdit->text().trimmed();
    m_sendBtn->setEnabled(!text.isEmpty());
}

void InputWindow::onExpandAnimationFinished()
{
    // 聚焦到输入框
    m_inputEdit->setFocus();
    this->activateWindow();
}

void InputWindow::onCloseWidget()
{
    qCDebug(logWordWizard) << "Closing widget";
    this->close();
    m_isVisible = false;
    emit signalCloseBtnClicked();
}

bool InputWindow::isFirstClose()
{
    return DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLOSE).toBool();
}

QAction* InputWindow::createCustomMenuAction(const QString& text, const QString& iconName)
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

void InputWindow::updateHoverStyle(QWidget* widget, bool hovered)
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
