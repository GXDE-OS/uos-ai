#ifndef MODELLISTWIDGET_H
#define MODELLISTWIDGET_H

#include <QFutureWatcher>
#include <QtConcurrent>

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

#include "themedlable.h"
#include "tasdef.h"
#include "networkdefs.h"
#include "iconcommandlinkbutton.h"

DWIDGET_USE_NAMESPACE

struct LLMServerProxy;

class ModelListWidget: public DWidget
{
    Q_OBJECT

public:
    explicit ModelListWidget(DWidget *parent = nullptr);

    void setModelList(const QList<LLMServerProxy> &);
    void removeModel(const LLMServerProxy &);

    void resetEditButton();
    QString getTitleName();
    void checkActivityExists();

    void hiddenGetFreeAccountButton();

private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();

public slots:
    void onAppendModel(const LLMServerProxy &);
    void onGetFreeAccount(); //领取免费账户

signals:
    void signalAddModel();
    void signalGetFreeAccountClicked();//免费获取账号按键被点击

private:
    void initUI();
    void initConnect();

    void adjustWidgetSize();

    DWidget *noModelWidget();
    DWidget *hasModelWidget();

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DBackgroundGroup *m_pNoModelWidget = nullptr;
    DBackgroundGroup *m_pHasModelWidget = nullptr;
    DCommandLinkButton *m_pEditButton = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;
    uos_ai::IconCommandLinkButton *m_pGetFreeAccountButton = nullptr;//领取免费账号按钮

    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;
};

#endif // MODELLISTWIDGET_H
