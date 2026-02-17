#include "ttslocalserver.h"
#include "networkdefs.h"
#include "ttscodetranslation.h"
#include "dbuslocalspeechrecognitionrequest.h"

#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudio)

TtsLocalServer::TtsLocalServer(const QString &id, QObject *parent)
    : TtsServer(id, parent)
{
    qCDebug(logAudio) << "Initializing TTS local server with ID:" << id;
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::error, this, &TtsLocalServer::error);
    connect(&m_dbus, &DbusLocalSpeechRecognitionRequest::appendAudioData, this, &TtsLocalServer::onTextMessageReceived);
}

void TtsLocalServer::cancel()
{
    normalExitServer();
}

void TtsLocalServer::openServer()
{
    if (!m_isOpen) {
        qCDebug(logAudio) << "Opening TTS server connection";
        m_isOpen = m_dbus.starttts();
        if (!m_isOpen) {
            qCWarning(logAudio) << "Failed to open TTS server connection";
        }
    }
}

void TtsLocalServer::sendText(const QString &text, bool isStart, bool isEnd)
{
    qCDebug(logAudio) << "Received text for TTS, length:" << text.length() 
                              << "isStart:" << isStart << "isEnd:" << isEnd;
    if (isStart) {
        clear();
    }

    m_text += text;
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
    if (!m_isEnd && m_text.length() < chunkSize) {
        qCDebug(logAudio) << "Text length below chunk size, waiting for more data";
        return;
    }

    if ((!m_chunkTexts.isEmpty() || !m_text.isEmpty())) {
        m_chunkTexts << splitString(m_text);

        QString text = m_chunkTexts.value(0);
        if (text.isEmpty()) {
            qCWarning(logAudio) << "Failed to split text:" << m_text;
            return;
        }

        m_chunkTexts.removeFirst();
        m_text.clear();

        qCDebug(logAudio) << "Sending text chunk to TTS server, length:" << text.length();
        m_dbus.appendText(id(), text, m_isEnd);
    }
}

void TtsLocalServer::continueSendText()
{
    openServer();
    sendServer();
}

void TtsLocalServer::normalExitServer()
{
    qCDebug(logAudio) << "Performing normal server exit";
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
