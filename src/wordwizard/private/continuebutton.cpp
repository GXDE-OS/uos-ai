#include "continuebutton.h"

#include <QPainter>
#include <QPainterPath>

#include <DPalette>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

ContinueButton::ContinueButton(QWidget *parent) : DPushButton(parent) {

}

void ContinueButton::paintEvent(QPaintEvent* event) {
    DPalette parentP = DGuiApplicationHelper::instance()->applicationPalette();

    QRectF rect = this->rect();
    QPainter pa(this);
    pa.setRenderHints(QPainter::Antialiasing);
    QColor backgroundColor = QColor(255, 255, 255, int(255 * 0.5));
    QColor borderColor     = QColor(0, 0, 0, int(255 * 0.05));
    QColor textColor       = this->palette().color(DPalette::Normal, DPalette::Text);
    textColor.setAlphaF(0.3);
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        backgroundColor = QColor(255, 255, 255, int(255 * 0.15));
    }
    if (this->isEnabled() && m_isHover) {
        borderColor = parentP.color(DPalette::Normal, DPalette::Highlight);
    }

    QPen pen(borderColor);
    pen.setWidth(1);
    if (this->isEnabled() && m_isHover) {
        pen.setWidth(2);
    }
    pa.setPen(pen);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawRoundedRect(2, 2, int(rect.width() - 4), int(rect.height() - 4), 8, 8);

    // 文本
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setAlignment(Qt::AlignVCenter);
    QRectF textRect = rect;
    textRect.setLeft(10);
    pa.setPen(textColor);
    pa.drawText(textRect, this->text(), textOption);

    // icon
    pa.setPen(Qt::PenStyle::NoPen);
    if (!(this->isEnabled() && m_isHover)) {
        pa.setOpacity(0.7);
    }
    pa.setBrush(QBrush(parentP.color(DPalette::Normal, DPalette::Highlight)));
    int iconRadius = 12;
    QPoint centerPoint(int(rect.right() - 10 - 12), int(rect.height() / 2));
    pa.drawEllipse(centerPoint, iconRadius, iconRadius);
    QPoint startPoint(centerPoint.x() - this->iconSize().width() / 2, centerPoint.y() - this->iconSize().height() / 2);
    pa.drawPixmap(startPoint, this->icon().pixmap(this->iconSize()));
    pa.setOpacity(1);
}

void ContinueButton::mousePressEvent(QMouseEvent *event)
{
    m_isPress = true;
    update();
    return DPushButton::mousePressEvent(event);
}

void ContinueButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_isPress = false;
    update();
    return DPushButton::mouseReleaseEvent(event);
}

#ifdef COMPILE_ON_QT6
void ContinueButton::enterEvent(QEnterEvent *event)
#else
void ContinueButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DPushButton::enterEvent(event);
}

void ContinueButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DPushButton::leaveEvent(event);
}

void ContinueButton::showEvent(QShowEvent *event)
{
    return DPushButton::showEvent(event);
}
