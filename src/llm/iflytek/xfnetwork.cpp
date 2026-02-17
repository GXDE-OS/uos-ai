#include "xfnetwork.h"
#include "timereventloop.h"
#include "networkdefs.h"
#include "xfcodetranslation.h"
#include "authweburl.h"
#include "servercodetranslation.h"
#include "basenetwork.h"

#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)

XFNetWork::XFNetWork(const AccountProxy &account, QObject *parent)
    : QObject(parent)
    , m_accountProxy(account)
{

}

QString XFNetWork::version(int model) const
{
    QString version;
    if (model == LLMChatModel::SPARKDESK_2) {
        version = "v2.1";
    } else if (model == LLMChatModel::SPARKDESK_3) {
        version = "v3.1";
    } else {
        version = "v1.1";
    }
    return version;
}

QJsonObject XFNetWork::header() const
{
    return QJsonObject{
        {"app_id", m_accountProxy.appId},
        {"uid", QUuid::createUuid().toString(QUuid::Id128).left(32)}
    };
}

QJsonObject XFNetWork::parameter(int model, qreal temperature, const QString &url) const
{
    QString domain;
    if (model == LLMChatModel::SPARKDESK_2) {
        domain = "generalv2";
    } else if (model == LLMChatModel::SPARKDESK_3) {
        domain = "generalv3";
    } else if (model == LLMChatModel::SPARKDESK && !url.isEmpty()) {
        if (url.contains("v4.0"))
            domain = "4.0Ultra";
        else if (url.contains("max-32k"))
            domain = "max-32k";
        else if (url.contains("v3.5"))
            domain = "generalv3.5";
        else if (url.contains("pro-128k"))
            domain = "pro-128k";
        else if (url.contains("v3.1"))
            domain = "generalv3";
        else if (url.contains("v1.1"))
            domain = "lite";
        else if (url.contains("kjwx"))
            domain = "kjwx";
    } else {
        domain = "general";
    }

    return QJsonObject{
        {
            "chat", QJsonObject{
                {"domain", domain},
                {"random_threshold", 0.5},
                {"auditing", "default"},
                {"temperature", qBound(0.0, temperature, 1.0)}
            }
        }
    };
}

QJsonObject XFNetWork::payloadMessage(const XFConversation &conversation) const
{
    return QJsonObject{
        {
            "text", conversation.getConversions()
        }
    };
}

QJsonObject XFNetWork::payloadFunctions(const QJsonArray &functions) const
{
    return QJsonObject{
        {
            "text", functions
        }
    };
}

