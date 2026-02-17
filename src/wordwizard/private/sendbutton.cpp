#include "sendbutton.h"

#include <QPainter>
#include <QPainterPath>

#include <DPalette>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

SendButton::SendButton(QWidget *parent) : DIconButton(parent) {

}

void SendButton::paintEvent(QPaintEvent* event) {
    DPalette parentP = DGuiApplicationHelper::instance()->applicationPalette();

    QRectF rect = this->rect();
    QPainter pa(this);
    pa.setRenderHints(QPainter::Antialiasing);
    
    // 设置背景色和边框色，与 ContinueButton 的 icon 部分一致
    QColor backgroundColor = QColor(255, 255, 255, int(255 * 0.5));
    QColor borderColor = QColor(0, 0, 0, int(255 * 0.05));
    
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        backgroundColor = QColor(255, 255, 255, int(255 * 0.15));
    }
    
    if (this->isEnabled() && m_isHover) {
        borderColor = parentP.color(DPalette::Normal, DPalette::Highlight);
    }

    // 绘制圆形背景，与 ContinueButton 的 icon 部分一致
    pa.setPen(Qt::PenStyle::NoPen);
    
    // 获取基础颜色
    QColor baseColor = parentP.color(DPalette::Normal, DPalette::Highlight);
    QColor adjustedColor = baseColor;
    
    // 禁用状态：opacity: 0.4
    if (!this->isEnabled()) {
        pa.setOpacity(0.4);
    }
    // 悬停状态：filter: brightness(1.2) - 增加亮度
    else if (m_isHover) {
        adjustedColor = QColor(
            qMin(255, int(baseColor.red() * 1.2)),
            qMin(255, int(baseColor.green() * 1.2)),
            qMin(255, int(baseColor.blue() * 1.2)),
            baseColor.alpha()
        );
        pa.setOpacity(1.0);
    }
    // 激活状态：filter: brightness(1.1) - 增加亮度
    else if (m_isPress) {
        adjustedColor = QColor(
            qMin(255, int(baseColor.red() * 1.1)),
            qMin(255, int(baseColor.green() * 1.1)),
            qMin(255, int(baseColor.blue() * 1.1)),
            baseColor.alpha()
        );
        pa.setOpacity(1.0);
    }
    // 正常状态
    else {
        pa.setOpacity(1.0);
    }
    
    pa.setBrush(QBrush(adjustedColor));
    
    int iconRadius = rect.width() / 2;
    QPoint centerPoint(int(rect.width() / 2), int(rect.height() / 2));
    pa.drawEllipse(centerPoint, iconRadius, iconRadius);
    
    // 绘制图标
    QPoint startPoint(centerPoint.x() - this->iconSize().width() / 2, centerPoint.y() - this->iconSize().height() / 2);
    
    // 激活状态时图标颜色：fill: rgba(255, 255, 255, 0.6)
    if (m_isPress && this->isEnabled()) {
        QPixmap iconPixmap = this->icon().pixmap(this->iconSize());
        QPainter iconPainter(&iconPixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        iconPainter.fillRect(iconPixmap.rect(), QColor(255, 255, 255, int(255 * 0.6)));
        pa.drawPixmap(startPoint, iconPixmap);
    } else {
        // 正常状态或非激活状态，使用原始图标颜色
        pa.drawPixmap(startPoint, this->icon().pixmap(this->iconSize()));
    }
    
    pa.setOpacity(1);
}

void SendButton::mousePressEvent(QMouseEvent *event)
{
    m_isPress = true;
    update();
    return DIconButton::mousePressEvent(event);
}

void SendButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_isPress = false;
    update();
    return DIconButton::mouseReleaseEvent(event);
}

#ifdef COMPILE_ON_QT6
void SendButton::enterEvent(QEnterEvent *event)
#else
void SendButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DIconButton::enterEvent(event);
}

void SendButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    m_isPress = false;
    update();
    return DIconButton::leaveEvent(event);
} 
