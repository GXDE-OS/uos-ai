#ifndef AIASSISTANTMAINWINDOWPROXY_H
#define AIASSISTANTMAINWINDOWPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AiassistantSubstitute;
class AiassistantMainWindowProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.mainWindow")
public:
    inline QString proxyPath() const{
        return "/aiassistant/deepinmain";
    }
public:
    AiassistantMainWindowProxy(AiassistantSubstitute* parent);
    ~AiassistantMainWindowProxy();
public:
    //听写接口设置
    void SpeechToText();

    //翻译接口设置
    void TextToTranslate();
public slots:
    //合成接口设置
    void TextToSpeech();

private:
    QString getSelectedString();
    AiassistantSubstitute *q;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTMAINWINDOWPROXY_H
