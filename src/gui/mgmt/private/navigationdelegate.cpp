#include "navigationdelegate.h"

#include <DApplication>
#include <DFontSizeManager>

#include <QPainter>
#include <QDebug>

UOSAI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

NavigationDelegate::NavigationDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
{
}

NavigationDelegate::~NavigationDelegate()
{
}

void NavigationDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.text = QString();
    DStyledItemDelegate::paint(painter, opt, index);
    auto level = static_cast<NavLevel>(index.data(NavLevelRole).toInt());
    bool isSelected = option.state & QStyle::State_Selected;

    // draw text
    switch (level) {
    case Level1: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::BrightText);
        painter->setPen(pen);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5, QFont::Medium, opt.font));
        QRect rect = opt.rect.marginsRemoved(QMargins(20, 8, 20, 7));
        auto text = painter->fontMetrics().elidedText(index.data().toString(), Qt::ElideRight, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case Level2: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::WindowText);
        painter->setPen(pen);
        auto rect = option.rect.marginsRemoved(QMargins(30, 8, 30, 8));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width());
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6, QFont::Medium, opt.font));
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case Split:
    case Level3:
        break;
    }
}

void NavigationDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    DStyledItemDelegate::initStyleOption(option, index);
    option->features &= ~QStyleOptionViewItem::HasDisplay;

    auto level = static_cast<NavLevel>(index.data(NavLevelRole).toInt());

    if (level == Level1) {
        option->font = DFontSizeManager::instance()->get(DFontSizeManager::T4, option->font);
        option->font.setBold(true);
        option->fontMetrics = QFontMetrics(option->font);
    }
}

QSize NavigationDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize = QStyledItemDelegate::sizeHint(option, index);
    int height = defaultSize.height() + 12;
    if (height < 36)
        height = 36;
    return QSize(defaultSize.width(), height);
}
