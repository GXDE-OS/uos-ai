#include "audiocontroler.h"
#include "audiorecorder.h"
#include "iatsocketserver.h"
#include "audioplayerstream.h"
#include "ttssocketserver.h"
#include "networkdefs.h"
#include "servercodetranslation.h"
#include "audiodbusinterface.h"
#include "networkmonitor.h"
#include "iatlocalserver.h"
#include "ttslocalserver.h"
#include "dbwrapper.h"

#include <QDebug>
#include <QSoundEffect>
#include <QLoggingCategory>

#ifdef AUDIOPATH
static const QString &audioPath = AUDIOPATH;
#else
static const QString &audioPath = "/usr/lib/uos-ai-assistant/audio";
#endif

Q_DECLARE_LOGGING_CATEGORY(logAudio)

AudioControler::AudioControler(QObject *parent)
    : QThread(parent)
{
    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultOutputChanged, this, [this]() {
        qCDebug(logAudio) << "Audio output device changed";
        emit playDeviceChanged(audioOutputDeviceValid());
    });

    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultInputChanged, this, [this]() {
        qCDebug(logAudio) << "Audio input device changed";
        emit recordDeviceChange(audioInputDeviceValid());
    });

    m_audioModel = DbWrapper::localDbWrapper().getLocalSpeech() ? Local : NetWork;
    qCDebug(logAudio) << "Initial audio model:" << m_audioModel;

    moveToThread(this);
    start();

    QTimer::singleShot(1, this, SLOT(prepare()));
}

AudioControler::~AudioControler()
{
    quit();
    wait();
}

AudioControler *AudioControler::instance()
{
    static AudioControler controler;
    return &controler;
}

bool AudioControler::audioInputDeviceValid()
{
    return AudioDbusInterface::instance()->isDefaultInputDeviceValid();
}

bool AudioControler::audioOutputDeviceValid()
{
    return AudioDbusInterface::instance()->isDefaultOutputDeviceValid();
}

bool AudioControler::switchModel(AudioModel model)
{
    qCDebug(logAudio) << "Switching audio model from" << m_audioModel << "to" << model;
    m_audioModel = model;
    return true;
}

void AudioControler::prepare()
{
    m_audioRecorder.reset(new AudioRecorder);
    connect(m_audioRecorder.data(), &AudioRecorder::recordStarted,  this, &AudioControler::onRecordStarted, Qt::QueuedConnection);
    connect(m_audioRecorder.data(), &AudioRecorder::recordStoped,   this, &AudioControler::onRecordStoped, Qt::QueuedConnection);
    connect(m_audioRecorder.data(), &AudioRecorder::audioRecorded,  this, &AudioControler::onAudioRecorded, Qt::QueuedConnection);
    connect(m_audioRecorder.data(), &AudioRecorder::levelUpdated,   this, &AudioControler::levelUpdated, Qt::QueuedConnection);

    connect(m_audioRecorder.data(), &AudioRecorder::recordError,    this, [this]() {
        emit recordError(AIServer::AudioInputDeviceInvalid
                         , ServerCodeTranslation::serverCodeTranslation(AIServer::AudioInputDeviceInvalid
                                                                        , "invalid input device"));
    }, Qt::QueuedConnection);

    m_audioPlayer.reset(new AudioPlayer);
    connect(m_audioPlayer.data(), &AudioPlayer::appendPlayText,      this, &AudioControler::onAppendPlayText, Qt::QueuedConnection);
    connect(m_audioPlayer.data(), &AudioPlayer::playerStreamStarted, this, &AudioControler::onPlayerStreamStarted, Qt::QueuedConnection);
    connect(m_audioPlayer.data(), &AudioPlayer::playerStreamStopped, this, &AudioControler::playTextFinished, Qt::QueuedConnection);
    connect(m_audioPlayer.data(), &AudioPlayer::playerFileStop,      this, &AudioControler::playTextFinished, Qt::QueuedConnection);

    connect(m_audioPlayer.data(), &AudioPlayer::playError,           this, [this]() {
        emit playerError(AIServer::AudioOutputDeviceInvalid
                         , ServerCodeTranslation::serverCodeTranslation(AIServer::AudioOutputDeviceInvalid
                                                                        , "invalid output device"));
    }, Qt::QueuedConnection);
}

