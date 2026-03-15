#include "transbutton.h"

#include <QPainter>
#include <QPainterPath>

#include <DPalette>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

TransButton::TransButton(QWidget * parent)
    : DPushButton(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFlat(true);
    connect(this, &TransButton::released, this, [&] {
        m_isPress = false;
        if (!this->icon().isNull() && !m_isIconNoPressColor) {
            this->setIcon(m_iconOrig);
        }
        this->update();
    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &TransButton::updateRectSize);
}

TransButton::TransButton(const QString &text, QWidget *parent) : TransButton(parent)
{
    setText(text);
}

TransButton::TransButton(const QIcon &icon, const QString &text, QWidget *parent) : TransButton(text, parent)
{
    setIcon(icon);
}

QSize TransButton::sizeHint() const {
    QSize size;
    if (this->text().isEmpty()) { // only icon
        size.setWidth(this->iconSize().width());
        size.setHeight(this->iconSize().height() + 2);
        return size;
    }

    QFont font = this->font();
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text());
    if (this->icon().isNull()) { // only text
        size.setWidth(qMin(textWidth + 10, this->maximumWidth()));
    } else {
        size.setWidth(qMin(this->iconSize().width() + textWidth + 15, this->maximumWidth()));
    }
    size.setHeight(fm.height() > this->iconSize().height() ? fm.height() : this->iconSize().height());
    return size;
}

void TransButton::changeText(const QString &text) {
    this->setText(text);
    this->updateRectSize();
}

void TransButton::paintEvent(QPaintEvent* e)
{
    QRectF rect = this->rect();
    QPainter pa(this);
    // Enable anti-aliasing for smooth edges
    pa.setRenderHint(QPainter::Antialiasing, true);
    pa.setRenderHint(QPainter::SmoothPixmapTransform, true);
    DPalette palette = this->palette();
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = QColor(0, 0, 0, 0);
    QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    QColor textColor = palette.color(DPalette::Normal, DPalette::BrightText);
    if (!isEnabled()) {
        textColor.setAlpha(76);
    } else if (isEnabled() && m_isHover && !m_isIconNoPressColor) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            backgroundColor = QColor(255, 255, 255, 50);
            textColor.setAlpha(255);
        } else {
            backgroundColor = QColor(0, 0, 0, 13);
            textColor.setAlpha(255);
        }
    } else {
        textColor.setAlpha(178);
        if (m_alpha != 1.0) {
            textColor.setAlphaF(m_alpha);
        }
    }
    if (isEnabled() && m_isPress) {
        textColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    }

    if (!m_isNotAcceptFocus && !this->isActiveWindow()) {
        backgroundColor = QColor(0, 0, 0, 0);
        textColor.setAlphaF(0.5);
    }

    int radius = 15;
    QPainterPath path(rect.bottomLeft() + QPointF(radius, 0));

    path.arcTo(QRectF(rect.bottomLeft() - QPointF(0, radius), QSizeF(radius, radius)), -90, -90);
    path.lineTo(rect.topLeft() + QPointF(0, radius));
    path.arcTo(QRectF(rect.topLeft(), QSizeF(radius, radius)), -180, -90);
    path.lineTo(rect.topRight() - QPointF(radius, 0));
    path.arcTo(QRectF(rect.topRight() - QPointF(radius, 0), QSizeF(radius, radius)), 90, -90);
    path.lineTo(rect.bottomRight() - QPointF(0, radius ));
    path.arcTo(QRectF(rect.bottomRight() - QPointF(radius, radius), QSizeF(radius, radius)), 0, -90);
    path.lineTo(rect.bottomLeft() + QPointF(radius, 0));

    pa.setPen(Qt::NoPen);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawPath(path);

    if (this->icon().isNull()) { // only text
        pa.setPen(textColor);
        QTextOption textOption;
        textOption.setWrapMode(QTextOption::NoWrap);
        textOption.setAlignment(Qt::AlignCenter);
        QString elidedText = this->fontMetrics().elidedText(this->text(), Qt::ElideRight, this->width() - 10);
        pa.drawText(rect, elidedText, textOption);
    } else {
        // only icon, no text
        if (text().isEmpty()) {
            pa.drawPixmap(0, 2, this->icon().pixmap(this->iconSize()));
        } else if (m_isIconRight) {
            pa.setPen(textColor);
            QTextOption textOption;
            textOption.setWrapMode(QTextOption::NoWrap);
            textOption.setAlignment(Qt::AlignCenter);
            rect.setWidth(rect.width() - (this->iconSize().width() + 2 + 5));
            QString elidedText = this->fontMetrics().elidedText(this->text(), Qt::ElideRight, this->width() - 15);
            pa.drawText(rect, elidedText, textOption);
            pa.setOpacity(textColor.alphaF());
            pa.drawPixmap(static_cast<int>(rect.width()) + 2, (this->height() - this->iconSize().height()) / 2, this->icon().pixmap(this->iconSize()));
            pa.setOpacity(1.0);
        } else {
            pa.setOpacity(textColor.alphaF());
            pa.drawPixmap(5, (this->height() - this->iconSize().height()) / 2, this->icon().pixmap(this->iconSize()));
            pa.setOpacity(1.0);
            pa.setPen(textColor);
            QTextOption textOption;
            textOption.setWrapMode(QTextOption::NoWrap);
            textOption.setAlignment(Qt::AlignCenter);
            rect.setX(this->iconSize().width() + 2);
            QString elidedText = this->fontMetrics().elidedText(this->text(), Qt::ElideRight, this->width() - 15);
            pa.drawText(rect, elidedText, textOption);
        }
    }
}

void TransButton::mousePressEvent(QMouseEvent *e)
{
    m_isPress = true;
    if (!this->icon().isNull() && !m_isIconNoPressColor) {
        DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
        QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
        this->setIcon(this->setIconColor(m_iconOrig, highlightColor));
    }
    update();
    return DPushButton::mousePressEvent(e);
}

void TransButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_isPress = false;
    if (!this->icon().isNull() && !m_isIconNoPressColor) {
        this->setIcon(m_iconOrig);
    }
    update();
    return DPushButton::mouseReleaseEvent(e);
}

#ifdef COMPILE_ON_QT6
void TransButton::enterEvent(QEnterEvent *event)
#else
void TransButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DPushButton::enterEvent(event);
}

void TransButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DPushButton::leaveEvent(event);
}

void TransButton::showEvent(QShowEvent *event)
{
    if (m_iconOrig.isNull() && !this->icon().isNull()) {
        m_iconOrig = this->icon();
    }
    updateRectSize();
    return DPushButton::showEvent(event);
}

void TransButton::updateRectSize()
{
    this->setMinimumSize(this->sizeHint());
    this->updateGeometry();
}

QIcon TransButton::setIconColor(QIcon icon, QColor color)
{
    QPixmap pixmap = icon.pixmap(this->iconSize());
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    QIcon colorIcon = QIcon(pixmap);
    return colorIcon;
}
