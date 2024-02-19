#ifndef TTSSERVER_H
#define TTSSERVER_H

#include <QObject>

const int chunkSize = 1000;

class TtsServer : public QObject
{
    Q_OBJECT
public:
    explicit TtsServer(const QString &id, QObject *parent = nullptr);
    virtual ~TtsServer();

    /**
     * @brief id
     * @return
     */
    QString id() const;

    /**
     * @brief setModel
     * @param model
     */
    void setModel(int model);
    int model() const;

    /**
     * @brief splitString
     * @param inputString
     * @return
     */
    QStringList splitString(const QString &inputString);

signals:
    /**
     * @brief error
     * @param code
     * @param errorString
     */
    void error(int code, const QString &errorString);

    /**
     * @brief appendAudioData
     */
    void appendAudioData(const QString &id, const QByteArray &data, bool isLast);

public:

    /**
     * @brief cancel
     */
    virtual void cancel() = 0;

    /**
     * @brief openServer
     */
    virtual void openServer() = 0;

    /**
     * @brief sendText
     * @param text
     */
    virtual void sendText(const QString &text, bool isStart, bool isEnd) = 0;

protected:
    QString m_id;
    int m_model = 0;
};

#endif // TTSSERVER_H
