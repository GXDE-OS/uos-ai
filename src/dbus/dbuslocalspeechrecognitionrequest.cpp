#include "dbuslocalspeechrecognitionrequest.h"

#include <QDebug>

#define LOCAL_SPEECHRECOGNITION_SERVICE      "com.deepin.speechrecognition"
#define LOCAL_SPEECHRECOGNITION_PATH         "/com/deepin/speechrecognition"
#define LOCAL_SPEECHRECOGNITION_INTERFACE    "com.deepin.speechrecognition"

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
    if (msg.arguments().isEmpty()) return false;

    return msg.arguments().at(0).toBool();
}

bool DbusLocalSpeechRecognitionRequest::stop()
{
    QDBusMessage msg = call(QDBus::Block, "stop");
    if (msg.arguments().isEmpty()) return false;

    return msg.arguments().at(0).toBool();
}

void DbusLocalSpeechRecognitionRequest::sendRecordedData(const QByteArray &data)
{
    call("sendRecordedData", data);
}

bool DbusLocalSpeechRecognitionRequest::starttts()
{
    QDBusMessage msg = call(QDBus::Block, "starttts");
    if (msg.arguments().isEmpty()) return false;

    return msg.arguments().at(0).toBool();
}

void DbusLocalSpeechRecognitionRequest::stoptts()
{
    call(QDBus::Block, "stoptts");
}

void DbusLocalSpeechRecognitionRequest::appendText(const QString &id, const QString &text, bool isEnd)
{
    call("appendText", id, "'" + text + "'", isEnd);
}
