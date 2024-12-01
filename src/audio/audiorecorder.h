#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioInput>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <QPropertyAnimation>
#include <QFile>

//#define SAVE_AUDIO_DATA

class AudioInfo : public QIODevice
{
    Q_OBJECT

public:
    AudioInfo();
    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

#ifdef SAVE_AUDIO_DATA
    // 保存录制的音频数据，测试用
    // 写文件头
    void initFileFormat(const QString &fileName, quint16 numChannels, quint16 sampleRate);
    // 写数据
    bool appendAudioDataToFile(const QString &filePath, const QByteArray &audioData);
#endif

signals:
    void audioWrite(const QByteArray &data);

private:
    friend class AudioRecorder;
};

class AudioLocalInputDevice : public QObject
{
    Q_OBJECT

public:
    explicit AudioLocalInputDevice();
    virtual ~AudioLocalInputDevice() override;

    bool start();
    bool stop();
    bool isRecording();

signals:
    void audioRecorded(QByteArray data);
    void recordStarted();
    void recordStoped();
    void recordError();

private:
    bool checkDevice();

private:
    QScopedPointer<AudioInfo>   m_audioInfo;
    QAudioFormat                m_audioFormat;
    QScopedPointer<QAudioInput> m_audioInput;

private:
    friend class AudioRecorder;
    bool m_isRecording = false;
};

class AudioRecorder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int level READ level WRITE setlevel)

public:
    explicit AudioRecorder();
    virtual ~AudioRecorder() override;

    static QAudioDeviceInfo findAudioInputDevice();

    bool start();
    bool stop();
    bool isRecording();

signals:
    void audioRecorded(QByteArray data);
    void recordStarted();
    void recordStoped();
    void recordError();
    void levelUpdated(int level);

private slots:
    void onAudioData(QByteArray data);

private:
    int updateLevel(const QByteArray &pcmData);
    qint16 calcAudioMaxAmplitude(const QAudioFormat &format);

    void setlevel(int level);
    int level() const;

private:
    QTimer m_timer;
    qint16 m_maxAmplitude = 0;
    int m_lastLevel = 0;
    int m_curLevel = 0;
    AudioLocalInputDevice *m_defaultDevice = nullptr;

    QPropertyAnimation *m_animation;
};

#endif // AUDIORECORDER_H
