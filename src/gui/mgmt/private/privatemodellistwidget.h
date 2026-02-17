#ifndef PRIVATEMODELLISTWIDGET_H
#define PRIVATEMODELLISTWIDGET_H
#include "uosai_global.h"
#include <QFutureWatcher>
#include <QtConcurrent>

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

#include "themedlable.h"
#include "tasdef.h"
#include "networkdefs.h"

struct LLMServerProxy;

namespace uos_ai {

class PrivateModelListWidget: public DWidget
{
    Q_OBJECT

public:
    explicit PrivateModelListWidget(DWidget *parent = nullptr);

    void setModelList(const QList<LLMServerProxy> &);
    void removeModel(const LLMServerProxy &);

    void resetEditButton();
    QString getTitleName();

private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();

public slots:
    void onAppendModel(const LLMServerProxy &);

signals:
    void signalAddModel();

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
};
}

#endif // PRIVATEMODELLISTWIDGET_H
