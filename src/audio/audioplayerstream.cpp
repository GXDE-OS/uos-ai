#include "audioplayerstream.h"
#include "audiodbusinterface.h"

#include <QDebug>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QList>
#include <QEventLoop>
#include <QFile>
#include <unistd.h>
#include <QMediaPlayer>
#include <qtimer.h>

bool PlayerThread::isPlaying() const
{
    if (m_stop)
        return false;

    return this->isRunning();
}

void PlayerThread::start(QIODevice *stream)
{
    m_isLast = false;
    m_stop = false;
    m_stream = stream;
    m_dataList.clear();
    QThread::start();

    emit m_player->playerStreamStarted(m_player->m_id);
}

void PlayerThread::appendAudio(const QString &id, QByteArray data, bool isLast)
{
    if (m_stop) {
        qWarning() << "last piece of audio has been appended";
        return;
    }

    if (this->isFinished()) {
        QThread::start();
        qWarning() << "playstream try start.";
    }

    QMutexLocker locker(&m_mutext);
    m_id = id;
    m_isLast = isLast;
    m_dataList.push_back(data);
}

void PlayerThread::run()
{
    while (!m_stop) {
        if (m_dataList.isEmpty()) {
            msleep(100);
            continue;
        }

        m_mutext.lock();
        QString id = m_id;
        bool isLast = m_isLast;
        QByteArrayList curDataList = m_dataList;
        m_dataList.clear();
        m_mutext.unlock();

        for (int i = 0; i < curDataList.size(); ++i) {
            auto &curByteArray = curDataList[i];
            int written = 0;
            while (written < curByteArray.size()) {
                if (m_stop || id != m_player->m_id) {
                    break;
                }

                int leftSize = curByteArray.size() - written;
                qint64 sizeWritten = m_stream->write(curByteArray.data() + written, leftSize);
                if (sizeWritten > 0) {
                    written += sizeWritten;
                }
                msleep(10);
            }
        }

        if (isLast) {
            // 增加超时退出，部分设备下在播放结束后state不会变化，会造成工作异常，这里超时设置为
            // 500毫秒，足够最后缓存的数据播放完毕
            int timeWait = 0;

            while (!m_stop && id == m_player->m_id && timeWait < 500 && !m_player->m_audioOutput.isNull() && m_player->m_audioOutput->state() == QAudio::ActiveState) {
                msleep(10);
                timeWait += 10;
            }

            if (id != m_player->m_id) {
                continue;
            }

            emit m_player->playerStreamStopped(m_player->m_id);
            break;
        }
    }
}

AudioPlayer::AudioPlayer()
{
    m_audioFormat.setSampleRate(16000);
    m_audioFormat.setChannelCount(1);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);

    connect(this, &AudioPlayer::playerFileStop, this, &AudioPlayer::onPlayerFileStop);
    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultOutputChanged, this, &AudioPlayer::checkDevice);
}

AudioPlayer::~AudioPlayer()
{
}

bool AudioPlayer::startStream(const QString &id)
{
    stopPlayer();

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (!deviceInfo.isFormatSupported(m_audioFormat)) {
        deviceInfo = findSupportedDevice();
    }
    if (deviceInfo.isNull() || (!deviceInfo.isFormatSupported(m_audioFormat))) {
        qCritical() << "invalid audio output device";
        return false;
    }

    m_audioOutput.reset(new QAudioOutput(deviceInfo, m_audioFormat));
    m_audioOutput->setBufferSize(10240);
    if (!m_audioOutput) {
        qCritical() << "AudioPlayer audio output is nullptr";
        return false;
    }

    QIODevice *stream = m_audioOutput->start();
    if (!stream) {
        qCritical() << "AudioPlayer m_audioOutput start failed";
        return false;
    }

    if (m_playerThread.isNull()) {
        m_playerThread.reset(new PlayerThread(this));
    }

    m_id = id;
    m_playerThread->start(stream);
    return true;
}

void AudioPlayer::appendStreamAudio(const QString &id, QByteArray data, bool isLast)
{
    if (m_playerThread == nullptr) {
        qWarning() << "player thread is not valid";
        return;
    }

    m_playerThread->appendAudio(id, data, isLast);
}

void AudioPlayer::stopStreamInternal()
{
    m_id.clear();

    if (m_playerThread != nullptr) {
        m_playerThread->stop();
    }

    if (m_audioOutput) {
        m_audioOutput->stop();
    }
    if (QObject *obj = m_audioOutput.take())
        obj->deleteLater();
}

void AudioPlayer::stopPlayer()
{
    onPlayerFileStop();
    stopStreamInternal();
}

QString AudioPlayer::id() const
{
    return m_id;
}

void AudioPlayer::onPlayerFileStop()
{
    for (QAudioOutput *audioOutput : m_audioFileOutputs) {
        audioOutput->stop();
        audioOutput->disconnect();
        audioOutput->deleteLater();
    }
    m_audioFileOutputs.clear();
}

bool AudioPlayer::checkDevice()
{
    if (AudioDbusInterface::instance()->isDefaultOutputDeviceValid()
            && QAudioDeviceInfo::defaultOutputDevice().isFormatSupported(m_audioFormat))
        return true;

    if (isStreamPlaying() || !m_audioFileOutputs.isEmpty())
        emit playError();

    stopPlayer();
    return false;
}

QAudioDeviceInfo AudioPlayer::findSupportedDevice()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (deviceInfo.isFormatSupported(audioFormat))
        return deviceInfo;

    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (int i = 0; i < devices.size(); ++i) {
        auto &dev = devices[i];
        if (dev.isFormatSupported(audioFormat))
            return dev;
    }

    return QAudioDeviceInfo();
}

bool AudioPlayer::isStreamPlaying() const
{
    if (m_playerThread.isNull())
        return false;

    return m_playerThread->isPlaying();
}

bool AudioPlayer::playFileSync(const QString &id, QString filePath)
{
    QAudioDeviceInfo deviceInfo = findSupportedDevice();
    if (deviceInfo.isNull()) {
        qCritical() << "invalid audio output device";
        return false;
    }

    onPlayerFileStop();

    QAudioOutput *audioOutput = new QAudioOutput(deviceInfo, m_audioFormat);
    connect(audioOutput, &QAudioOutput::stateChanged, [ = ](QAudio::State state) {
        if (state == QAudio::IdleState) {
            emit playerFileStop(id);
        } else if (state == QAudio::ActiveState) {
            stopStreamInternal();
        }
    });

    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();

    AudioPlayDevice *audioDevice = new AudioPlayDevice(data, audioOutput);
    if (!audioDevice->open(QIODevice::ReadOnly)) {
        qCritical() << "audio device open failed";
        delete audioOutput;
        return false;
    }

    audioOutput->start(audioDevice);
    m_audioFileOutputs << audioOutput;

    // 有时会收不到QAudio::IdleState信号，这里根据音频时长信息设置个超时来保障
    int audioDurationMs = data.size() * 1000 / ((m_audioFormat.sampleRate() * m_audioFormat.channelCount() * m_audioFormat.sampleSize()) / 8) + 500;

    QTimer *timer = new QTimer(audioOutput);
    connect(timer, &QTimer::timeout, this, [ = ]() {
        emit playerFileStop(id);
    });
    timer->setSingleShot(true);
    timer->start(audioDurationMs);

    return true;
}
