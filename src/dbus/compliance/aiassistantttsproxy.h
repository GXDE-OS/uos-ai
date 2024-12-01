#ifndef AIASSISTANTTTSPROXY_H
#define AIASSISTANTTTSPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AiassistantSubstitute;
class AiassistantTtsProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.tts")
public:
    inline QString proxyPath() const {
        return "/aiassistant/tts";
    }
public:
    explicit AiassistantTtsProxy(AiassistantSubstitute* parent);
    ~AiassistantTtsProxy();
public slots:
    void setTTSEnable(bool enable);
    bool getTTSEnable();
    void setEnableWindow(bool enable);
    bool getEnableWindow();
    bool isTTSInWorking();
    void stopTTSDirectly();
private:
    AiassistantSubstitute *q;

};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTTTSPROXY_H
