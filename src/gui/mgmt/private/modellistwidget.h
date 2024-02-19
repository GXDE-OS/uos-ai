#ifndef MODELLISTWIDGET_H
#define MODELLISTWIDGET_H

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>

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
    DBackgroundGroup *m_pNoModelWidget = nullptr;
    DBackgroundGroup *m_pHasModelWidget = nullptr;
    DCommandLinkButton *m_pEditButton = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;
};

#endif // MODELLISTWIDGET_H
