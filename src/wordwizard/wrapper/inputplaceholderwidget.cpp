#include "inputplaceholderwidget.h"
#include "dconfigmanager.h"
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <DFontSizeManager>

#ifdef COMPILE_ON_QT6
#include <QEnterEvent>
#endif

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace uos_ai {

InputPlaceholderWidget::InputPlaceholderWidget(QWidget *parent)
    : DWidget(parent)
    , m_isPressed(false)
    , m_isHovered(false)
{
    setFixedSize(64, 30);
    setCursor(Qt::PointingHandCursor);

    // 从配置中读取是否已经点击过
    m_hasBeenClicked = !DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLICKEDASKAI, true).toBool();

    // 创建水平布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(6, 0, 0, 1);
    layout->setAlignment(Qt::AlignLeft);

    // 创建文案标签
    m_textLabel = new QLabel(tr("Ask AI"), this);
    m_textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);  // 设置文本居中对齐
    DFontSizeManager::instance()->bind(m_textLabel, DFontSizeManager::T7, QFont::Normal);
    // 添加到布局
    layout->addWidget(m_textLabel, 0, Qt::AlignVCenter | Qt::AlignLeft);
    
    updateTheme();
    
    // 连接主题变化信号
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &InputPlaceholderWidget::updateTheme);
}

void InputPlaceholderWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    
    // 获取当前主题
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor;

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        backgroundColor = QColor(0, 0, 0, int(255 * 0.08));
    } else {
        backgroundColor = QColor(255, 255, 255, int(255 * 0.08));
    }

    // 根据状态调整文案，竖线模拟输入光标
    if (m_isPressed) {
        m_textLabel->setText("|");
    } else {
        m_textLabel->setText(tr("Ask AI"));
    }

    // 绘制椭圆背景
    QRect rect = QRect(this->rect().left(),this->rect().top()+3,this->rect().width()-4,this->rect().height()-6);
    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, 6, 6);

    // 绘制右上角红点（如果还没有点击过）
    if (!m_hasBeenClicked) {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        int dotSize = 8;
        // 圆心在右上角，所以矩形需要向左向下偏移
        QRect dotRect(rect.right() - 4, rect.top() - 3, dotSize, dotSize);
        painter.drawEllipse(dotRect);
    }
}

void InputPlaceholderWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPressed = true;
        update();
    }
    DWidget::mousePressEvent(event);
}

void InputPlaceholderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isPressed) {
        m_isPressed = false;
        if (!m_hasBeenClicked) {
            m_hasBeenClicked = true;
            DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLICKEDASKAI, false);
        }
        update();
        emit clicked();
    }
    DWidget::mouseReleaseEvent(event);
}

#ifdef COMPILE_ON_QT6
void InputPlaceholderWidget::enterEvent(QEnterEvent *event)
#else
void InputPlaceholderWidget::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;
    update();
    DWidget::enterEvent(event);
}

void InputPlaceholderWidget::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    m_isPressed = false;
    update();
    DWidget::leaveEvent(event);
}

void InputPlaceholderWidget::updateTheme()
{
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    QColor textColor;
    
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        textColor = QColor(0, 0, 0);
    } else {
        textColor = QColor(255, 255, 255);
    }
}

} 
