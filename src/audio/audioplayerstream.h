#ifndef AUDIOPLAYERSTREAM_H
#define AUDIOPLAYERSTREAM_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QBuffer>
#include <QWaitCondition>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include <QDebug>

#ifdef COMPILE_ON_QT6
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSink>
#endif

class AudioPlayer;
class AudioPlayDevice : public QIODevice
{
public:
    AudioPlayDevice(const QByteArray &data = QByteArray(),
                    QObject *parent = nullptr);
    ~AudioPlayDevice() override;

    void setData(QByteArray data);
    void close() override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    QByteArray m_data;
    qint64     m_written{ 0 };
};

class PlayerThread : public QThread
{
    Q_OBJECT
public:
    PlayerThread(AudioPlayer *player);
    ~PlayerThread() override;

public:
    bool isPlaying() const;

    void start(QIODevice *stream);

    void appendAudio(const QString &id, QByteArray data, bool isLast);

    void stop();

protected:
    virtual void run() override;

private:
    QMutex m_mutext;

    QByteArrayList     m_dataList;
    QIODevice         *m_stream{ nullptr };
    bool               m_stop{ false };
    bool               m_isLast{ false };
    QString            m_id;
    AudioPlayer       *m_player{ nullptr };
};

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    AudioPlayer();
    virtual ~AudioPlayer() override;
#ifdef COMPILE_ON_QT6
    static QAudioDevice findSupportedDevice();
#else
    static QAudioDeviceInfo findSupportedDevice();
#endif
    bool isStreamPlaying() const;
    bool startStream(const QString &id);
    void appendStreamAudio(const QString &id, QByteArray data, bool isLast);

    bool playFileSync(const QString &id, QString filePath);
    void stopPlayer();

    QString id() const;

signals:
    void playerStreamStarted(const QString &id);
    void playerStreamStopped(const QString &id);

    void playerFileStart(const QString &id);
    void playerFileStop(const QString &id);

    void appendPlayText(const QString &id, const QString &text, bool isStart, bool isEnd);
    void playError();

private slots:
    void onPlayerFileStop();
    bool checkDevice();

private:
    void stopStreamInternal();

private:
    friend class PlayerThread;

    QAudioFormat                     m_audioFormat;
    QScopedPointer<PlayerThread>     m_playerThread;

    QString m_id;
#ifdef COMPILE_ON_QT6
    QScopedPointer<QAudioSink>     m_audioOutput;
    QList<QAudioSink *> m_audioFileOutputs;
#else
    QScopedPointer<QAudioOutput>     m_audioOutput;
    QList<QAudioOutput *> m_audioFileOutputs;
#endif
};

#endif // AUDIOPLAYERSTREAM_H
