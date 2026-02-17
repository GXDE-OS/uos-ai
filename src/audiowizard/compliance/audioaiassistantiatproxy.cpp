#include "audioaiassistantiatproxy.h"
#include "audioaiassistantsetting.h"
#include "audioaiassistant.h"
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AudioAiassistantIatProxy::AudioAiassistantIatProxy(AudioAiassistant *parent)
    : QObject(parent)
    , q(parent)
{
}

AudioAiassistantIatProxy::~AudioAiassistantIatProxy()
{

}

void AudioAiassistantIatProxy::setIatEnable(bool on)
{
    qCDebug(logAudioWizard) << "Setting IAT enable to:" << on;
    AudioAiassistantSetting::instance()->setIatEnable(on);
}

bool AudioAiassistantIatProxy::getIatEnable()
{
    return AudioAiassistantSetting::instance()->getIatEnable();
}

void AudioAiassistantIatProxy::setIatLanguage(QString language)
{
    qCDebug(logAudioWizard) << "Setting IAT language to:" << language;
    AudioAiassistantSetting::instance()->setIatLanguage(language);
}

QString AudioAiassistantIatProxy::getIatLanguage()
{
    return AudioAiassistantSetting::instance()->getIatLanguage();
}

bool AudioAiassistantIatProxy::setEos(int eos)
{
   qCDebug(logAudioWizard) << "Setting EOS to:" << eos;
   return  AudioAiassistantSetting::instance()->setEos(eos);
}

int  AudioAiassistantIatProxy::getEos()
{
    return AudioAiassistantSetting::instance()->getEos();
}

bool AudioAiassistantIatProxy::setBos(int bos)
{
   qCDebug(logAudioWizard) << "Setting BOS to:" << bos;
   return  AudioAiassistantSetting::instance()->setBos(bos);
}

int  AudioAiassistantIatProxy::getBos()
{
    return AudioAiassistantSetting::instance()->getBos();
}

bool  AudioAiassistantIatProxy::setBosWarning(int warningTime)
{ 
    qCDebug(logAudioWizard) << "Setting BOS warning time to:" << warningTime;
    return AudioAiassistantSetting::instance()->setBosWarning(warningTime);
}

int  AudioAiassistantIatProxy::getBosWarning()
{
   return AudioAiassistantSetting::instance()->getBosWarning();
}


