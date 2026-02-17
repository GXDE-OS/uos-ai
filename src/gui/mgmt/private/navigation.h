#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <QScopedPointer>
#include <QFrame>

#include <DWidget>
#include <dtkwidget_global.h>

class NavigationPrivate;

namespace uos_ai {
class LIBDTKWIDGETSHARED_EXPORT Navigation : public QFrame
{
    Q_OBJECT
public:
    explicit Navigation(QWidget *parent = 0);
    ~Navigation();

signals:
    void selectedGroup(const QString &key);

public slots:
    void onSelectGroup(const QString &key);
    void updateNavigationTitles(QList<DTK_WIDGET_NAMESPACE::DWidget *> sortTitles);
    void onCurrentChanged(const QModelIndex &current);

private:
    QScopedPointer<NavigationPrivate> d_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(d_ptr), Navigation)
};
}

#endif // NAVIGATION_H
