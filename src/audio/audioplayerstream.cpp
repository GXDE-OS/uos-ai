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
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudio)

PlayerThread::PlayerThread(AudioPlayer *player): QThread ()
    , m_player(player)
{

}

PlayerThread::~PlayerThread()
{

}

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
        qCWarning(logAudio) << "Attempted to append audio to stopped thread";
        return;
    }

    if (this->isFinished()) {
        qCDebug(logAudio) << "Restarting finished player thread";
        QThread::start();
    }

    QMutexLocker locker(&m_mutext);
    m_id = id;
    m_isLast = isLast;
    m_dataList.push_back(data);
}

void PlayerThread::stop()
{
    m_id.clear();
    m_isLast = false;
    m_stop = true;
    m_dataList.clear();
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
#ifdef COMPILE_ON_QT6
                //根据缓冲区剩余空间传入数据
                int chunkSize = qMin(leftSize, m_player->m_audioOutput->bytesFree());
                if (chunkSize < 0) {
                    chunkSize = 0;
                }
                qint64 sizeWritten = m_stream->write(curByteArray.data() + written, chunkSize);
                if (sizeWritten > 0) {
                    written += sizeWritten;
                } else {
                    QThread::msleep(1);
                }
#else
                leftSize = leftSize < 10240 ? leftSize : 10240;
                qint64 sizeWritten = m_stream->write(curByteArray.data() + written, leftSize);
                if (sizeWritten > 0) {
                    written += sizeWritten;
                }
                msleep(100);
#endif
            }
        }

        if (isLast) {
#ifndef COMPILE_ON_QT6
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
#endif
            break;
        }
    }
}

AudioPlayer::AudioPlayer()
{
    m_audioFormat.setSampleRate(16000);
    m_audioFormat.setChannelCount(1);
#ifdef COMPILE_ON_QT6
    m_audioFormat.setSampleFormat(QAudioFormat::SampleFormat::Int16);
#else
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
#endif

    connect(this, &AudioPlayer::playerFileStop, this, &AudioPlayer::onPlayerFileStop);
    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultOutputChanged, this, &AudioPlayer::checkDevice);
}

AudioPlayer::~AudioPlayer()
{
}

bool AudioPlayer::startStream(const QString &id)
{
    qCDebug(logAudio) << "Starting audio stream for ID:" << id;
    stopPlayer();
#ifdef COMPILE_ON_QT6
    QAudioDevice deviceInfo = QMediaDevices::defaultAudioOutput();
#else
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
#endif
    if (!deviceInfo.isFormatSupported(m_audioFormat)) {
        qCDebug(logAudio) << "Default device format not supported, finding alternative";
        deviceInfo = findSupportedDevice();
    }
    if (deviceInfo.isNull() || (!deviceInfo.isFormatSupported(m_audioFormat))) {
        qCritical() << "invalid audio output device";
        return false;
    }
#ifdef COMPILE_ON_QT6
    m_audioOutput.reset(new QAudioSink(deviceInfo, m_audioFormat));
#else
    m_audioOutput.reset(new QAudioOutput(deviceInfo, m_audioFormat));
#endif
    m_audioOutput->setBufferSize(10240);
    if (!m_audioOutput) {
        qCCritical(logAudio) << "Failed to create audio output";
        return false;
    }

    QIODevice *stream = m_audioOutput->start();
    if (!stream) {
        qCCritical(logAudio) << "Failed to start audio output stream";
        return false;
    }

    if (m_playerThread.isNull()) {
        qCDebug(logAudio) << "Creating new player thread";
        m_playerThread.reset(new PlayerThread(this));
    }

    m_id = id;
    m_playerThread->start(stream);
#ifdef COMPILE_ON_QT6
    connect(m_audioOutput.data(), &QAudioSink::stateChanged, this, [ = ](QAudio::State state) {
        qCDebug(logAudio) << "Audio output state changed:" << state;
        if (state == QAudio::IdleState) {
            emit playerStreamStopped(m_id);
        }
    });
#endif
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
#ifdef COMPILE_ON_QT6
        m_audioOutput->reset();
#else
        m_audioOutput->stop();
#endif
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
#ifdef COMPILE_ON_QT6
    for (QAudioSink *audioOutput : m_audioFileOutputs) {
        audioOutput->stop();
        audioOutput->disconnect();
        audioOutput->deleteLater();
    }
#else
    for (QAudioOutput *audioOutput : m_audioFileOutputs) {
        audioOutput->stop();
        audioOutput->disconnect();
        audioOutput->deleteLater();
    }
#endif

    m_audioFileOutputs.clear();
}

