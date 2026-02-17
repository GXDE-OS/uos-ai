#include "dbuslocalspeechrecognitionrequest.h"

#include <QLoggingCategory>

#define LOCAL_SPEECHRECOGNITION_SERVICE      "com.deepin.speechrecognition"
#define LOCAL_SPEECHRECOGNITION_PATH         "/com/deepin/speechrecognition"
#define LOCAL_SPEECHRECOGNITION_INTERFACE    "com.deepin.speechrecognition"

Q_DECLARE_LOGGING_CATEGORY(logDBus)

DbusLocalSpeechRecognitionRequest::DbusLocalSpeechRecognitionRequest(QObject *parent)
    : QDBusInterface(LOCAL_SPEECHRECOGNITION_SERVICE, LOCAL_SPEECHRECOGNITION_PATH, LOCAL_SPEECHRECOGNITION_INTERFACE, QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(LOCAL_SPEECHRECOGNITION_SERVICE, LOCAL_SPEECHRECOGNITION_PATH, LOCAL_SPEECHRECOGNITION_INTERFACE,
                                         "appendAudioData", this,
                                         SIGNAL(appendAudioData(const QString &, const QByteArray &, bool)));
    QDBusConnection::systemBus().connect(LOCAL_SPEECHRECOGNITION_SERVICE, LOCAL_SPEECHRECOGNITION_PATH, LOCAL_SPEECHRECOGNITION_INTERFACE,
                                         "error", this,
                                         SIGNAL(error(int, const QString &)));
    QDBusConnection::systemBus().connect(LOCAL_SPEECHRECOGNITION_SERVICE, LOCAL_SPEECHRECOGNITION_PATH, LOCAL_SPEECHRECOGNITION_INTERFACE,
                                         "recordError", this,
                                         SIGNAL(recordError(int, const QString &)));
    QDBusConnection::systemBus().connect(LOCAL_SPEECHRECOGNITION_SERVICE, LOCAL_SPEECHRECOGNITION_PATH, LOCAL_SPEECHRECOGNITION_INTERFACE,
                                         "textReceived", this,
                                         SIGNAL(textReceived(const QString &, bool)));
}

bool DbusLocalSpeechRecognitionRequest::start()
{
    QDBusMessage msg = call(QDBus::Block, "start");
    if (msg.arguments().isEmpty()) {
        qCWarning(logDBus) << "Failed to start speech recognition: empty response";
        return false;
    }

    bool result = msg.arguments().at(0).toBool();
    return result;
}

bool DbusLocalSpeechRecognitionRequest::stop()
{
    QDBusMessage msg = call(QDBus::Block, "stop");
    if (msg.arguments().isEmpty()) {
        qCWarning(logDBus) << "Failed to stop speech recognition: empty response";
        return false;
    }

    bool result = msg.arguments().at(0).toBool();
    return result;
}

void DbusLocalSpeechRecognitionRequest::sendRecordedData(const QByteArray &data)
{
    call("sendRecordedData", data);
}

bool DbusLocalSpeechRecognitionRequest::starttts()
{
    QDBusMessage msg = call(QDBus::Block, "starttts");
    if (msg.arguments().isEmpty()) {
        qCWarning(logDBus) << "Failed to start text-to-speech: empty response";
        return false;
    }

    bool result = msg.arguments().at(0).toBool();
    qCDebug(logDBus) << "Text-to-speech start result:" << result;
    return result;
}

void DbusLocalSpeechRecognitionRequest::stoptts()
{
    qCDebug(logDBus) << "Stopping text-to-speech";
    call(QDBus::Block, "stoptts");
}

void DbusLocalSpeechRecognitionRequest::appendText(const QString &id, const QString &text, bool isEnd)
{
    qCDebug(logDBus) << "Appending text for id:" << id << "isEnd:" << isEnd;
    call("appendText", id, "'" + text + "'", isEnd);
}
