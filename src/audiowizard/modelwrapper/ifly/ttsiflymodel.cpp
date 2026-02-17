#include "ttsiflymodel.h"
#include "authweburl.h"
#include "networkdefs.h"
#include "ttscodetranslation.h"

#include <QTimer>
#include <QRegularExpression>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

const int CHUNK_SIZE = 1000;
const int STATUS_LAST_FRAME = 2;

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

TtsIflyModel::TtsIflyModel(const QString &id, QObject *parent)
    : TtsModel(id, parent)
{
    qCDebug(logAudioWizard) << "Initializing TTS iFly model with ID:" << id;
    m_account = AccountProxy::xfInlineAccount();
    m_account.socketProxy.socketProxyType = SocketProxyType::NO_PROXY;
    m_web = AuthWebUrl::webSocket(m_account.socketProxy);

    connect(m_web.data(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [ = ](QAbstractSocket::SocketError errorCode) {
        // 这里有时候正常对出了，服务器也要返回一个RemoteHostClosedError
        if (QAbstractSocket::RemoteHostClosedError == errorCode && m_normalExit) {
            m_normalExit = false;
            return;
        }

        qCWarning(logAudioWizard) << "WebSocket error:" << errorCode << m_web->errorString();
        AIServer::ErrorType serverError = AIServer::socketErrorToAiServerError(static_cast<QAbstractSocket::SocketError>(errorCode));
        QString errorMessage = TtsCodeTranslation::serverCodeTranslation(serverError, m_web->errorString());

        // Qt组装的，去掉Qt起始信息
        if (errorMessage.startsWith("QWebSocketPrivate::processHandshake:"))
            errorMessage = errorMessage.mid(QString("QWebSocketPrivate::processHandshake:").length()).trimmed();

        emit error(serverError, errorMessage);
    });
    connect(m_web.data(), &QWebSocket::textMessageReceived, this, &TtsIflyModel::onTextMessageReceived);
    connect(m_web.data(), &QWebSocket::disconnected, this, &TtsIflyModel::onDisconnected);
    connect(m_web.data(), &QWebSocket::connected, this, &TtsIflyModel::onConnected);
}

void TtsIflyModel::cancel()
{
    clear();
    m_normalExit = true;
    m_error = AIServer::OperationCanceledError;
    m_web->close(QWebSocketProtocol::CloseCodeNormal);
}

void TtsIflyModel::sendText(const QString &text, bool isStart, bool isEnd)
{
    if (isStart) {
        qCDebug(logAudioWizard) << "Starting new TTS session";
        clear();
    }

    m_text += text;
    m_isEnd = isEnd;

    if (m_web->state() == QAbstractSocket::UnconnectedState) {
        const QUrl &url = AuthWebUrl::createUrl("GET", "wss://tts-api.xfyun.cn/v2/tts", m_account.apiKey, m_account.apiSecret);
        qCDebug(logAudioWizard) << "Connecting to TTS server:" << url.toString(QUrl::RemoveUserInfo);
        m_web->open(url);
    }

    sendServer();
}

void TtsIflyModel::clear()
{
    m_isEnd = false;
    m_error = -1;
    m_text.clear();
    m_chunkTexts.clear();
}

QStringList TtsIflyModel::splitString(const QString &inputString)
{
    QStringList results;
    QRegularExpression re(R"([\s,，.。;；?!！]+)"); // 匹配非中文非英文的字符
    QStringList chunks = inputString.split(re);

    QString chunk;
    int charCount = 0;

    for (const QString &str : chunks) {
        if (chunk.isEmpty()) {
            chunk = str;
            charCount = str.length();
        } else if (charCount + str.length() + 1 <= CHUNK_SIZE) { // 加1是为了考虑分隔符的长度
            chunk += " " + str;
            charCount += str.length() + 1;
        } else {
            results << chunk;
            chunk = str;
            charCount = str.length();
        }
    }

    if (!chunk.isEmpty()) {
        results << chunk;
    }

    return results;
}

void TtsIflyModel::sendServer()
{
    if (!m_isEnd && m_text.length() < CHUNK_SIZE) {
        qCDebug(logAudioWizard) << "Text length below chunk size, waiting for more data";
        return;
    }

    if ((!m_chunkTexts.isEmpty() || !m_text.isEmpty()) && m_web->state() == QAbstractSocket::ConnectedState) {
        qCDebug(logAudioWizard) << "Processing text for TTS, current length:" << m_text.length();
        m_chunkTexts << splitString(m_text);

        QString text = m_chunkTexts.value(0);
        if (text.isEmpty()) {
            qCWarning(logAudioWizard) << "Failed to split text for TTS";
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

void TtsIflyModel::onDisconnected()
{
    if (m_isEnd && m_text.isEmpty() && m_chunkTexts.isEmpty()) {
        clear();
    }
}

void TtsIflyModel::onConnected()
{
    sendServer();
}

void TtsIflyModel::continueSendText()
{
    if (m_web->state() == QAbstractSocket::UnconnectedState) {
        const QUrl &url = AuthWebUrl::createUrl("GET", "wss://tts-api.xfyun.cn/v2/tts", m_account.apiKey, m_account.apiSecret);
        m_web->open(url);
        qWarning() << "Attempting to connect to the TTS server again";
    }
    sendServer();
}

void TtsIflyModel::onTextMessageReceived(const QString &message)
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
            qCDebug(logAudioWizard) << "Received audio data, size:" << audio.size();
            emit appendAudioData(m_id, audio, finished);
        }

        if (finished) {
            qCDebug(logAudioWizard) << "TTS session completed successfully";
            clear();
            m_normalExit = true;
            m_web->close(QWebSocketProtocol::CloseCodeNormal);
        } else if (status == STATUS_LAST_FRAME) {
            qCDebug(logAudioWizard) << "Received last frame, continuing with next chunk";
            m_normalExit = true;
            m_web->close(QWebSocketProtocol::CloseCodeNormal);
            QTimer::singleShot(10, this, SLOT(continueSendText()));
        }
    } else {
        qCWarning(logAudioWizard) << "TTS server error - code:" << code << "message:" << errorMessage;
        emit error(AIServer::ContentAccessDenied, TtsCodeTranslation::serverCodeTranslation(code, errorMessage));
    }
}
