#include "audiorecorder.h"
#include "audiodbusinterface.h"

#include <QFile>
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker>

AudioInfo::AudioInfo()
{
}

void AudioInfo::start()
{
    open(QIODevice::WriteOnly);
}

void AudioInfo::stop()
{
    close();
}

qint64 AudioInfo::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    return 0;
}

qint64 AudioInfo::writeData(const char *data, qint64 len)
{
    QByteArray audioData(data, len);
    emit audioWrite(audioData);
    return len;
}

AudioLocalInputDevice::AudioLocalInputDevice()
{
    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultInputChanged, this, &AudioLocalInputDevice::checkDevice);

    m_audioFormat.setSampleRate(16000);
    m_audioFormat.setChannelCount(1);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    m_audioInfo.reset(new AudioInfo());
    connect(m_audioInfo.get(), &AudioInfo::audioWrite, this, &AudioLocalInputDevice::audioRecorded);
}

AudioLocalInputDevice::~AudioLocalInputDevice()
{

}

bool AudioLocalInputDevice::start()
{
    if (m_isRecording) {
        qWarning() << "recorder already started";
        return true;
    }

    if (!checkDevice()) {
        qWarning() << "audio recorder device may not be valid";
        return false;
    }

    m_audioInput.reset(new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), m_audioFormat));
    m_audioInfo->start();
    m_audioInput->start(m_audioInfo.data());
    m_isRecording = true;
    emit recordStarted();
    return true;
}

bool AudioLocalInputDevice::stop()
{
    if (!m_isRecording) {
        return true;
    }

    if (QObject *obj = m_audioInput.take()) {
        obj->deleteLater();
    }
    m_audioInfo->stop();
    m_isRecording = false;
    emit recordStoped();
    return true;
}

bool AudioLocalInputDevice::checkDevice()
{
    if (AudioDbusInterface::instance()->isDefaultInputDeviceValid()
            && QAudioDeviceInfo::defaultInputDevice().isFormatSupported(m_audioFormat))
        return true;

    if (m_isRecording) {
        stop();
        emit recordError();
    }

    return false;
}

bool AudioLocalInputDevice::isRecording()
{
    return m_isRecording;
}

AudioRecorder::AudioRecorder()
{
    m_timer.setSingleShot(true);

    m_defaultDevice = new AudioLocalInputDevice();
    connect(m_defaultDevice, &AudioLocalInputDevice::audioRecorded, this, &AudioRecorder::onAudioData, Qt::QueuedConnection);
    connect(m_defaultDevice, &AudioLocalInputDevice::recordStarted, this, [this]() {
        m_animation->setStartValue(0);
        emit recordStarted();
    }, Qt::QueuedConnection);
    connect(m_defaultDevice, &AudioLocalInputDevice::recordStoped,  this, [this]() {
        m_animation->setEndValue(0);
        m_animation->start();
        emit recordStoped();
    }, Qt::QueuedConnection);
    connect(m_defaultDevice, &AudioLocalInputDevice::recordError,   this, [this]() {
        m_animation->setEndValue(0);
        m_animation->start();
        emit recordError();
    }, Qt::QueuedConnection);

    m_maxAmplitude = calcAudioMaxAmplitude(m_defaultDevice->m_audioFormat);

    m_animation = new QPropertyAnimation(this, "level");
    m_animation->setDuration(100);
    m_animation->setStartValue(0);
    m_animation->setEasingCurve(QEasingCurve::Linear);
}

AudioRecorder::~AudioRecorder()
{
    m_defaultDevice->deleteLater();
}

qint16 AudioRecorder::calcAudioMaxAmplitude(const QAudioFormat &format)
{
    qint16 maxValue;
    if (format.sampleType() == QAudioFormat::SignedInt) {
        maxValue = qPow(2, format.sampleSize() - 1) - 1;
    } else {
        maxValue = qPow(2, format.sampleSize()) - 1;
    }
    return maxValue;
}

void AudioRecorder::setlevel(int level)
{
    m_curLevel = level;
    emit levelUpdated(level);
}

int AudioRecorder::level() const
{
    return m_curLevel;
}

QAudioDeviceInfo AudioRecorder::findAudioInputDevice()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();
    if (deviceInfo.isFormatSupported(audioFormat)) {
        return deviceInfo;
    }

    const auto allDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const auto &dev : allDevices) {
        if (dev.isFormatSupported(audioFormat)) {
            return dev;
        }
    }

    return QAudioDeviceInfo();
}

bool AudioRecorder::start()
{
    m_lastLevel = 0;
    m_curLevel = 0;
    return m_defaultDevice->start();
}

bool AudioRecorder::stop()
{
    m_lastLevel = 0;
    m_curLevel = 0;
    return m_defaultDevice->stop();
}

bool AudioRecorder::isRecording()
{
    return m_defaultDevice->isRecording();
}

int AudioRecorder::updateLevel(const QByteArray &pcmData)
{
    if (m_maxAmplitude && !pcmData.isEmpty()) {
        qint16 maxValue = 0;

        int dataSize = pcmData.size() / sizeof(qint16);
        const qint16 *data = reinterpret_cast<const qint16 *>(pcmData.constData());

        for (int i = 0; i < dataSize; ++i) {
            maxValue = qMax(data[i], maxValue);
        }

        maxValue = qMin(maxValue, m_maxAmplitude);
        return (maxValue * 1.0 / m_maxAmplitude) * 100;
    }

    return 0;
}

void AudioRecorder::onAudioData(QByteArray data)
{
    emit audioRecorded(data);

    m_lastLevel = qMax(updateLevel(data), m_lastLevel);
    if (!m_timer.isActive()) {
        m_timer.start(50);
        m_animation->setStartValue(m_curLevel);
        m_animation->setEndValue(m_lastLevel);
        m_animation->stop();
        m_animation->start();
        m_lastLevel = 0;
    }
}

