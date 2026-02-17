#include "wizarddpushbutton.h"

#include <QPainter>
#include <QPainterPath>

#include <DPalette>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

WizardDPushButton::WizardDPushButton(QWidget * parent)
    : DPushButton(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setFlat(true);
    updateRectSize();
    connect(this, &WizardDPushButton::released, this, &WizardDPushButton::onButtonReleased);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &WizardDPushButton::updateRectSize);
}

WizardDPushButton::WizardDPushButton(const QString &text, QWidget * parent)
    : DPushButton(text, parent)
{
    setText(text);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setFlat(true);
    updateRectSize();
    connect(this, &WizardDPushButton::released, this, &WizardDPushButton::onButtonReleased);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &WizardDPushButton::updateRectSize);
}

void WizardDPushButton::paintEvent(QPaintEvent* e)
{
    QRectF rect = this->rect();
    QPainter pa(this);
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);
    QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    QColor textColor = parentPb.color(DPalette::Normal, DPalette::BrightText);

    QColor iconColor = textColor;

    QPixmap pixmap = this->icon().pixmap(QSize(64, 64));
    QPainter iconPainter(&pixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    if (isEnabled() && m_isPress) {
        backgroundColor.setAlphaF(0.9);
        textColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
        iconColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    } else {
        backgroundColor.setAlphaF(0);
    }

    if (isEnabled() && m_isHover) {
        textColor.setAlphaF(1);
        iconPainter.setOpacity(1);
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType)
            backgroundColor = QColor(255, 255, 255, 50);
        else
            backgroundColor = QColor(0, 0, 0, 13);
    } else {
        iconPainter.setOpacity(0.7);
        textColor.setAlphaF(0.7);
    }

    iconPainter.fillRect(pixmap.rect(), iconColor);
    iconPainter.end();

    QIcon coloredIcon(pixmap);

    int radius = 10;
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

    if (text().isNull())
        pa.drawPixmap(2, (this->height() - this->iconSize().height()) / 2, coloredIcon.pixmap(this->iconSize()));
    else {
        pa.drawPixmap(6, (this->height() - this->iconSize().height()) / 2, coloredIcon.pixmap(this->iconSize()));
    }

    pa.setPen(textColor);
    QTextOption textOption = Qt::AlignVCenter | Qt::AlignLeft;
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setAlignment(Qt::AlignCenter);
    rect.setX(this->iconSize().width() + 2);
    int offset = this->rect().height() < 26 ? 1 : 2;
    rect.setY(-offset);

    QString displayText = text();
    if (!displayText.isNull()) {
        QFont font = this->font();
        QFontMetrics fm(font);
        int availableWidth = rect.width() - 12;
        displayText = fm.elidedText(text(), Qt::ElideRight, availableWidth);
    }
    
    pa.drawText(rect, displayText, textOption);
}

void WizardDPushButton::mousePressEvent(QMouseEvent *e)
{
    m_isPress = true;
    update();
    return DPushButton::mousePressEvent(e);
}

void WizardDPushButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_isPress = false;
    update();
    return DPushButton::mouseReleaseEvent(e);
}

void WizardDPushButton::onButtonReleased()
{
    m_isPress = false;
    update();
}

#ifdef COMPILE_ON_QT6
void WizardDPushButton::enterEvent(QEnterEvent *event)
#else
void WizardDPushButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DPushButton::enterEvent(event);
}

void WizardDPushButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DPushButton::leaveEvent(event);
}

void WizardDPushButton::showEvent(QShowEvent *event)
{
    m_isHover = false;
    m_isPress = false;
    updateRectSize();
    update();
    return DPushButton::showEvent(event);
}

void WizardDPushButton::updateRectSize()
{
    if (!text().isNull()) {//图标加文字
        QFont font = this->font();
        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(text());
        int buttonWidth = textWidth + 32;
        
        // 限制最大宽度
        if (buttonWidth > m_maxWidth) {
            buttonWidth = m_maxWidth;
        }
        
        this->setFixedWidth(buttonWidth);
        this->setFixedHeight(fm.height() + 6);
    } else {
        this->setFixedWidth(24);//图标
        this->setFixedHeight(24);
    }
    adjustSize();
}

void WizardDPushButton::setHoverStatus(bool isHover)
{
    m_isHover = isHover;
    update();
}




