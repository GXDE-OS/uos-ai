#ifndef LOCALMODELLISTWIDGET_H
#define LOCALMODELLISTWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>

DWIDGET_USE_NAMESPACE

class ThemedLable;

class LocalModelListWidget: public DWidget
{
    Q_OBJECT

protected:
    enum LocalModel {
        TextToImage,
        SpeechRecognition
    };

public:
    explicit LocalModelListWidget(DWidget *parent = nullptr);
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

    bool checkModelExist(const LocalModel);

    DBackgroundGroup *textToImageWidget();
    DBackgroundGroup *speechRecognitionWidget();
    DBackgroundGroup *localLLMWidget();
    DBackgroundGroup *embeddingPluginsWidget();

    DBackgroundGroup *yourong1_5BLLMWidget();
    DBackgroundGroup *yourong7BLLMWidget();
    DBackgroundGroup *deepseek_1_5BLLMWidget();

    bool updateLocalModelSwitch(const LocalModel, bool);

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DBackgroundGroup *m_pTextToImageWidget = nullptr;
    DBackgroundGroup *m_pSpeechRecognitionWidget = nullptr;
    DBackgroundGroup *m_pLocalLlmWidget = nullptr;
    DBackgroundGroup *m_pEmbeddingPluginsWidget = nullptr;
    DBackgroundGroup *m_pYouRong1_5B = nullptr;
    DBackgroundGroup *m_pYouRong7B = nullptr;
    DBackgroundGroup *m_pDeepseek1_5B = nullptr;

    bool m_isShowRedPoint = false;
};

#endif // LOCALMODELLISTWIDGET_H
