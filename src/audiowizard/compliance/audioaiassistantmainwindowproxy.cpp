#include "audioaiassistantmainwindowproxy.h"
#include "audioaiassistantsetting.h"
#include "audioaiassistant.h"
#include "serverwrapper.h"
#include "networkmonitor.h"
#include "networkdefs.h"
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AudioAiassistantMainWindowProxy::AudioAiassistantMainWindowProxy(AudioAiassistant *parent)
    : QObject(parent)
    , q(parent)
{
    connect(q,&AudioAiassistant::onNotify,this,&AudioAiassistantMainWindowProxy::onNotify,Qt::DirectConnection);
    
    // 监听网络状态变化
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged, 
            this, &AudioAiassistantMainWindowProxy::onNetworkStateChanged);
}

AudioAiassistantMainWindowProxy::~AudioAiassistantMainWindowProxy()
{

}

void AudioAiassistantMainWindowProxy::onNetworkStateChanged(bool online)
{
    if (!online) {
        qCWarning(logAudioWizard) << "Network disconnected, sending dbus error";
        // 发送网络断开的dbus错误信号
        stopAsr();
        QJsonDocument doc;
        QJsonObject json = doc.object();
        json["code"] = "900003";
        json["failType"] = 99;
        json["status"] = -1;
        json["descInfo"] = "network error";
        json["text"] = "";
        QString msg = QString(QJsonDocument(json).toJson());
        emit onNotify(msg);
        qCDebug(logAudioWizard) << "Emitting onNotify with result JSON";
    }
}

void AudioAiassistantMainWindowProxy::SpeechToText()
{
    qCDebug(logAudioWizard) << "Initiating speech-to-text conversion";
    q->speechToText();
}

void AudioAiassistantMainWindowProxy::TextToSpeech()
{
    qCDebug(logAudioWizard) << "Initiating text-to-speech conversion";
    q->textToSpeech();
}

//转写
QString AudioAiassistantMainWindowProxy::startAsr(const QVariantMap &param)
{
    qCDebug(logAudioWizard) << "Starting ASR with parameters:" << param.keys();
    if (!NetworkMonitor::getInstance().isOnline()) {
        QJsonDocument doc;
        QJsonObject json = doc.object();
        json["code"] = "900003";
        json["failType"] = 99;
        json["status"] = -1;
        json["descInfo"] = "network error";
        json["text"] = "";
        QString msg = QString(QJsonDocument(json).toJson());
        emit onNotify(msg);
        return "";
    }
    return q->startAsr(param);
}

void AudioAiassistantMainWindowProxy::stopAsr()
{
    qCDebug(logAudioWizard) << "Stopping ASR";
    q->stopAsr();
}

void AudioAiassistantMainWindowProxy::TextToTranslate()
{
    //随航翻译快捷键
    qCDebug(logAudioWizard) << "Initiating text translation";
    emit ServerWrapper::instance()->sigToTranslate();
}

