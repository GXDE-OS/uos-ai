#include "ttssocketserver.h"
#include "authweburl.h"
#include "networkdefs.h"
#include "ttscodetranslation.h"

#include <QTimer>
#include <QRegularExpression>

const int STATUS_LAST_FRAME     = 2;

TtsSocketServer::TtsSocketServer(const QString &id, const AccountProxy &account, QObject *parent)
    : TtsServer(id, parent)
    , m_account(account)
{
    m_account.socketProxy.socketProxyType = SocketProxyType::NO_PROXY;
    m_web = AuthWebUrl::webSocket(m_account.socketProxy);

    connect(m_web.data(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [ = ](QAbstractSocket::SocketError errorCode) {
        // 这里有时候正常对出了，服务器也要返回一个RemoteHostClosedError
        if (QAbstractSocket::RemoteHostClosedError == errorCode && m_normalExit) {
            m_normalExit = false;
            return;
        }

        qWarning() << "TtsSocketServer::request error " << errorCode << m_web->errorString();
        AIServer::ErrorType serverError = AIServer::socketErrorToAiServerError(static_cast<QAbstractSocket::SocketError>(errorCode));
        QString errorMessage = TtsCodeTranslation::serverCodeTranslation(serverError, m_web->errorString());

        // Qt组装的，去掉Qt起始信息
        if (errorMessage.startsWith("QWebSocketPrivate::processHandshake:"))
            errorMessage = errorMessage.mid(QString("QWebSocketPrivate::processHandshake:").length()).trimmed();

        emit error(serverError, errorMessage);
    });
    connect(m_web.data(), &QWebSocket::textMessageReceived, this, &TtsSocketServer::onTextMessageReceived);
    connect(m_web.data(), &QWebSocket::disconnected, this, &TtsSocketServer::onDisconnected);
    connect(m_web.data(), &QWebSocket::connected, this, &TtsSocketServer::onConnected);
}

void TtsSocketServer::cancel()
{
    clear();
    m_normalExit = true;
    m_error = AIServer::OperationCanceledError;
    m_web->close(QWebSocketProtocol::CloseCodeNormal);
}

void TtsSocketServer::openServer()
{
    if (m_web->state() != QAbstractSocket::ConnectedState) {
        const QUrl &url = AuthWebUrl::createUrl("GET", "wss://tts-api.xfyun.cn/v2/tts", m_account.apiKey, m_account.apiSecret);
        m_web->open(url);
    }
}

void TtsSocketServer::sendText(const QString &text, bool isStart, bool isEnd)
{
    if (isStart) {
        clear();
    }

    m_text  += text;
    m_isEnd = isEnd;

    if (m_web->state() == QAbstractSocket::UnconnectedState) {
        const QUrl &url = AuthWebUrl::createUrl("GET", "wss://tts-api.xfyun.cn/v2/tts", m_account.apiKey, m_account.apiSecret);
        m_web->open(url);
        qWarning() << "Attempting to connect to the TTS server again";
    }

    sendServer();
}

void TtsSocketServer::clear()
{
    m_isEnd = false;
    m_error = -1;
    m_text.clear();
    m_chunkTexts.clear();
}

void TtsSocketServer::sendServer()
{
    if (!m_isEnd && m_text.length() < chunkSize)
        return;

    if ((!m_chunkTexts.isEmpty() || !m_text.isEmpty()) && m_web->state() == QAbstractSocket::ConnectedState) {
        m_chunkTexts << splitString(m_text);

        QString text = m_chunkTexts.value(0);
        if (text.isEmpty()) {
            qWarning() << "split text error " << m_text;
            return;
        }

        m_chunkTexts.removeFirst();
        m_text.clear();

        QJsonObject data;
        data["status"] = STATUS_LAST_FRAME;
        data["text"] = QString::fromUtf8(text.toUtf8().toBase64());

        QJsonObject commonArgs;
        commonArgs.insert("app_id", m_account.appId);

        QJsonObject businessArgs;
        businessArgs.insert("aue", "raw");
        businessArgs.insert("auf", "audio/L16;rate=16000");
        businessArgs.insert("vcn", "aisjiuxu");
        businessArgs.insert("tte", "utf8");

        QJsonObject jsonObject;
        jsonObject["common"] = commonArgs;
        jsonObject["business"] = businessArgs;
        jsonObject["data"] = data;

        m_web->sendTextMessage(QJsonDocument(jsonObject).toJson());

#ifdef DEBUG_LOG
        qWarning() << "tts server text " << text;
#endif
    }
}

void TtsSocketServer::onDisconnected()
{
    if (m_isEnd && m_text.isEmpty() && m_chunkTexts.isEmpty()) {
        clear();
    }
}

void TtsSocketServer::onConnected()
{
    sendServer();
}

void TtsSocketServer::continueSendText()
{
    openServer();
    sendServer();
}

void TtsSocketServer::onTextMessageReceived(const QString &message)
{
    QJsonObject msgObj = QJsonDocument::fromJson(message.toUtf8()).object();
    int code = msgObj.value("code").toInt();
    QString errorMessage = msgObj.value("message").toString();
    if (code == 0) {
        QJsonObject dataObj = msgObj.value("data").toObject();
        int status = dataObj.value("status").toInt();
        QByteArray audio = QByteArray::fromBase64(dataObj.value("audio").toVariant().toByteArray());
        bool finished = (m_isEnd && m_text.isEmpty() && m_chunkTexts.isEmpty() && status == STATUS_LAST_FRAME);

        if (!audio.isEmpty()) {
            emit appendAudioData(m_id, audio, finished);
        }

        if (finished) {
            clear();
            m_normalExit = true;
            m_web->close(QWebSocketProtocol::CloseCodeNormal);
        } else if (status == STATUS_LAST_FRAME) {
            m_normalExit = true;
            m_web->close(QWebSocketProtocol::CloseCodeNormal);
            QTimer::singleShot(10, this, SLOT(continueSendText()));
        }
    } else {
        qWarning() << "TtsSocketServer code = " << code << " message = " << errorMessage;
        emit error(AIServer::ContentAccessDenied, TtsCodeTranslation::serverCodeTranslation(code, errorMessage));
    }
}
