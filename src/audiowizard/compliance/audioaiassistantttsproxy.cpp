#include "audioaiassistantttsproxy.h"
#include "audioaiassistantsetting.h"
#include "audioaiassistant.h"
#include <QDebug>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AudioAiassistantTtsProxy::AudioAiassistantTtsProxy(AudioAiassistant *parent)
    : QObject(parent)
    , q(parent)
{
}

AudioAiassistantTtsProxy::~AudioAiassistantTtsProxy()
{

}

void AudioAiassistantTtsProxy::setTTSEnable(bool enable)
{
    qCDebug(logAudioWizard) << "Setting TTS enable to:" << enable;
    AudioAiassistantSetting::instance()->setTTSEnable(enable);
}

bool AudioAiassistantTtsProxy::getTTSEnable()
{
    bool enabled = AudioAiassistantSetting::instance()->getTTSEnable();
    return enabled;
}

void AudioAiassistantTtsProxy::setEnableWindow(bool enable)
{
    qCDebug(logAudioWizard) << "Setting window enable to:" << enable;
    AudioAiassistantSetting::instance()->setEnableWindow(enable);
}

bool AudioAiassistantTtsProxy::getEnableWindow()
{
    return AudioAiassistantSetting::instance()->getEnableWindow();
}

bool AudioAiassistantTtsProxy::isTTSInWorking()
{
    bool working = q->isTTSInWorking();
    return working;
}

void AudioAiassistantTtsProxy::stopTTSDirectly()
{
    qCDebug(logAudioWizard) << "Stopping TTS directly";
    return q->stopTTSDirectly();
}
