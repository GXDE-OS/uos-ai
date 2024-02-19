#include "basenetwork.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"
#include "networkdefs.h"

#include <tuple>
#include <QJsonDocument>
#include <QNetworkReply>

BaseNetWork::BaseNetWork(const AccountProxy &account)
    : m_accountProxy(account)
{

}

BaseNetWork::~BaseNetWork()
{

}

void BaseNetWork::setTimeOut(int msec)
{
    m_timeOut = msec;
}

QSharedPointer<HttpAccessmanager> BaseNetWork::getHttpNetworkAccessManager(const QString &token) const
{
    QSharedPointer<HttpAccessmanager> httpAccessManager(new HttpAccessmanager(token));

    const SocketProxy &socketProxy = m_accountProxy.socketProxy;
    if (socketProxy.socketProxyType == SocketProxyType::HTTP_PROXY)
        httpAccessManager->setHttpProxy(socketProxy.host, socketProxy.port, socketProxy.user, socketProxy.pass);
    else if (socketProxy.socketProxyType == SocketProxyType::SOCKET_PROXY)
        httpAccessManager->setSocketProxy(socketProxy.host, socketProxy.port, socketProxy.user, socketProxy.pass);
    else if (socketProxy.socketProxyType == SocketProxyType::SYSTEM_PROXY)
        httpAccessManager->setSystemProxy(socketProxy.host, socketProxy.port);

    return httpAccessManager;
}

BaseNetWork::NetWorkResponse BaseNetWork::request(const QUrl &url, const QJsonObject &data, QHttpMultiPart *multipart, const QString &token)
{
    QJsonDocument jsonDocument(data);
    const QByteArray &sendData = jsonDocument.toJson(QJsonDocument::Compact);

    QString apiToken = token;
    if (apiToken.isEmpty())
        apiToken = m_accountProxy.apiKey;

    QSharedPointer<HttpAccessmanager> httpAccessManager = getHttpNetworkAccessManager(apiToken);
    QNetworkRequest req = httpAccessManager->baseNetWorkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "text/event-stream");

    QNetworkReply *reply;
    if (multipart) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + multipart->boundary());
        reply = httpAccessManager->post(req, multipart);
        multipart->setParent(reply);
    } else if (data.isEmpty()) {
        reply = httpAccessManager->get(req);
    } else {
        req.setHeader(QNetworkRequest::ContentLengthHeader, sendData.size());
        reply = httpAccessManager->post(req, sendData);
    }

    HttpEventLoop loop(reply, "BaseNetWork::create");
    loop.setHttpOutTime(m_timeOut);
    connect(this, &BaseNetWork::requestAborted, &loop, &HttpEventLoop::abortReply);
    connect(&loop, &HttpEventLoop::sigReadyRead, this, &BaseNetWork::readyReadDeltaContent);
    connect(&loop, &HttpEventLoop::sigFinished, this, &BaseNetWork::requestFinished);
    loop.exec();

    QNetworkReply::NetworkError netReplyError;
    bool isAuthError = httpAccessManager->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop.getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop.getNetWorkError();

    AIServer::ErrorType serverErrorCode = AIServer::networkReplyErrorToAiServerError(static_cast<QNetworkReply::NetworkError>(netReplyError));
    return NetWorkResponse(serverErrorCode, loop.getNetWorkErrorString(), loop.getHttpResult());
}
