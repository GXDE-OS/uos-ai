#ifndef DISABLEAPPWIDGET_H
#define DISABLEAPPWIDGET_H
#include "uosai_global.h"

#include <QVector>
#include <QString>
#include <QScrollArea>
#include <QGridLayout>

#include <DWidget>
#include <DLabel>
#include <DIconButton>

namespace uos_ai {
class DisableAppWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit DisableAppWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~DisableAppWidget();

    bool isEmpty() const { return m_disableAppList.isEmpty(); }
    void addApp(const QString &appName);
    void clearApps();
    void updateLayout();
    QString getAppDisplayName(const QString &appName);
    QString getAppIconName(const QString &appName);
    QString processIconName(const QString &iconName);
    QStringList getAppInfos() const { return m_disableAppList; }

signals:
    void requestRemoveApp(const QString &appName);
    void becameEmpty();
    void appAdded(const QString &appName);
    void requestDisabledAppsUpdate(const QStringList &appList);

private slots:
    void removeDisableApp(const QString &appName);

private:
    void initUI();
    void initConnect();
    void adjustGridLayout();
    void clearLayout();
    void checkMouseHoverOnItems(const QPoint &globalPos);

    DTK_WIDGET_NAMESPACE::DLabel *m_pTitleLabel;
    DTK_WIDGET_NAMESPACE::DWidget *m_scrollContent;
    QScrollArea *m_scrollArea;
    QGridLayout *m_gridLayout;
    QStringList m_disableAppList;
    QVector<QWidget *> m_itemWidgets;
};
}

#endif // DISABLEAPPWIDGET_H
