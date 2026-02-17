#ifndef NAVIGATIONDELEGATE_H
#define NAVIGATIONDELEGATE_H
#include "uosai_global.h"

#include <QScopedPointer>

#include <DStyle>
#include <DStyledItemDelegate>

namespace uos_ai {
class NavigationDelegatePrivate;
class NavigationDelegate: public DTK_WIDGET_NAMESPACE::DStyledItemDelegate
{
    Q_OBJECT
public:
    NavigationDelegate(QAbstractItemView *parent);
    ~NavigationDelegate();


    enum DataRole {
        NavLevelRole = Dtk::UserRole + 100,
        NavKeyRole,
    };

    enum NavLevel {
        Split = 1001,
        Level1,
        Level2,
        Level3
    };

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
}

#endif //NAVIGATIONDELEGATE_H
