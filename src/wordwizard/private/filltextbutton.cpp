#include "filltextbutton.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

FillTextButton::FillTextButton(const QString &text, QWidget * parent)
    : DPushButton(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setText(text);
    setFlat(true);
    connect(this, &FillTextButton::released, this, &FillTextButton::onButtonReleased);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &FillTextButton::updateRectSize);
}

QSize FillTextButton::sizeHint() const {
    QSize size;
    size.setWidth(this->width());
    QFont font = this->font();
    QFontMetrics fm(font);
    size.setHeight(10 + this->iconSize().height() + 5 + fm.height() + 10);
    return size;
}

void FillTextButton::clearPressColor() {
    m_isPress = false;
    update();
}

void FillTextButton::showEvent(QShowEvent *event)
{
    this->updateRectSize();
    return DPushButton::showEvent(event);
}

void FillTextButton::paintEvent(QPaintEvent* e)
{
    QRectF rect = this->rect();
    QPainter pa(this);
    pa.setRenderHint(QPainter::Antialiasing);
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = QColor(255, 255, 255, 255);
    bool isDark = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
    QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    QColor textColor = parentPb.color(DPalette::Normal, DPalette::BrightText);
    QColor iconColor = textColor;
    QColor borderColor      = QColor(255, 255, 255, int(0.2 * 255));
    QColor borderOuterColor = QColor(0, 0, 0, int(0.03 * 255));
    QColor shadowColor      = QColor(0, 0, 0, int(0.05 * 255));
    if (isDark) {
        borderColor      = QColor(255, 255, 255, int(0.1 * 255));
        borderOuterColor = QColor(0, 0, 0, int(0.1 * 255));
        shadowColor      = QColor(0, 0, 0, int(0.1 * 255));
    }

    QPixmap pixmap = this->icon().pixmap(QSize(64, 64));
    QPainter iconPainter(&pixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    if (isEnabled() && m_isPress) {
        backgroundColor.setAlphaF(0.9);
        if (isDark) {
            backgroundColor.setAlphaF(0.3);
        }
        textColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
        iconColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    } else if (isEnabled() && m_isHover) {
        backgroundColor.setAlphaF(0.7);
        if (isDark) {
            backgroundColor.setAlphaF(0.15);
        }
        textColor.setAlphaF(1);
        iconPainter.setOpacity(1);
    } else {
        backgroundColor.setAlphaF(0.5);
        if (isDark) {
            backgroundColor.setAlphaF(0.05);
        }
        textColor.setAlphaF(0.7);
        iconPainter.setOpacity(0.7);
    }

    if (isEnabled() && !isActiveWindow()) {
        iconPainter.setOpacity(0.4);
        textColor.setAlphaF(0.4);
    }

    iconPainter.fillRect(pixmap.rect(), iconColor);
    iconPainter.end();

    // 阴影
    pa.setPen(shadowColor);
    pa.setBrush(Qt::NoBrush);
    QRect shadowRect(1, int(rect.height() - 1 - 16), 16, 16);
    pa.drawArc(shadowRect, 210 * 16, 60 * 16);
    shadowRect.setRect(int(rect.width() - 2 - 16), int(rect.height() - 1 - 16), 16, 16);
    pa.drawArc(shadowRect, 270 * 16, 60 * 16);
    pa.drawLine(QPoint(10, int(rect.height() - 1)), QPoint(int(rect.width() - 1 - 10), int(rect.height() - 1)));
    // 内边框
    pa.setPen(borderColor);
    pa.setBrush(QBrush(backgroundColor));
    pa.drawRoundedRect(2, 2, int(rect.width() - 4), int(rect.height() - 4 - 1), 7, 7);
    // 外边框
    pa.setPen(borderOuterColor);
    pa.setBrush(Qt::NoBrush);
    pa.drawRoundedRect(1, 1, int(rect.width() - 2), int(rect.height() - 2 - 1), 8, 8);

    QIcon coloredIcon(pixmap);
    pa.drawPixmap((this->width() - this->iconSize().width()) / 2, 10, coloredIcon.pixmap(this->iconSize()));

    pa.setPen(textColor);
    QTextOption textOption = Qt::AlignVCenter | Qt::AlignCenter;
    textOption.setWrapMode(QTextOption::NoWrap);
    QRectF textRect = rect;
    textRect.setX(5);
    textRect.setY(10 + this->iconSize().height() + 5);
    textRect.setWidth(this->width() - 10);
    textRect.setHeight(QFontMetrics(this->font()).height());
    QString showText = this->text();
    QFontMetrics fm(this->font());
    if (fm.horizontalAdvance(showText) > this->width() - 10) {
        do {
            showText = showText.mid(0, showText.length() - 1);
        } while (fm.horizontalAdvance(showText + "…") >= this->width() - 10);
        showText += "…";
    }
    pa.drawText(textRect, showText, textOption);
}

void FillTextButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::MouseButton::LeftButton) {
        return;
    }

    if (m_isPress) {
        m_isPressOnly = false;
    } else {
        m_isPress = true;
        m_isPressOnly = true;
    }
    update();
    return DPushButton::mousePressEvent(e);
}

void FillTextButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::MouseButton::LeftButton) {
        return;
    }

    //m_isPress = false;
    //update();
    m_isPressOnly = false;
    return DPushButton::mouseReleaseEvent(e);
}

#ifdef COMPILE_ON_QT6
void FillTextButton::enterEvent(QEnterEvent *event)
#else
void FillTextButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DPushButton::enterEvent(event);
}

void FillTextButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DPushButton::leaveEvent(event);
}

void FillTextButton::onButtonReleased()
{
    if (m_isPressOnly) {
        m_isPress = false;
        update();
    }
    m_isPressOnly = false;
}

void FillTextButton::updateRectSize() {
    this->setFixedSize(this->sizeHint());
    this->adjustSize();
}
