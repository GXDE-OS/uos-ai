#ifndef PRIVATEMODELLISTWIDGET_H
#define PRIVATEMODELLISTWIDGET_H
#include <QFutureWatcher>
#include <QtConcurrent>

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

#include "themedlable.h"

namespace uos_ai {

class PrivateModelListWidget: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit PrivateModelListWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void refresh();
    void removeProvider(const QString &id);

    void resetEditButton();
    QString getTitleName();

private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();
    void onEditItemClicked(const QString &id);
    void onAddModel();
private:
    void initUI();
    void initConnect();

    void adjustWidgetSize();

    DTK_WIDGET_NAMESPACE::DWidget *noModelWidget();
    DTK_WIDGET_NAMESPACE::DWidget *hasModelWidget();

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pNoModelWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_pHasModelWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_pEditButton = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_pAddButton = nullptr;
};
}

#endif // PRIVATEMODELLISTWIDGET_H
