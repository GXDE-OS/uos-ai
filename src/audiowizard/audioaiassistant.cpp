// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "audioaiassistant_p.h"
#include "utils/util.h"
#include "utils/compliance/atspidesktop.h"
#include "utils/compliance/qselectionmonitor.h"
#include "private/eaiexecutor.h"
#include "gui/iatwidget.h"
#include "wordwizard/wordwizard.h"
#include "private/baseclipboard.h"

#include "compliance/audioaiassistantmainwindowproxy.h"
#include "compliance/audioaiassistantiatproxy.h"
#include "compliance/audioaiassistantttsproxy.h"

#include <QDBusConnection>
#include <QDebug>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>

// Add this at the top of the file with other includes
#include <QLoggingCategory>


#define  EVENT_TIME_INTERVAL 10

// Move this to the top of the file, before any usage
Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

UOSAI_USE_NAMESPACE

AudioAiassistantPrivate::AudioAiassistantPrivate(AudioAiassistant *parent) : q(parent)
{

}

AudioAiassistantPrivate::~AudioAiassistantPrivate()
{
    if (m_tts) {
        delete m_tts;
        m_tts = nullptr;
    }
    if (m_asr) {
        delete m_asr;
        m_asr = nullptr;
    }
}

AudioAiassistant::AudioAiassistant(QObject *parent)
    : QObject(parent)
    , d(new AudioAiassistantPrivate(this))
{
    m_selectclip = BaseClipboard::ttsInstance();
}

AudioAiassistant::~AudioAiassistant()
{
    delete d;
    d = nullptr;
}

bool AudioAiassistant::registerInterface()
{
    qCDebug(logAudioWizard) << "Registering D-Bus interface";
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (!connection.registerService("com.iflytek.aiassistant")){
        qCWarning(logAudioWizard) << "Service 'com.iflytek.aiassistant' already registered";
        return false;
    }

    // init AtspiDesktop
    AudioAiassistantSetting::instance();
    AtspiDesktop::getInstance()->start();
    QSelectionMonitor::getInstance();

    AudioAiassistantMainWindowProxy *mainWin = new AudioAiassistantMainWindowProxy(this);
    connection.registerObject(mainWin->proxyPath(), mainWin, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);

    AudioAiassistantIatProxy* iatdbusServiceProxy = new AudioAiassistantIatProxy(this);
    connection.registerObject(iatdbusServiceProxy->proxyPath(), iatdbusServiceProxy, QDBusConnection::ExportAllSlots);

    AudioAiassistantTtsProxy* ttsdbusServiceProxy = new AudioAiassistantTtsProxy(this);
    connection.registerObject(ttsdbusServiceProxy->proxyPath(), ttsdbusServiceProxy, QDBusConnection::ExportAllSlots);

    qCDebug(logAudioWizard) << "Successfully registered D-Bus interfaces";
    return true;
}

bool AudioAiassistant::isTTSInWorking()
{
    bool ret = false;
    if (d->m_tts)
        ret = d->m_tts->isWorking();
    return ret;
}

void AudioAiassistant::stopTTSDirectly()
{
    if (d->m_tts)
        d->m_tts->stopTTS();
}

// 朗读
void AudioAiassistant::textToSpeech()
{
    if(!EAiExec()->isAudioOutputAvailable()) {
        qCWarning(logAudioWizard) << "Audio output device not available";
        return;
    }

    if (!AudioAiassistantSetting::instance()->getTTSEnable()) {
        qCWarning(logAudioWizard) << "TTS disabled in settings";
        return;
    }

    QString text = m_selectclip->getClipText().trimmed();
    if (text.isEmpty()) {
        qCInfo(logAudioWizard) << "No text selected for TTS";
        Util::playSystemSound_SSE_Error();
        return;
    }

    if (d->m_tts && d->m_tts->isWorking()) {
        qCInfo(logAudioWizard) << "Stopping current TTS session";
        d->m_tts->stopTTS();
        return;
    }

    if (!d->m_tts) {
        qCDebug(logAudioWizard) << "Creating new TTS widget";
        d->m_tts = new TtsWidget();
    }

    qCDebug(logAudioWizard) << "Starting TTS with text length:" << text.length();
    d->m_tts->stopTTS();
    bool windowVisible = AudioAiassistantSetting::instance()->getEnableWindow();
    d->m_tts->startTTS(text, windowVisible);
}

// 听写
void AudioAiassistant::speechToText()
{
    qCDebug(logAudioWizard) << "Starting speech-to-text";
    emit this->sigIatTriggered();

    // 检查目前是否可写
    if (!WordWizard::fcitxWritable()) {
        qCWarning(logAudioWizard) << "Input not writable, will retry";
        QTimer::singleShot(500, [&] {
            if (!WordWizard::fcitxWritable()) {
                qCWarning(logAudioWizard) << "Still not writable after retry";
                return;
            }
            qCDebug(logAudioWizard) << "Creating new IAT widget after retry";
            d->m_iat = new IatWidget(this);
            d->m_iat->startIat();
        });
        return;
    }

    qCDebug(logAudioWizard) << "Creating new IAT widget";
    d->m_iat = new IatWidget(this);
    d->m_iat->startIat();
}

//转写
QString AudioAiassistant::startAsr(const QVariantMap &param)
{
    qCDebug(logAudioWizard) << "Starting ASR with parameters:" << param.keys();
    if (!d->m_asr) {
        qCDebug(logAudioWizard) << "Creating new ASR wrapper";
        d->m_asr = new AsrWrapper();
        connect(d->m_asr,&AsrWrapper::onNotify,this,&AudioAiassistant::onNotify,Qt::DirectConnection);
    }
    return d->m_asr->startAsr(param);
}

void AudioAiassistant::stopAsr()
{
    qCDebug(logAudioWizard) << "Stopping ASR";
    if (d->m_asr) {
        d->m_asr->stopAsr();
    }
}
