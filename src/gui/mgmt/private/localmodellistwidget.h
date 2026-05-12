#ifndef LOCALMODELLISTWIDGET_H
#define LOCALMODELLISTWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>

namespace uos_ai {

class ThemedLable;

class LocalModelListWidget: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit LocalModelListWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    void updateLocalModelList();
    void clearRedPoint();
    void stopDownload();
    QString getTitleName();

signals:
    void sigRedPointVisible(bool);

private slots:
    void onThemeTypeChanged();
    void onSetRedPointVisible(bool);

private:
    void initUI();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *embeddingPluginsWidget();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *yourong1_5BLLMWidget();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *yourong7BLLMWidget();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *deepseek_1_5BLLMWidget();
private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pEmbeddingPluginsWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pYouRong1_5B = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pYouRong7B = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pDeepseek1_5B = nullptr;

    bool m_isShowRedPoint = false;
};

}

#endif // LOCALMODELLISTWIDGET_H
