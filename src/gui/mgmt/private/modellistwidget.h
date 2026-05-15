#ifndef MODELLISTWIDGET_H
#define MODELLISTWIDGET_H

#include "themedlable.h"
#include "iconcommandlinkbutton.h"
#include "modelinfo.h"
#include "uosfreeaccounts.h"

#include <QFutureWatcher>
#include <QtConcurrent>

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

namespace uos_ai {

class ModelListWidget: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit ModelListWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void removeProvider(const QString &id);

    void resetEditButton();
    QString getTitleName();
    void checkActivityExists();

    void hiddenGetFreeAccountButton();

    bool hasFreeAccount() const;
public slots:
    void refresh();
    void onGetFreeAccount(); //领取免费账户
    void onAddModel();
private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();
    void onEditItemClicked(const QString &id);

signals:
    void signalGetFreeAccountClicked();//免费获取账号按键被点击

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
    IconCommandLinkButton *m_pGetFreeAccountButton = nullptr;//领取免费账号按钮

    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;
};

}

#endif // MODELLISTWIDGET_H
