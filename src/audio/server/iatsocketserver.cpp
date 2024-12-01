#include "iatsocketserver.h"
#include "authweburl.h"
#include "networkdefs.h"
#include "iatcodetranslation.h"

const int STATUS_FIRST_FRAME    = 0;
const int STATUS_CONTINUE_FRAME = 1;
const int STATUS_LAST_FRAME     = 2;

const int payloadSize = 1280;
const int payloadInterval = 40;

IatSocketServer::IatSocketServer(const AccountProxy &account, QObject *parent)
    : IatServer(parent)
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

        qWarning() << "IatSocketServer::request error " << errorCode << m_web->errorString();
        AIServer::ErrorType serverError = AIServer::socketErrorToAiServerError(static_cast<QAbstractSocket::SocketError>(errorCode));
        QString errorMessage = IatCodeTranslation::serverCodeTranslation(serverError, m_web->errorString());

        // Qt组装的，去掉Qt起始信息
        if (errorMessage.startsWith("QWebSocketPrivate::processHandshake:"))
            errorMessage = errorMessage.mid(QString("QWebSocketPrivate::processHandshake:").length()).trimmed();

        emit error(serverError, errorMessage);
    });
    connect(m_web.data(), &QWebSocket::textMessageReceived, this, &IatSocketServer::onTextMessageReceived);
    connect(m_web.data(), &QWebSocket::disconnected, this, &IatSocketServer::onDisconnected);

    m_processTimer.setInterval(payloadInterval);
    connect(&m_processTimer, &QTimer::timeout, this, &IatSocketServer::processData);

    m_abnormalExitTimer.setSingleShot(true);
    connect(&m_abnormalExitTimer, &QTimer::timeout, this, [=](){
        if (m_error == AIServer::OperationCanceledError) {
            qDebug() << __func__ << "time out, server exit...";
            normalExitServer();
        }
    });
}

void IatSocketServer::cancel()
{
    if (m_data.isEmpty() || m_web->state() != QAbstractSocket::ConnectedState) {
        normalExitServer();
    } else {
        m_processTimer.start();
        m_error = AIServer::OperationCanceledError;
    }
}

void IatSocketServer::onTextMessageReceived(const QString &message)
{
    QJsonObject msgObj = QJsonDocument::fromJson(message.toUtf8()).object();
    int code = msgObj.value("code").toInt();
    QString errorMessage = msgObj.value("message").toString();
    if (code == 0) {
        QJsonObject dataObj = msgObj.value("data").toObject();
        int status = dataObj.value("status").toInt();
        m_pgsParser.appendResult(dataObj.value("result").toObject());
        emit textReceived(m_pgsParser.getText(), status == STATUS_LAST_FRAME);
        //qDebug() << __func__ << "received message: " << m_pgsParser.getText() << "    is last frame: " << (status == STATUS_LAST_FRAME);

        if (status == STATUS_LAST_FRAME) {
            if (m_abnormalExitTimer.isActive())
                m_abnormalExitTimer.stop();
            normalExitServer(); // 收到最后一帧数据后再退出
        }
    } else {
        qWarning() << "IatSocketServer code = " << code << " message = " << errorMessage;
        emit error(AIServer::ContentAccessDenied, IatCodeTranslation::serverCodeTranslation(code, errorMessage));
    }
}

void IatSocketServer::onDisconnected()
{
    clear();
}

void IatSocketServer::processData()
{
    if (m_data.isEmpty() && m_error == AIServer::OperationCanceledError) {
        m_processTimer.stop();
        //emit textReceived(m_pgsParser.getText(), true);
        m_abnormalExitTimer.start(3 * 1000);
        return;
    }

    if (m_data.isEmpty()) {
        m_processTimer.stop();
        return;
    }

    if (m_web->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray extractedData = m_data.left(payloadSize);
    if (extractedData.size() < payloadSize && m_error == AIServer::OperationCanceledError) {
        m_readStatus = STATUS_LAST_FRAME;
    } else if (extractedData.size() < payloadSize) {
        return;
    }

    m_data.remove(0, payloadSize);

    if (m_readStatus == STATUS_FIRST_FRAME) {
        m_pgsParser.clear();

        QJsonObject data;
        data["status"] = 0;
        data["format"] = "audio/L16;rate=16000";
        data["audio"] = QString::fromUtf8(extractedData.toBase64());
        data["encoding"] = "raw";

        QJsonObject commonArgs;
        commonArgs.insert("app_id", m_account.appId);

        QJsonObject businessArgs;
        businessArgs.insert("domain", "iat");
        businessArgs.insert("language", "zh_cn");
        businessArgs.insert("accent", "mandarin");
        businessArgs.insert("vad_eos", 10000);
        businessArgs.insert("dwa", "wpgs");

        QJsonObject jsonObject;
        jsonObject["common"] = commonArgs;
        jsonObject["business"] = businessArgs;
        jsonObject["data"] = data;

        m_web->sendTextMessage(QJsonDocument(jsonObject).toJson());
        m_readStatus = STATUS_CONTINUE_FRAME;
    } else if (m_readStatus == STATUS_CONTINUE_FRAME) {
        QJsonObject data;
        data["status"] = 1;
        data["format"] = "audio/L16;rate=16000";
        data["audio"] = QString::fromUtf8(extractedData.toBase64());
        data["encoding"] = "raw";

        QJsonObject jsonObject;
        jsonObject["data"] = data;

        m_web->sendTextMessage(QJsonDocument(jsonObject).toJson());
    } else if (m_readStatus == STATUS_LAST_FRAME) {
        QJsonObject data;
        data["status"] = 2;
        data["format"] = "audio/L16;rate=16000";
        data["audio"] = QString::fromUtf8(extractedData.toBase64());
        data["encoding"] = "raw";

        QJsonObject jsonObject;
        jsonObject["data"] = data;

        //qDebug() << __func__ << "sendTextMessage last frame.";
        m_web->sendTextMessage(QJsonDocument(jsonObject).toJson());
    }
}

void IatSocketServer::openServer()
{
    if (m_abnormalExitTimer.isActive()) {
        m_abnormalExitTimer.stop();
    }

    if (m_web->state() == QAbstractSocket::ConnectedState) {
        normalExitServer();
    }

    if (m_web->state() != QAbstractSocket::ConnectedState) {
        const QUrl &url = AuthWebUrl::createUrl("GET", "wss://iat-api.xfyun.cn/v2/iat", m_account.apiKey, m_account.apiSecret);
        m_web->open(url);
    }
}

void IatSocketServer::sendData(const QByteArray &data)
{
    m_data += data;

    if (m_processTimer.isActive())
        return;

    m_processTimer.start();
}

void IatSocketServer::clear()
{
    m_error = -1;
    m_pgsParser.clear();
    m_data.clear();
    m_processTimer.stop();
    m_readStatus = STATUS_FIRST_FRAME;
}

void IatSocketServer::normalExitServer()
{
    clear();
    m_normalExit = true;
    m_web->close(QWebSocketProtocol::CloseCodeNormal);
}