void AudioControler::resetIatServer()
{
    if (!m_iatServer.isNull() && m_iatServer->model() == m_audioModel)
        return;

    switch (m_audioModel) {
    case NetWork:
        m_iatServer.reset(new IatSocketServer(AccountProxy::xfInlineAccount()));
        break;
    case Local:
        m_iatServer.reset(new IatLocalServer());
        break;
    default:
        break;
    }

    if (!m_iatServer.isNull()) {
        m_iatServer->setModel(m_audioModel);
        connect(m_iatServer.data(), &IatServer::error, this, &AudioControler::iatserverError, Qt::QueuedConnection);
        connect(m_iatServer.data(), &IatServer::textReceived, this, &AudioControler::textReceived, Qt::QueuedConnection);
    }
}

void AudioControler::resetTtsServer(const QString &id)
{
    switch (m_audioModel) {
    case NetWork:
        m_ttsServer.reset(new TtsSocketServer(id, AccountProxy::xfInlineAccount()));
        break;
    case Local:
        m_ttsServer.reset(new TtsLocalServer(id));
        break;
    default:
        break;
    }

    if (!m_ttsServer.isNull()) {
        m_ttsServer->setModel(m_audioModel);
        connect(m_ttsServer.data(), &TtsServer::error, this, &AudioControler::ttsServerError, Qt::QueuedConnection);
        connect(m_ttsServer.data(), &TtsServer::appendAudioData, this, &AudioControler::ttsAudioData, Qt::QueuedConnection);
        connect(m_ttsServer.data(), &TtsServer::audioNullFinished, this, &AudioControler::playTextFinished, Qt::QueuedConnection);
    }
}

QPair<int, QString> AudioControler::formatError(int code, QString errorMessage)
{
    if (!NetworkMonitor::getInstance().isOnline() && Local != m_audioModel) {
        code = AIServer::NetworkError;
        errorMessage = QCoreApplication::translate("AudioControler", "Unable to connect to the server, please check your network or try again later.");
    }

    return qMakePair(code, errorMessage);
}

void AudioControler::clearTtsData()
{
    m_audioData.clear();

    if (m_ttsServer.isNull())
        return;

    m_ttsServer->cancel();
    m_ttsServer.clear();
}

void AudioControler::ttsServerError(int code, const QString &errorString)
{
    const QPair<int, QString> &errorPair = formatError(code, errorString);

    stopPlayTextAudio();
    emit playerError(errorPair.first, errorPair.second);
}

void AudioControler::iatserverError(int code, const QString &errorString)
{
    const QPair<int, QString> &errorPair = formatError(code, errorString);

    stopRecorder();
    emit recordError(errorPair.first, errorPair.second);
}

bool AudioControler::startRecorder()
{
    if (!audioInputDeviceValid()) {
        qCWarning(logAudio) << "Invalid audio input device";
        emit playerError(AIServer::AudioInputDeviceInvalid
                         , ServerCodeTranslation::serverCodeTranslation(AIServer::AudioInputDeviceInvalid
                                                                        , "invalid input device"));
        return false;
    }

    return m_audioRecorder->start();
}

bool AudioControler::stopRecorder()
{
    return m_audioRecorder->stop();
}

bool AudioControler::stopPlayTextAudio()
{
    m_audioPlayer->stopPlayer();
    QTimer::singleShot(0, this, SLOT(clearTtsData()));
    return true;
}

void AudioControler::playSystemSound(AudioSystemEffect effect)
{
    QString path;

    switch (effect) {
    case Active:
        path = audioPath + "/active.wav";
        break;
    case Sleep:
        path = audioPath + "/sleep.wav";
        break;
    default:
        break;
    }

    if (path.isEmpty())
        return;

    m_sound.reset(new QSoundEffect);
    m_sound->setSource(QUrl::fromLocalFile(path));
    m_sound->play();
}

