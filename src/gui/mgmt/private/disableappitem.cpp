#include "disableappitem.h"

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DStyle>

#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QDynamicPropertyChangeEvent>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DisableAppItem::DisableAppItem(const QString &displayName, const QString &iconName, const QString &originalAppName, QWidget *parent)
    : DFrame(parent)
    , m_appName(displayName)
    , m_originalAppName(originalAppName)
    , m_iconName(iconName)
    , m_isHovered(false)
{
    initUI();
    onThemeTypeChanged();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &DisableAppItem::onThemeTypeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, [this](DGuiApplicationHelper::ColorType themeType) {
        update();
    });
    connect(m_deleteBtn, &DIconButton::clicked, this, [this]() {
        qCInfo(logAIGUI) << "Delete button clicked for app:" << m_originalAppName;
        emit deleteClicked(m_originalAppName);
    });
}

DisableAppItem::~DisableAppItem()
{
}

void DisableAppItem::initUI()
{
    setFixedSize(275, 52);
    setFrameRounded(true);
    setFrameShape(DFrame::NoFrame);
    setFocusPolicy(Qt::NoFocus);

    // 确保能够正确响应鼠标悬停事件
    setAttribute(Qt::WA_Hover, true);

    // 水平布局用于排列图标、名称和删除按钮
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(8);

    // 应用图标
    m_iconLabel = new DLabel();
    m_iconLabel->setFixedSize(32, 32);
    updateIconPixmap();

    // 应用名称
    m_nameLabel = new DLabel();
    m_nameLabel->setText(m_appName);
    m_nameLabel->setToolTip(m_appName);
    m_nameLabel->setElideMode(Qt::ElideRight);
    DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, QFont::Medium);

    // 删除按钮
    m_deleteBtn = new DIconButton(this);
    m_deleteBtn->setIcon(QIcon::fromTheme("edit-delete"));
    m_deleteBtn->setFixedSize(24, 24);
    m_deleteBtn->setIconSize(QSize(16, 16));
    m_deleteBtn->setFlat(true);

    // 默认隐藏删除按钮，悬停时显示
    m_deleteBtn->setVisible(false);

    // 添加部件到布局
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_nameLabel, 1);
    layout->addWidget(m_deleteBtn);
}

void DisableAppItem::updateIconPixmap()
{
    auto ratio = qApp->devicePixelRatio();
    const QSize base(32, 32);
    QPixmap pixmap = QIcon::fromTheme(m_iconName).pixmap(
                QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps) ? base : base * ratio);
    pixmap.setDevicePixelRatio(ratio);
    m_iconLabel->setPixmap(pixmap);
}

void DisableAppItem::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed, updating icon for app:" << m_appName;
    updateIconPixmap();
}

void DisableAppItem::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取当前主题类型
    bool isDarkTheme = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;

    // 根据主题类型和鼠标悬停状态设置背景色和边框色
    if (isDarkTheme) {
        if (m_isHovered) {
            // 深色主题悬停状态
            painter.setBrush(QColor(255, 255, 255, 13));  // 10% 白色
            painter.setPen(QPen(QColor(255, 255, 255, 26), 1));  // 15% 白色
        } else {
            // 深色主题普通状态
            painter.setBrush(QColor(255, 255, 255, 8));  // 5% 白色
            painter.setPen(QPen(QColor(255, 255, 255, 15), 1));  // 10% 白色
        }
    } else {
        if (m_isHovered) {
            // 浅色主题悬停状态
            painter.setBrush(QColor(0, 0, 0, 13));  // 5% 黑色
            painter.setPen(QPen(QColor(0, 0, 0, 26), 1));  // 10% 黑色
        } else {
            // 浅色主题普通状态
            painter.setBrush(QColor(0, 0, 0, 8));  // 10% 黑色
            painter.setPen(QPen(QColor(0, 0, 0, 15), 1));  // 20% 黑色
        }
    }

    // 绘制圆角背景和边框
    QRect rect = this->rect();
    painter.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 8, 8);

    DFrame::paintEvent(event);
}
#ifdef COMPILE_ON_QT6
void DisableAppItem::enterEvent(QEnterEvent *event)
#else
void DisableAppItem::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;
    m_deleteBtn->setVisible(true);
    update();
    DFrame::enterEvent(event);
}

void DisableAppItem::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    m_deleteBtn->setVisible(false);
    update();
    DFrame::leaveEvent(event);
}

bool DisableAppItem::event(QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        m_isHovered = true;
        m_deleteBtn->setVisible(true);
        update();
    } else if (event->type() == QEvent::Leave) {
        m_isHovered = false;
        m_deleteBtn->setVisible(false);
        update();
    } else if (event->type() == QEvent::DynamicPropertyChange) {
        QDynamicPropertyChangeEvent *propEvent = static_cast<QDynamicPropertyChangeEvent*>(event);
        if (propEvent->propertyName() == "isHovered") {
            if (property("isHovered").toBool()) {
                m_isHovered = true;
                m_deleteBtn->setVisible(true);
                update();
            }
        }
    }
    return DFrame::event(event);
}

void DisableAppItem::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        update();
    }

    DFrame::changeEvent(event);
}

void DisableAppItem::setAppName(const QString &name)
{
    m_appName = name;
    m_nameLabel->setText(name);
}

void DisableAppItem::setAppIcon(const QString &iconName)
{
    m_iconName = iconName;
    updateIconPixmap();
}

QString DisableAppItem::appName() const
{
    return m_appName;
}

