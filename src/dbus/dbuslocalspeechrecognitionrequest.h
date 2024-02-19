#ifndef DBUSLOCALSPEECHRECOGNITIONREQUEST_H
#define DBUSLOCALSPEECHRECOGNITIONREQUEST_H

#include <QDBusInterface>

class DbusLocalSpeechRecognitionRequest: public QDBusInterface
{
    Q_OBJECT

public:
    explicit DbusLocalSpeechRecognitionRequest(QObject *parent = nullptr);

    bool start();

    bool stop();

    void sendRecordedData(const QByteArray &);

    bool starttts();

    void stoptts();

    void appendText(const QString &id, const QString &text, bool isEnd);

signals:
    void appendAudioData(const QString &id, const QByteArray &text, bool isEnd);

    void error(int, const QString &error);

    void recordError(int, const QString &error);

    void textReceived(const QString &text, bool);
};

#endif // DBUSLOCALSPEECHRECOGNITIONREQUEST_H