bool AudioControler::startAppendPlayText(const QString &id, const QString &text, bool isEnd)
{
    qCDebug(logAudio) << "Starting to play text, ID:" << id << "Length:" << text.length() << "isEnd:" << isEnd;
    
    if (!audioOutputDeviceValid()) {
        emit playerError(AIServer::AudioOutputDeviceInvalid
                         , ServerCodeTranslation::serverCodeTranslation(AIServer::AudioOutputDeviceInvalid
                                                                        , "invalid output device"));
        return false;
    }

    //特殊字符过滤 * # \ _
    QString filterResult = filterMarkdown(text);
    qCDebug(logAudio) << "Filtered text length:" << filterResult.length();

    if (m_tempDir.isValid()) {
        QString tempFile = m_tempDir.path() + "/" + id;
        if (QFile::exists(tempFile)) {
            qCDebug(logAudio) << "Playing from existing audio file:" << tempFile;
            return m_audioPlayer->playFileSync(id, tempFile);
        } else {
            if (m_audioPlayer->id() == id && m_audioPlayer->isStreamPlaying()) {
                qCDebug(logAudio) << "Appending to existing stream";
                emit m_audioPlayer->appendPlayText(id, filterResult, false, isEnd);
            } else {
                qCDebug(logAudio) << "Appending to existing stream";
                m_audioData.clear();
                bool ret = m_audioPlayer->startStream(id);
                if (ret) emit m_audioPlayer->appendPlayText(id, filterResult, true, isEnd);
                return ret;
            }
        }
    } else {
        qWarning() << "create tempdir error = " << m_tempDir.errorString();
        return false;
    }

    return true;
}

QString AudioControler::filterMarkdown(const QString &markdownText)
{
    QRegularExpression filterRegex("[*#\\\\_]");

    QString filteredText = markdownText;
    filteredText.replace(filterRegex, "");

    filteredText = filteredText.simplified();

    return filteredText;
}

void AudioControler::onAudioRecorded(QByteArray data)
{
    if (!m_audioRecorder->isRecording())
        return;

    if (m_iatServer.isNull())
        return;

    QTimer::singleShot(5, this, [ = ]() {
        m_iatServer->sendData(data);
    });
}

void AudioControler::onRecordStoped()
{
    if (m_iatServer.isNull())
        return;

    m_iatServer->cancel();
}

void AudioControler::onRecordStarted()
{
    resetIatServer();
    m_iatServer->openServer();
}

void AudioControler::ttsAudioData(const QString &id, const QByteArray &data, bool isLast)
{
    if (m_audioPlayer->id() != id) {
        qCWarning(logAudio) << "ID mismatch, expected:" << m_audioPlayer->id() << "got:" << id;
        return;
    }

    if (!data.isEmpty()) {
        m_audioData += data;
    }

    if (isLast) {
        QString tempFile = m_tempDir.path() + "/" + m_audioPlayer->id();
        QFile file(tempFile);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(logAudio) << "Failed to write audio file:" << file.errorString() << tempFile;
        } else {
            file.write(m_audioData);
            file.close();
            qCDebug(logAudio) << "Saved audio data to:" << tempFile;
        }

        clearTtsData();
    }

    m_audioPlayer->appendStreamAudio(id, data, isLast);
}

void AudioControler::onPlayerStreamStarted(const QString &id)
{
    resetTtsServer(id);
    m_ttsServer->openServer();
}

void AudioControler::onAppendPlayText(const QString &id, const QString &text, bool isStart, bool isEnd)
{
    if (m_ttsServer.isNull()) {
        resetTtsServer(id);
    }

    if (m_ttsServer->id() != id) {
        qWarning() << "id error " << m_audioPlayer->id() << id;
        return;
    }

    m_ttsServer->sendText(text, isStart, isEnd);
}
