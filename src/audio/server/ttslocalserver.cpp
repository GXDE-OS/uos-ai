#include "ttslocalserver.h"
#include "networkdefs.h"
#include "ttscodetranslation.h"
#include "dbuslocalspeechrecognitionrequest.h"

#include <QTimer>

TtsLocalServer::TtsLocalServer(const QString &id, QObject *parent)
    : TtsServer(id, parent)
{
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::error, this, &TtsLocalServer::error);
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::appendAudioData, this, &TtsLocalServer::onTextMessageReceived);
}

void TtsLocalServer::cancel()
{
    normalExitServer();
}

void TtsLocalServer::openServer()
{
    if (!m_isOpen)
        m_isOpen = m_dbus.starttts();
}

void TtsLocalServer::sendText(const QString &text, bool isStart, bool isEnd)
{
    if (isStart) {
        clear();
    }

    m_text  += text;
    m_isEnd = isEnd;

    sendServer();
}

void TtsLocalServer::clear()
{
    m_isEnd = false;
    m_text.clear();
}

void TtsLocalServer::sendServer()
{
    if (!m_isEnd && m_text.length() < chunkSize)
        return;

    if ((!m_chunkTexts.isEmpty() || !m_text.isEmpty())) {
        m_chunkTexts << splitString(m_text);

        QString text = m_chunkTexts.value(0);
        if (text.isEmpty()) {
            qWarning() << "split text error " << m_text;
            return;
        }

        m_chunkTexts.removeFirst();
        m_text.clear();

        m_dbus.appendText(id(), text, m_isEnd);

        qWarning() << "tts server text " << text;
    }
}

void TtsLocalServer::continueSendText()
{
    openServer();
    sendServer();
}

void TtsLocalServer::normalExitServer()
{
    m_isOpen = false;
    clear();
    m_dbus.stoptts();
}

void TtsLocalServer::onTextMessageReceived(const QString &id, const QByteArray &text, bool isEnd)
{
    emit appendAudioData(id, text, isEnd);

    if (isEnd) {
        normalExitServer();
    } else {
        QTimer::singleShot(10, this, SLOT(continueSendText()));
    }
}
