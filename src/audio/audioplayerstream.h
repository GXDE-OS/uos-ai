#ifndef AUDIOPLAYERSTREAM_H
#define AUDIOPLAYERSTREAM_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QBuffer>
#include <QWaitCondition>
#include <QAudioOutput>
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include <QDebug>

class AudioPlayer;
class AudioPlayDevice : public QIODevice
{
public:
    AudioPlayDevice(const QByteArray &data = QByteArray(),
                    QObject *parent = nullptr)
        : QIODevice(parent)
        , m_data(data)
    {
    }

    ~AudioPlayDevice() override
    {

    }

    void setData(QByteArray data)
    {
        m_data = data;
        m_written = 0;
    }

    void close() override
    {
        m_written = 0;
        QIODevice::close();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (m_written >= m_data.size())
            return 0;
        qint64 len = (m_written + maxSize) > m_data.size() ? (m_data.size() - m_written) : maxSize;
        memcpy(data, m_data.data() + m_written, len);
        m_written += len;
        return len;
    }

    qint64 writeData(const char *data, qint64 maxSize) override
    {
        Q_UNUSED(data);
        Q_UNUSED(maxSize);
        return -1;
    }

private:
    QByteArray m_data;
    qint64     m_written{ 0 };
};

class PlayerThread : public QThread
{
    Q_OBJECT
public:
    PlayerThread(AudioPlayer *player)
        : m_player(player)
    {

    }
    ~PlayerThread()
    {

    }

public:
    bool isPlaying() const;

    void start(QIODevice *stream);

    void appendAudio(const QString &id, QByteArray data, bool isLast);

    void stop()
    {
        m_id.clear();
        m_isLast = false;
        m_stop = true;
        m_dataList.clear();
    }

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

    static QAudioDeviceInfo findSupportedDevice();

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
    QScopedPointer<QAudioOutput>     m_audioOutput;
    QScopedPointer<PlayerThread>     m_playerThread;

    QString m_id;
    QList<QAudioOutput *> m_audioFileOutputs;
};

#endif // AUDIOPLAYERSTREAM_H
