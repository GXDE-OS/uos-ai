#include "audioaiassistantsetting.h"
#include "private/eaiexecutor.h"
#include "audiowizard/gui/iatwidget.h"
#include <dconfigmanager.h>

#include <QSettings>
#include <QApplication>
#include <QThread>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AudioAiassistantSetting::AudioAiassistantSetting(QObject *parent) : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

AudioAiassistantSetting *AudioAiassistantSetting::instance()
{
    static AudioAiassistantSetting set;
    return &set;
}

void AudioAiassistantSetting::setTTSEnable(bool enable)
{
    DConfigManager::instance()->setValue(TTS_GROUP, "enable", enable);
}

bool AudioAiassistantSetting::getTTSEnable()
{
    bool enabled = DConfigManager::instance()->value(TTS_GROUP, "enable", true).toBool();
    qCDebug(logAudioWizard) << "Retrieved TTS enable state:" << enabled;
    return enabled;
}

void AudioAiassistantSetting::setEnableWindow(bool enable)
{
    DConfigManager::instance()->setValue(TTS_GROUP, "enableWindow", enable);
}

bool AudioAiassistantSetting::getEnableWindow()
{
    return DConfigManager::instance()->value(TTS_GROUP, "enableWindow", true).toBool();
}

void AudioAiassistantSetting::setIatEnable(bool on)
{
    qCDebug(logAudioWizard) << "Setting IAT enable to:" << on;
    DConfigManager::instance()->setValue(IAT_GROUP, "enable", on);
}

bool AudioAiassistantSetting::getIatEnable()
{
    bool enabled = DConfigManager::instance()->value(IAT_GROUP, "enable", true).toBool();
    qCDebug(logAudioWizard) << "Retrieved IAT enable state:" << enabled;
    return enabled;
}

void AudioAiassistantSetting::setIatLanguage(QString language)
{
    qCDebug(logAudioWizard) << "Setting IAT language to:" << language;
    DConfigManager::instance()->setValue(IAT_GROUP, "language", language);
}

QString AudioAiassistantSetting::getIatLanguage()
{
    QString defaultv = "zh_cn";
    return DConfigManager::instance()->value(IAT_GROUP, "language", defaultv).toString();
}

bool AudioAiassistantSetting::setEos(int eos)
{
    if(eos < 0 || eos > 10000)
    {
        qCWarning(logAudioWizard) << "Invalid EOS value:" << eos;
        return false;
    }

    qCDebug(logAudioWizard) << "Setting EOS to:" << eos;
    DConfigManager::instance()->setValue(IAT_GROUP, "eos", eos);
    return true;
}

int  AudioAiassistantSetting::getEos()
{
    int defaultv = 10000;
    return DConfigManager::instance()->value(IAT_GROUP, "eos", defaultv).toInt();
}

bool AudioAiassistantSetting::setBos(int bos)
{
    if(bos < 0 || bos > 10000)
    {
        return false;
    }

    DConfigManager::instance()->setValue(IAT_GROUP, "bos", bos);
    return true;
}

int  AudioAiassistantSetting::getBos()
{
    int defaultv = 10000;
    return DConfigManager::instance()->value(IAT_GROUP, "bos", defaultv).toInt();
}

bool  AudioAiassistantSetting::setBosWarning(int warningTime)
{
    if(warningTime < 0 || warningTime > 10000)
    {
        return false;
    }

    DConfigManager::instance()->setValue(IAT_GROUP, "BosWarning", warningTime);
    return true;
}

int  AudioAiassistantSetting::getBosWarning()
{
    int defaultv = 5000;
    return DConfigManager::instance()->value(IAT_GROUP, "BosWarning", defaultv).toInt();
}

QVariantMap AudioAiassistantSetting::getIatParam()
{
    //params_str = "sub = iat, domain = iat, dwa=wpgs, language = zh_cn, accent = mandarin, sample_rate = 16000, eos=3000, result_type = json, result_encoding = utf8";
    QVariantMap param;
    param["sub"] = QString("iat");
    param["dwa"] = QString("wpgs");
    param["language"] = getIatLanguage();
    param["accent"] = QString("mandarin");
    param["sample_rate"] = QString("16000");
    param["vad_eos"] = QString::number(getEos());
    param["vad_bos"] = QString::number(getBos());
    param["result_type"] = QString("json");
    param["result_encoding"] = QString("utf8");
    return param;
}

void AudioAiassistantSetting::sync()
{
}
