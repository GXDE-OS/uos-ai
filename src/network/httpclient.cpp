#include "httpclient.h"
#include "http/httpmanager.h"
#include "http/httpeventloop.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logNet)

using namespace uos_ai;

HttpClient::HttpClient()
{

}

HttpClient::~HttpClient()
{
}

void HttpClient::setTimeOut(int msec)
{
    if (msec < 1000)
        msec = 1000;

    m_timeOut = msec;
}

QSharedPointer<HttpManager> HttpClient::getHttpAccessManager() const
{
    QSharedPointer<HttpManager> httpAccessManager(new HttpManager());
    return httpAccessManager;
}

HttpClient::HttpResponse HttpClient::request(const QUrl &url, const QJsonObject &data, const QVariantHash &header, QHttpMultiPart *multipart)
{
    QJsonDocument jsonDocument(data);
    const QByteArray &sendData = jsonDocument.toJson(QJsonDocument::Compact);

    QSharedPointer<HttpManager> httpAccessManager = getHttpAccessManager();
    QNetworkRequest req = httpAccessManager->baseNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "text/event-stream");

    for (auto it = header.begin(); it != header.end(); ++it)
        req.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

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

    HttpEventLoop loop(reply, "HttpClient::request");
    loop.setHttpOutTime(m_timeOut);
    connect(this, &HttpClient::doAbort, &loop, &HttpEventLoop::abortReply);
    connect(&loop, &HttpEventLoop::sigReadyRead, this, &HttpClient::readyRead);
    connect(&loop, &HttpEventLoop::sigFinished, this, &HttpClient::finished);
    loop.exec();

    QNetworkReply::NetworkError netReplyError;
    bool isAuthError = httpAccessManager->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop.getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop.getNetworkError();

    if (netReplyError != QNetworkReply::NetworkError::NoError)
        qCDebug(logNet) << "HttpClient error [" << netReplyError
        << "] code [" << loop.getHttpStatusCode() << "] error string [" << loop.getNetworkErrorString()
        << "] data [" << QString::fromUtf8(loop.getHttpResult()) << "]";
    return HttpResponse(netReplyError, loop.getNetworkErrorString(), loop.getHttpResult());
}

HttpClient::HttpResponse HttpClient::executeRequest(QNetworkReply *reply, QSharedPointer<HttpManager> http)
{
    HttpEventLoop loop(reply, "executeRequest");
    loop.setHttpOutTime(m_timeOut);
    connect(this, &HttpClient::doAbort, &loop, &HttpEventLoop::abortReply);
    connect(&loop, &HttpEventLoop::sigReadyRead, this, &HttpClient::readyRead);
    connect(&loop, &HttpEventLoop::sigFinished, this, &HttpClient::finished);
    loop.exec();

    QNetworkReply::NetworkError netReplyError;
    bool isAuthError = http->isAuthenticationRequiredError();
    if (isAuthError)
        netReplyError = QNetworkReply::NetworkError::AuthenticationRequiredError;
    else if (loop.getHttpStatusCode() == 429)
        netReplyError = QNetworkReply::NetworkError::InternalServerError;
    else
        netReplyError = loop.getNetworkError();

    if (netReplyError != QNetworkReply::NetworkError::NoError)
        qCWarning(logNet) << "HttpClient error [" << netReplyError
        << "] code [" << loop.getHttpStatusCode() << "] error string [" << loop.getNetworkErrorString()
        << "] data [" << QString::fromUtf8(loop.getHttpResult()) << "]";
    return HttpResponse(netReplyError, loop.getNetworkErrorString(), loop.getHttpResult());
}

HttpClient::HttpResponse HttpClient::get(const QUrl &url, const QVariantHash &header)
{
    QNetworkRequest req;
    auto http = createRequest(req, url, header);

    QNetworkReply *reply = http->get(req);
    return executeRequest(reply, http);
}

HttpClient::HttpResponse HttpClient::put(const QUrl &url, const QJsonObject &data, const QVariantHash &header)
{
    return put(url, QJsonDocument(data).toJson(QJsonDocument::Compact), header);
}

HttpClient::HttpResponse HttpClient::put(const QUrl &url, const QByteArray &data, const QVariantHash &header)
{
    QNetworkRequest req;
    auto http = createRequest(req, url, header);
    req.setHeader(QNetworkRequest::ContentLengthHeader, data.size());

    QNetworkReply *reply = http->put(req, data);
    return executeRequest(reply, http);
}

HttpClient::HttpResponse HttpClient::post(const QUrl &url, const QJsonObject &data, const QVariantHash &header)
{
    return post(url, QJsonDocument(data).toJson(QJsonDocument::Compact), header);
}

HttpClient::HttpResponse HttpClient::post(const QUrl &url, const QByteArray &data, const QVariantHash &header)
{
    QNetworkRequest req;
    auto http = createRequest(req, url, header);
    req.setHeader(QNetworkRequest::ContentLengthHeader, data.size());

    QNetworkReply *reply = http->post(req, data);
    return executeRequest(reply, http);
}

QSharedPointer<HttpManager> HttpClient::createRequest(QNetworkRequest &req, const QUrl &url, const QVariantHash &header)
{
    QSharedPointer<HttpManager> httpAccessManager = getHttpAccessManager();
    req = httpAccessManager->baseNetworkRequest(url);
    for (auto it = header.begin(); it != header.end(); ++it)
        req.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

    return httpAccessManager;
}
