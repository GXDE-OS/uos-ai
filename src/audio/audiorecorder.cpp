#include "audiorecorder.h"
#include "audiodbusinterface.h"

#include <QFile>
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudio)

AudioInfo::AudioInfo()
{
}

void AudioInfo::start()
{
#ifdef SAVE_AUDIO_DATA
    qCDebug(logAudio) << "Initializing audio file format";
    initFileFormat("./audio", 1, 16000);
#endif
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

#ifdef SAVE_AUDIO_DATA
    appendAudioDataToFile("./audio", audioData);
#endif

    return len;
}

#ifdef SAVE_AUDIO_DATA
void AudioInfo::initFileFormat(const QString &fileName, quint16 numChannels, quint16 sampleRate)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "open file failed.";
    }

    // 写入RIFF头
    file.write("RIFF", 4);
    quint32 fileSize = 36;
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);

    // 写入格式块的大小（通常为16）
    quint32 formatSize = 16;
    file.write(reinterpret_cast<const char*>(&formatSize), 4);

    // 写入格式标识符（通常为1）1表示pcm
    quint16 formatTag = 1;
    file.write(reinterpret_cast<const char*>(&formatTag), 2);

    // 写入通道数和采样率
    quint16 numChannelsToWrite = numChannels;
    file.write(reinterpret_cast<const char*>(&numChannelsToWrite), 2);
    quint32 sampleRateToWrite = sampleRate;
    file.write(reinterpret_cast<const char*>(&sampleRateToWrite), 4);

    // 写入比特率
    quint32 byteRate = sampleRate * numChannels * 2; // 2字节/采样
    file.write(reinterpret_cast<const char*>(&byteRate), 4);

    // 写入块对齐
    quint16 blockAlign = numChannels * 2;
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);

    // 位深度（通常为16）
    quint16 bitsPerSample = 16;
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // "data" 块标识符
    file.write("data", 4);

    file.close();
}
bool AudioInfo::appendAudioDataToFile(const QString &filePath, const QByteArray &audioData)
{
    QFile file(filePath);
    if (!file.open(QIODevice::Append)) {
        return false;
    }
    // 获取文件大小，并计算需要跳过的字节数
    qint64 bytesToSkip = file.size();
    if (bytesToSkip > 0) {
        file.seek(bytesToSkip);
    }

    qint64 bytesWritten = file.write(audioData);
    // 检查是否写入完成
    if (bytesWritten != audioData.size()) {
        return false;
    }
    return true;
}
#endif

AudioLocalInputDevice::AudioLocalInputDevice()
{
    connect(AudioDbusInterface::instance(), &AudioDbusInterface::defaultInputChanged, this, &AudioLocalInputDevice::checkDevice);

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
    m_audioInfo.reset(new AudioInfo());
    connect(m_audioInfo.get(), &AudioInfo::audioWrite, this, &AudioLocalInputDevice::audioRecorded);
}

AudioLocalInputDevice::~AudioLocalInputDevice()
{

}

bool AudioLocalInputDevice::start()
{
    if (m_isRecording) {
        qCWarning(logAudio) << "Recording already in progress";
        return true;
    }

    if (!checkDevice()) {
        qCWarning(logAudio) << "Invalid audio input device";
        return false;
    }
#ifdef COMPILE_ON_QT6
    qCDebug(logAudio) << "Creating QAudioSource with default input device";
    m_audioInput.reset(new QAudioSource(QMediaDevices::defaultAudioInput(), m_audioFormat));
#else
    qCDebug(logAudio) << "Creating QAudioInput with default input device";
    m_audioInput.reset(new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), m_audioFormat));
#endif

    m_audioInfo->start();
    m_audioInput->start(m_audioInfo.data());
    m_isRecording = true;
    qCDebug(logAudio) << "Audio recording started successfully";
    emit recordStarted();
    return true;
}

bool AudioLocalInputDevice::stop()
{
    if (!m_isRecording) {
        qCDebug(logAudio) << "No active recording to stop";
        return true;
    }

    qCDebug(logAudio) << "Stopping audio recording";
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
#ifdef COMPILE_ON_QT6
    if (AudioDbusInterface::instance()->isDefaultInputDeviceValid()
            && QMediaDevices::defaultAudioInput().isFormatSupported(m_audioFormat))
        return true;
#else
    if (AudioDbusInterface::instance()->isDefaultInputDeviceValid()
            && QAudioDeviceInfo::defaultInputDevice().isFormatSupported(m_audioFormat))
        return true;
#endif

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
#ifdef COMPILE_ON_QT6
    if (format.sampleFormat() == QAudioFormat::SampleFormat::Int16) {
        maxValue = 32767;  // 2^15 - 1，对应16位有符号整数的最大值
    } else if (format.sampleFormat() == QAudioFormat::SampleFormat::UInt8) {
        maxValue = 255;    // 2^8 - 1，对应8位无符号整数的最大值
    } else {
        maxValue = 0;      // 不支持的格式
    }
#else
    if (format.sampleType() == QAudioFormat::SignedInt) {
        maxValue = qPow(2, format.sampleSize() - 1) - 1;
    } else {
        maxValue = qPow(2, format.sampleSize()) - 1;
    }
#endif
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

#ifdef COMPILE_ON_QT6
QAudioDevice AudioRecorder::findAudioInputDevice()
{
    // 设置所需的音频格式
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleFormat(QAudioFormat::Int16); // 替代 setSampleSize 和 setSampleType

    // 获取默认音频输入设备
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (defaultDevice.isFormatSupported(audioFormat)) {
        return defaultDevice;
    }

    // 遍历所有可用的音频输入设备，寻找支持所需格式的设备
    const auto allDevices = QMediaDevices::audioInputs();
    for (const auto &device : allDevices) {
        if (device.isFormatSupported(audioFormat)) {
            return device;
        }
    }

    // 如果没有找到合适的设备，返回默认设备，或可以返回一个无效的 QAudioDevice
    return QAudioDevice();
}
#else
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
#endif

bool AudioRecorder::start()
{
    qCDebug(logAudio) << "Starting audio recorder";
    m_lastLevel = 0;
    m_curLevel = 0;
    return m_defaultDevice->start();
}

bool AudioRecorder::stop()
{
    qCDebug(logAudio) << "Stopping audio recorder";
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