QPair<int, QByteArray> XFNetWork::wssRequest(const QByteArray &sendData, const QString &path)
{
    qCDebug(logLLM) << "XFNetWork Starting WebSocket request to path:" << path;
    TimerEventLoop loop;
    int retError = -1;

    QSharedPointer<QWebSocket> websocket = AuthWebUrl::webSocket(m_accountProxy.socketProxy);
    connect(websocket.data(), &QWebSocket::connected, [websocket, sendData]() {
        qCDebug(logLLM) << "XFNetWork WebSocket connected, sending data";
        websocket->sendTextMessage(sendData);
    });
    connect(websocket.data(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [ &loop, &retError, websocket](QAbstractSocket::SocketError error) {
        retError = error;
        qCWarning(logLLM) << "XFNetWork WebSocket error:" << websocket->errorString();
        loop.quit();
    });

    connect(websocket.data(), &QWebSocket::readChannelFinished, [&loop]() {
        loop.quit();
    });

    connect(this, &XFNetWork::requestAborted, [&loop, websocket]() {
        websocket->close(QWebSocketProtocol::CloseCodeNormal);
        loop.exit(TimerEventLoop::EVENTLOOP_USER_CANCEL);
    });

    QByteArray responseData;
    bool wssSuccess = false;
    connect(websocket.data(), &QWebSocket::textMessageReceived, [&loop, &responseData, &wssSuccess, this](const QString & message) {
        loop.resetTime();

        responseData += message.toUtf8();

        QJsonObject msgHeaderObj = QJsonDocument::fromJson(message.toUtf8()).object().value("header").toObject();
        wssSuccess = msgHeaderObj["code"].toInt() ? false : true;

        emit readyReadDeltaContent(message.toUtf8());
    });

    websocket->open(AuthWebUrl::createUrl("GET", path, m_accountProxy.apiKey, m_accountProxy.apiSecret));

    loop.setTimeout(15000);
    int loopRet = loop.exec();

    if (loopRet == TimerEventLoop::EVENTLOOP_TIME_OUT) {
        retError = QAbstractSocket::SocketTimeoutError;
        qCWarning(logLLM) << "XFNetWork WebSocket request timed out after 15 seconds";
    }

    AIServer::ErrorType serverError = AIServer::socketErrorToAiServerError(static_cast<QAbstractSocket::SocketError>(retError));
    if (loopRet == TimerEventLoop::EVENTLOOP_USER_CANCEL)
        serverError = AIServer::OperationCanceledError;

    const QJsonObject &headerobject = QJsonDocument::fromJson(responseData).object().value("header").toObject();
    if (wssSuccess)
        serverError = AIServer::NoError;

    if (headerobject.contains("code") && headerobject.value("code").toInt() > 0) {
        int code = headerobject.value("code").toInt();
        QString message = headerobject.value("message").toString();
        if (code == 11202 || code == 11203) {
            serverError = AIServer::ServerRateLimitError;
            qCWarning(logLLM) << "XFNetWork Server rate limit error occurred";
        } else if (code == 10005 && message.contains("app_id")) {
            serverError = AIServer::AuthenticationRequiredError;
            qCWarning(logLLM) << "XFNetWork Authentication error occurred - invalid app_id";
        } else if (code == 10163) {
            serverError = AIServer::ContentExceededError;
            qCWarning(logLLM) << "XFNetWork Content exceeded error occurred";
        } else {
            serverError = AIServer::ContentAccessDenied;
            qCWarning(logLLM) << "XFNetWork Content access denied with code:" << code;
        }

        QString errorMessage = ServerCodeTranslation::serverCodeTranslation(serverError, QString());
        if (errorMessage.isEmpty())
            responseData = XFCodeTranslation::serverCodeTranslation(code, message).toUtf8();
        else
            responseData = errorMessage.toUtf8();

        qCWarning(logLLM) << "XFNetWork::request error " << code << responseData.data();
    } else if (serverError != AIServer::NoError) {
        if (websocket->errorString().contains("Unauthorized")) {
            serverError = AIServer::AuthenticationRequiredError;
            qCWarning(logLLM) << "XFNetWork Unauthorized access attempt";
        }

        qCWarning(logLLM) << "Request failed with error:" << serverError 
                         << "Details:" << websocket->errorString();
        responseData = ServerCodeTranslation::serverCodeTranslation(serverError, websocket->errorString()).toUtf8();

        // Qt组装的，去掉Qt起始信息
        if (responseData.startsWith("QWebSocketPrivate::processHandshake:"))
            responseData = responseData.mid(QString("QWebSocketPrivate::processHandshake:").length()).trimmed();
    }

    qCDebug(logLLM) << "WXFNetWork ebSocket request completed with status:" << serverError;
    websocket->disconnect();

    return qMakePair(serverError, responseData);
}

QPair<int, QByteArray> XFNetWork::httpRequest(const QJsonObject &data, const QString &path)
{
    qCDebug(logLLM) << "XFNetWork Starting HTTP request to path:" << path;
    QUrl url = AuthWebUrl::createUrl("POST", path, m_accountProxy.apiKey, m_accountProxy.apiSecret);

    BaseNetWork httpNetwork(m_accountProxy);
    httpNetwork.setTimeOut(60000);

    connect(this, &XFNetWork::requestAborted, &httpNetwork, &BaseNetWork::requestAborted);
    BaseNetWork::NetWorkResponse baseresult = httpNetwork.request(url, data, nullptr);

    const QJsonObject &headerobject = QJsonDocument::fromJson(baseresult.data).object().value("header").toObject();
    if (headerobject.contains("code") && headerobject.value("code").toInt() > 0) {
        int code = headerobject.value("code").toInt();
        QString message = headerobject.value("message").toString();
        if (code == 11202 || code == 11203) {
            baseresult.error = AIServer::ServerRateLimitError;
            qCWarning(logLLM) << "XFNetWork Server rate limit error occurred";
        } else if (code == 10005 && message.contains("app_id")) {
            baseresult.error = AIServer::AuthenticationRequiredError;
            qCWarning(logLLM) << "XFNetWork Authentication error occurred - invalid app_id";
        } else {
            baseresult.error = AIServer::ContentAccessDenied;
            qCWarning(logLLM) << "XFNetWork Content access denied with code:" << code;
        }

        QString errorMessage = ServerCodeTranslation::serverCodeTranslation(baseresult.error, QString());
        if (errorMessage.isEmpty())
            baseresult.data = XFCodeTranslation::serverCodeTranslation(code, message).toUtf8();
        else
            baseresult.data = errorMessage.toUtf8();
    } else if (baseresult.error != AIServer::NoError) {
        if (baseresult.errorString.contains("Unauthorized")) {
            baseresult.error = AIServer::AuthenticationRequiredError;
        }

        qWarning() << "XFNetWork::request error " << baseresult.error << baseresult.errorString << baseresult.data;
        baseresult.data = ServerCodeTranslation::serverCodeTranslation(baseresult.error, baseresult.errorString).toUtf8();

        // Qt组装的，去掉Qt起始信息
        if (baseresult.data.startsWith("QWebSocketPrivate::processHandshake:"))
            baseresult.data = baseresult.data.mid(QString("QWebSocketPrivate::processHandshake:").length()).trimmed();

        qCWarning(logLLM) << "XFNetWork HTTP request failed with error:" << baseresult.error 
                << "Details:" << baseresult.errorString;
    }

    return qMakePair(baseresult.error, baseresult.data);
}
