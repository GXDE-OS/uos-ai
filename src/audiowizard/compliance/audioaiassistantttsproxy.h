#ifndef AUDIOAIASSISTANTTTSPROXY_H
#define AUDIOAIASSISTANTTTSPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AudioAiassistant;
class AudioAiassistantTtsProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.tts")
public:
    inline QString proxyPath() const {
        return "/aiassistant/tts";
    }
public:
    explicit AudioAiassistantTtsProxy(AudioAiassistant* parent);
    ~AudioAiassistantTtsProxy();
public slots:
    void setTTSEnable(bool enable);
    bool getTTSEnable();
    void setEnableWindow(bool enable);
    bool getEnableWindow();
    bool isTTSInWorking();
    void stopTTSDirectly();
private:
    AudioAiassistant *q;

};

UOSAI_END_NAMESPACE

#endif // AUDIOAIASSISTANTTTSPROXY_H
