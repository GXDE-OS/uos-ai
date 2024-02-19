#ifndef LOCALMODELLISTWIDGET_H
#define LOCALMODELLISTWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>

DWIDGET_USE_NAMESPACE

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

private slots:
    void onThemeTypeChanged();

private:
    void initUI();

    bool checkModelExist(const LocalModel);

    DBackgroundGroup *textToImageWidget();
    DBackgroundGroup *speechRecognitionWidget();

    bool updateLocalModelSwitch(const LocalModel, bool);

private:
    DBackgroundGroup *m_pTextToImageWidget = nullptr;
    DBackgroundGroup *m_pSpeechRecognitionWidget = nullptr;
};

#endif // LOCALMODELLISTWIDGET_H
