#include "iatlocalserver.h"

const int payloadSize = 1280;
const int payloadInterval = 40;

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
        m_processTimer.start();
    }
}

void IatLocalServer::processData()
{
    if (m_data.isEmpty()) {
        m_processTimer.stop();
        return;
    }

    QByteArray extractedData = m_data.left(payloadSize);
    m_data.remove(0, payloadSize);
    m_dbus.sendRecordedData(extractedData);

    if (!m_isOpen)
        normalExitServer();
}

void IatLocalServer::openServer()
{
    if (!m_isOpen)
        m_isOpen = m_dbus.start();
}

void IatLocalServer::sendData(const QByteArray &data)
{
    if (!m_isOpen)
        return;

    m_data += data;

    if (m_processTimer.isActive())
        return;

    m_processTimer.start();
}

void IatLocalServer::normalExitServer()
{
    m_data.clear();
    m_processTimer.stop();
    m_dbus.stop();
}

void IatLocalServer::onTextMessageReceived(const QString &message, bool isEnd)
{
    emit textReceived(message, isEnd);
    if (isEnd) normalExitServer();
}