bool AudioPlayer::checkDevice()
{

    if (AudioDbusInterface::instance()->isDefaultOutputDeviceValid()
#ifdef COMPILE_ON_QT6
            && QMediaDevices::defaultAudioOutput().isFormatSupported(m_audioFormat)) {
#else
            && QAudioDeviceInfo::defaultOutputDevice().isFormatSupported(m_audioFormat)) {
#endif
        return true;
    }

    if (isStreamPlaying() || !m_audioFileOutputs.isEmpty())
        emit playError();

    stopPlayer();
    return false;
}

#ifdef COMPILE_ON_QT6
QAudioDevice AudioPlayer::findSupportedDevice()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleFormat(QAudioFormat::SampleFormat::Int16);

    // 获取默认输出设备
    QAudioDevice deviceInfo = QMediaDevices::defaultAudioOutput();

    // 检查默认设备是否支持指定格式
    if (deviceInfo.isFormatSupported(audioFormat)) {
        return deviceInfo;
    }

    // 遍历所有可用的音频输出设备
    const auto devices = QMediaDevices::audioOutputs();
    for (const auto &dev : devices) {
        if (dev.isFormatSupported(audioFormat)) {
            return dev;
        }
    }

    // 如果没有找到支持的设备，返回无效设备
    return QAudioDevice();
}
#else
QAudioDeviceInfo AudioPlayer::findSupportedDevice()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
#ifdef COMPILE_ON_QT6
    audioFormat.setSampleFormat(QAudioFormat::SampleFormat::Int16);
    QAudioDevice deviceInfo = QMediaDevices::defaultAudioOutput();
#else
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
#endif
    if (deviceInfo.isFormatSupported(audioFormat))
        return deviceInfo;

#ifdef COMPILE_ON_QT6
    const auto devices = QMediaDevices::audioOutputs();
    for (const auto &dev : devices) {
        if (dev.isFormatSupported(audioFormat)) {
            return dev;
        }
    }

    return QAudioDevice();
#else
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (int i = 0; i < devices.size(); ++i) {
        auto &dev = devices[i];
        if (dev.isFormatSupported(audioFormat))
            return dev;
    }
    return QAudioDeviceInfo();
#endif

}
#endif

bool AudioPlayer::isStreamPlaying() const
{
    if (m_playerThread.isNull())
        return false;

    return m_playerThread->isPlaying();
}

bool AudioPlayer::playFileSync(const QString &id, QString filePath)
{
    qCDebug(logAudio) << "Playing audio file synchronously:" << filePath;
#ifdef COMPILE_ON_QT6
    QAudioDevice deviceInfo = findSupportedDevice();
#else
    QAudioDeviceInfo deviceInfo = findSupportedDevice();
#endif
    if (deviceInfo.isNull()) {
        qCCritical(logAudio) << "No valid audio output device found";
        return false;
    }

    onPlayerFileStop();
#ifdef COMPILE_ON_QT6
    QAudioSink *audioOutput = new QAudioSink(deviceInfo, m_audioFormat);
    connect(audioOutput, &QAudioSink::stateChanged, [ = ](QAudio::State state) {
#else
    QAudioOutput *audioOutput = new QAudioOutput(deviceInfo, m_audioFormat);
    connect(audioOutput, &QAudioOutput::stateChanged, [ = ](QAudio::State state) {
#endif
        qCDebug(logAudio) << "File playback state changed:" << state;
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
        qCCritical(logAudio) << "Failed to open audio device";
        delete audioOutput;
        return false;
    }

    audioOutput->start(audioDevice);
    m_audioFileOutputs << audioOutput;

    // 有时会收不到QAudio::IdleState信号，这里根据音频时长信息设置个超时来保障
    int audioDurationMs = data.size() * 1000 / ((m_audioFormat.sampleRate() * m_audioFormat.channelCount()
#ifdef COMPILE_ON_QT6
                                                 * m_audioFormat.bytesPerSample()
#else
                                                 * m_audioFormat.sampleSize()
#endif
                                                 ) / 8) + 500;

    QTimer *timer = new QTimer(audioOutput);
    connect(timer, &QTimer::timeout, this, [ = ]() {
        qCDebug(logAudio) << "File playback timeout reached";
        emit playerFileStop(id);
    });
    timer->setSingleShot(true);
    timer->start(audioDurationMs);

    return true;
}

AudioPlayDevice::AudioPlayDevice(const QByteArray &data, QObject *parent)
    : QIODevice(parent)
    , m_data(data)
{
}

AudioPlayDevice::~AudioPlayDevice()
{

}

void AudioPlayDevice::setData(QByteArray data)
{
    m_data = data;
    m_written = 0;
}

void AudioPlayDevice::close()
{
    m_written = 0;
    QIODevice::close();
}

qint64 AudioPlayDevice::readData(char *data, qint64 maxSize)
{
    if (m_written >= m_data.size())
        return 0;
    qint64 len = (m_written + maxSize) > m_data.size() ? (m_data.size() - m_written) : maxSize;
    memcpy(data, m_data.data() + m_written, len);
    m_written += len;
    return len;
}

qint64 AudioPlayDevice::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}
