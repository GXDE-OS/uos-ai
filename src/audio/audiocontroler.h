#ifndef AUDIOCONTROLER_H
#define AUDIOCONTROLER_H

#include <QThread>
#include <QSharedPointer>
#include <QTemporaryDir>

class IatServer;
class TtsServer;

class AudioPlayer;
class AudioRecorder;
class AudioControler : public QThread
{
    Q_OBJECT

public:
    enum AudioSystemEffect {
        Active = 0,
        Sleep  = 1
    };

    enum AudioModel {
        NetWork = 0,
        Local   = 1
    };

    explicit AudioControler(QObject *parent = nullptr);
    ~AudioControler();

    static AudioControler *instance();

    /**
     * @brief audioInputDeviceValid
     * @return
     */
    static bool audioInputDeviceValid();

    /**
     * @brief audioOutputDeviceValid
     * @return
     */
    static bool audioOutputDeviceValid();

public:
    /**
     * @brief switchModel
     * @param model
     * @return
     */
    bool switchModel(AudioModel model);

    /**
     * @brief startRecorder
     */
    bool startRecorder();

    /**
     * @brief stopRecorder
     */
    bool stopRecorder();

    /**
     * @brief playTextAudio
     * @param id
     * @param text
     * @return
     */
    bool startAppendPlayText(const QString &id, const QString &text, bool isEnd);

    /**
     * @brief stopPlayTextAudio
     * @return
     */
    bool stopPlayTextAudio();

    /**
     * @brief playSystemSound
     * @param effect
     */
    void playSystemSound(AudioSystemEffect effect);

signals:
    /**
     * @brief levelUpdated
     * @param level
     */
    void levelUpdated(int level);

    /**
     * @brief textReceived
     * @param text
     */
    void textReceived(const QString &text, bool isEnd);

    /**
     * @brief playTextFinished
     */
    void playTextFinished(const QString &id);

    /**
     * @brief error
     * @param code
     * @param errorString
     */
    void recordError(int code, const QString &errorString);

    /**
     * @brief error
     * @param code
     * @param errorString
     */
    void playerError(int code, const QString &errorString);

    /**
     * @brief playDeviceChanged
     */
    void playDeviceChanged(bool valid);

    /**
     * @brief recordDeviceChange
     */
    void recordDeviceChange(bool valid);

private slots:
    /**
     * @brief prepare
     */
    void prepare();

    /**
     * @brief audioRecorded
     * @param data
     */
    void onAudioRecorded(QByteArray data);

    /**
     * @brief recordStoped
     */
    void onRecordStoped();

    /**
     * @brief onRecordStarted
     */
    void onRecordStarted();

    /**
     * @brief ttsAudioData
     * @param data
     * @param isLast
     */
    void ttsAudioData(const QString &id, const QByteArray &data, bool isLast);

    /**
     * @brief playerStarted
     */
    void onPlayerStreamStarted(const QString &id);

    /**
     * @brief onAppendPlayText
     * @param id
     * @param text
     * @param isEnd
     */
    void onAppendPlayText(const QString &id, const QString &text, bool isStart, bool isEnd);

    /**
     * @brief ttsServerError
     * @param code
     * @param errorString
     */
    void ttsServerError(int code, const QString &errorString);

    /**
     * @brief iatserverError
     * @param code
     * @param errorString
     */
    void iatserverError(int code, const QString &errorString);

    /**
     * @brief clearTtsData
     */
    void clearTtsData();

private:
    /**
     * @brief initIatServer
     */
    void resetIatServer();

    /**
     * @brief initTtsServer
     */
    void resetTtsServer(const QString &id);

    /**
     * @brief formatError
     * @param code
     * @param errorMessage
     * @return
     */
    QPair<int, QString> formatError(int code, QString errorMessage);

private:
    QSharedPointer<AudioRecorder> m_audioRecorder;
    QSharedPointer<IatServer> m_iatServer;

    QSharedPointer<AudioPlayer> m_audioPlayer;
    QSharedPointer<TtsServer> m_ttsServer;

    QByteArray m_audioData;
    QTemporaryDir m_tempDir;

    AudioModel m_audioModel = AudioModel::NetWork;
};

#endif // AUDIOCONTROLER_H
