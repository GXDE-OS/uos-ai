#include "languageComboDelegate.h"

#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <DComboBox>

#include <DPalette>
#include <DGuiApplicationHelper>
#include <DStyle>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

void LanguageComboDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    // Get the text from the model
    QString text = index.data(Qt::DisplayRole).toString();

    // Set up the rectangle for drawing
    QRect rect = option.rect;

    // Draw background if selected
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else {
#ifdef COMPILE_ON_QT6
        painter->fillRect(rect, option.palette.window());
#else
        painter->fillRect(rect, option.palette.background());
#endif
        painter->setPen(option.palette.text().color());
    }

    // Calculate text positions with 32px right offset
    int lineHeight = rect.height() / 2;
    QRect topRect = rect.adjusted(32, 0, 0, 0); // Right offset 32px
    topRect.setHeight(lineHeight);
    if (index.row() == 2) {
        topRect.setHeight(rect.height() - m_index1SmallHeight - 1);
    }
    QRect bottomRect = rect.adjusted(32, 0, 0, 0); // Right offset 32px
    bottomRect.setTop(topRect.bottom());
    bottomRect.setHeight(lineHeight);
    if (index.row() == 2) {
        bottomRect.setTop(topRect.bottom());
        bottomRect.setHeight(m_index1SmallHeight + 1);
    }
    // Draw check icon for current index item
    if (m_currentIndex == index.row()) {
        QRect iconRect(rect.left() + 10, rect.top() + lineHeight - 17, 16, 16);
        
        // Use theme icon instead of self-drawn path
        QIcon checkIcon = QIcon::fromTheme("uos-ai-assistant_checkmark");
        if (!checkIcon.isNull()) {
            if (option.state & QStyle::State_Selected) {
                checkIcon.paint(painter, iconRect, Qt::AlignCenter, checkIcon.Selected);
            } else {
                checkIcon.paint(painter, iconRect, Qt::AlignCenter, checkIcon.Normal);
            }
        }
    }


    // Draw first line with normal font
    QFont normalFont = option.font;

    // Store font for index 1
    if (index.row() == 1) {
        m_index1Font = normalFont;
    }

    painter->setFont(normalFont);
    painter->drawText(topRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    // Draw second line with smaller font and index
    QFont smallFont;
    if (index.row() == 2 && !m_index1Font.isCopyOf(QFont())) {
        // Use index 1 font for smallFont when index is 2
        smallFont = m_index1Font;
        smallFont.setPointSize(m_index1Font.pointSize() - 2);
    } else {
        // Use current normalFont for other cases
        smallFont = normalFont;
        smallFont.setPointSize(normalFont.pointSize() - 2);
    }
    painter->setFont(smallFont);
    painter->setOpacity(0.7); // Slightly transparent for the smaller text

    // Add index number after the text
    QString smallText;
    switch (index.row()) {
    case 0:
        smallText = QString(QObject::tr("Simplified Chinese"));
        break;
    case 1:
        smallText = QString(QObject::tr("Traditional Chinese"));
        break;
    case 2:
        smallText = QString(QObject::tr("Tibetan"));
        break;
    case 3:
        smallText = QString(QObject::tr("English"));
        break;
    case 4:
        smallText = QString(QObject::tr("Japanese"));
        break;
    case 5:
        smallText = QString(QObject::tr("German"));
        break;
    case 6:
        smallText = QString(QObject::tr("Spanish"));
        break;
    case 7:
        smallText = QString(QObject::tr("French"));
        break;
    case 8:
        smallText = QString(QObject::tr("Italian"));
        break;
    case 9:
        smallText = QString(QObject::tr("Korean"));
        break;
    case 10:
        smallText = QString(QObject::tr("Malay"));
        break;
    case 11:
        smallText = QString(QObject::tr("Portuguese"));
        break;
    case 12:
        smallText = QString(QObject::tr("Russian"));
        break;
    case 13:
        smallText = QString(QObject::tr("Thai"));
        break;
    case 14:
        smallText = QString(QObject::tr("Vietnamese"));
        break;
    case 15:
        smallText = QString(QObject::tr("Arabic (SA)"));
        break;
    default:
        break;
    }

    painter->drawText(bottomRect, Qt::AlignVCenter | Qt::AlignLeft, smallText);

    painter->restore();
}

QSize LanguageComboDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    // Double the height to accommodate two lines
    if (index.row() == 1) {
        m_index1SmallHeight = size.height();
    }
    if (index.row() == 2) {
        size.setHeight(size.height() + m_index1SmallHeight + 2);
    } else {
        size.setHeight(size.height() * 2 + 2);
    }
    return size;
}
