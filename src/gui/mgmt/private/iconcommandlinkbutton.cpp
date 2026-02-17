#include "iconcommandlinkbutton.h"
#include "wrapcheckbox.h"
#include "private/echatwndmanager.h"
#include "utils/esystemcontext.h"

#include <DGuiApplicationHelper>
#include <DFontSizeManager>

#include <QApplication>
#include <QPixmap>

using namespace uos_ai;

IconCommandLinkButton::IconCommandLinkButton(const QString &text, IconPosition iconPos, DWidget *parent)
    : DCommandLinkButton(text, parent), m_iconPosition(iconPos)
{
    EWndManager()->registeWindow(this);
}

QSize IconCommandLinkButton::sizeHint() const
{
    QSize textSize = fontMetrics().size(0, text());
    QSize size = QSize(this->iconSize().width() + textSize.width() + 10, qMax(this->iconSize().height(), textSize.height()) + 2);
    return size;
}

void IconCommandLinkButton::paintEvent(QPaintEvent *e)
{
    DStyleOptionButton opt;
    initStyleOption(&opt);

    DStylePainter painter(this);

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);

    QPixmap pixmap = this->icon().pixmap(QSize(64, 64));
    QPainter iconPainter(&pixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    bool isDarkType = false;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType)
        isDarkType = true;

    if (isDarkType)
        iconPainter.setOpacity(0.7);

    if (isEnabled() && m_isHover) {
        if (isDarkType)
            iconPainter.setOpacity(1);
        else
            iconPainter.setOpacity(0.7);
    }

    if (!this->isEnabled() || !isActiveWindow()) {
        iconPainter.setOpacity(0.4);
    }

    iconPainter.fillRect(pixmap.rect(), highlightColor);
    iconPainter.end();

    QIcon coloredIcon(pixmap);
    QSize iconSize = this->iconSize();
    QRect rect = opt.rect;
    
    QRect iconRect;
    QRect textRect;
    
    if (m_iconPosition == IconPosition::Left) {
        iconRect = QRect(rect.left() + 3, (rect.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());
        textRect = QRect(rect.left() + iconSize.width() + 5, rect.top(), rect.width() - iconSize.width() - 10, rect.height());
    } else {
        QSize textSize = fontMetrics().size(0, text());
        textRect = QRect(rect.left() + 3, rect.top(), textSize.width(), rect.height());
        iconRect = QRect(rect.left() + textSize.width() + 5, (rect.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());
    }
    
    painter.drawItemPixmap(iconRect, Qt::AlignCenter, coloredIcon.pixmap(this->iconSize()));

    opt.rect = textRect;
    painter.drawControl(DStyle::CE_TextButton, opt);
}

#ifdef COMPILE_ON_QT6
void IconCommandLinkButton::enterEvent(QEnterEvent *event)
#else
void IconCommandLinkButton::enterEvent(QEvent *event)
#endif
{
    m_isHover = true;
    update();
    return DCommandLinkButton::enterEvent(event);
}

void IconCommandLinkButton::leaveEvent(QEvent *event)
{
    m_isHover = false;
    update();
    return DCommandLinkButton::leaveEvent(event);
}
