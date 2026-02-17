#include "circlebutton.h"

#include <QPainter>
#include <QPainterPath>

#include <DPalette>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

CircleButton::CircleButton(QWidget * parent)
    : DPushButton(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFlat(true);
    connect(this, &CircleButton::released, this, [&] {
        m_isPress = false;
        this->update();
    });
}

QSize CircleButton::sizeHint() const {
    return this->size();
}

void CircleButton::paintEvent(QPaintEvent* e)
{
    QRectF rect = this->rect();
    QPainter pa(this);
    pa.setRenderHint(QPainter::Antialiasing);
    DPalette palette = this->palette();
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    QColor iconColor = QColor(255, 255, 255, 255);
    if (!isEnabled()) {
        backgroundColor.setAlphaF(0.3);
        iconColor.setAlphaF(1.0);
    } else if (m_isPress) {
        backgroundColor.setAlphaF(1.0);
        iconColor.setAlphaF(0.7);
    } else if (m_isHover) {
        backgroundColor.setAlphaF(1.0);
        iconColor.setAlphaF(1.0);
    } else {
        backgroundColor.setAlphaF(0.8);
        iconColor.setAlphaF(1.0);
    }

    if (!this->isActiveWindow()) {
        backgroundColor.setAlphaF(backgroundColor.alphaF() * 0.7);
    }

    int radius = this->width();
    QPainterPath path(rect.bottomLeft() + QPointF(radius, 0));

    path.arcTo(QRectF(rect.bottomLeft() - QPointF(0, radius), QSizeF(radius, radius)), -90, -90);
    path.arcTo(QRectF(rect.topLeft(), QSizeF(radius, radius)), -180, -90);
    path.arcTo(QRectF(rect.topRight() - QPointF(radius, 0), QSizeF(radius, radius)), 90, -90);
    path.arcTo(QRectF(rect.bottomRight() - QPointF(radius, radius), QSizeF(radius, radius)), 0, -90);

    pa.setPen(Qt::NoPen);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawPath(path);

    QPointF iconPoint;
    iconPoint.setX((this->width() - this->iconSize().width()) / 2.0);
    iconPoint.setY((this->width() - this->iconSize().width()) / 2.0 - 0.5);
    pa.setOpacity(iconColor.alphaF());
    pa.drawPixmap(iconPoint, this->icon().pixmap(this->iconSize()));
}

void CircleButton::mousePressEvent(QMouseEvent *e)
{
    m_isPress = true;
    update();
    return DPushButton::mousePressEvent(e);
}

void CircleButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_isPress = false;
    update();
    return DPushButton::mouseReleaseEvent(e);
}

#ifdef COMPILE_ON_QT6
void CircleButton::enterEvent(QEnterEvent *event)
#else
void CircleButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DPushButton::enterEvent(event);
}

void CircleButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DPushButton::leaveEvent(event);
}

void CircleButton::showEvent(QShowEvent *event)
{
    if (m_iconOrig.isNull()) {
        m_iconOrig = this->icon();
    }
    return DPushButton::showEvent(event);
}

QIcon CircleButton::setIconColor(QIcon icon, QColor color)
{
    QPixmap pixmap = icon.pixmap(this->iconSize());
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    QIcon colorIcon = QIcon(pixmap);
    return colorIcon;
}
