#include "iatlocalserver.h"
#include <QLoggingCategory>

const int payloadSize = 1280;
const int payloadInterval = 40;

Q_DECLARE_LOGGING_CATEGORY(logAudio)

IatLocalServer::IatLocalServer(QObject *parent)
    : IatServer(parent)
{
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::recordError, this, &IatLocalServer::error);
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::textReceived, this, &IatLocalServer::onTextMessageReceived);

    m_processTimer.setInterval(payloadInterval);
    connect(&m_processTimer, &QTimer::timeout, this, &IatLocalServer::processData);
}

void IatLocalServer::cancel()
{
    m_isOpen = false;

    if (m_data.isEmpty()) {
        normalExitServer();
    } else {
        qCDebug(logAudio) << "Starting process timer for remaining data";
        m_processTimer.start();
    }
}

void IatLocalServer::processData()
{
    if (m_data.isEmpty()) {
        qCDebug(logAudio) << "No more data to process, stopping timer";
        m_processTimer.stop();
        return;
    }

    QByteArray extractedData = m_data.left(payloadSize);
    m_data.remove(0, payloadSize);
    m_dbus.sendRecordedData(extractedData);

    if (!m_isOpen) {
        qCDebug(logAudio) << "Server closed, performing normal exit";
        normalExitServer();
    }
}

void IatLocalServer::openServer()
{
    if (!m_isOpen) {
        qCDebug(logAudio) << "Opening server connection";
        m_isOpen = m_dbus.start();
        if (!m_isOpen) {
            qCWarning(logAudio) << "Failed to open server connection";
        }
    }
}

void IatLocalServer::sendData(const QByteArray &data)
{
    if (!m_isOpen) {
        qCDebug(logAudio) << "Server not open, ignoring data";
        return;
    }

    qCDebug(logAudio) << "Received data, size:" << data.size();
    m_data += data;

    if (m_processTimer.isActive())
        return;

    qCDebug(logAudio) << "Starting process timer";
    m_processTimer.start();
}

void IatLocalServer::normalExitServer()
{
    qCDebug(logAudio) << "Performing normal server exit";
    m_data.clear();
    m_processTimer.stop();
    m_dbus.stop();
}

void IatLocalServer::onTextMessageReceived(const QString &message, bool isEnd)
{
    qCDebug(logAudio) << "Text message received, length:" << message.length() << "isEnd:" << isEnd;
    emit textReceived(message, isEnd);
    if (isEnd) {
        qCDebug(logAudio) << "End of message received, performing normal exit";
        normalExitServer();
    }
}
