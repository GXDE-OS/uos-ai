#include "httprequest.h"
#include "httpaccessmanager.h"
#include "httpeventloop.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

HttpRequest::HttpRequest(HttpAccessmanager *accessmanager, const std::string &url)
{
    m_httpAccessManager = accessmanager;
    m_url.setUrl(QString::fromStdString(url));
    m_contentType = "text/xml; charset=utf-8";
}

void HttpRequest::setContentType(const std::string &content_type)
{
    m_contentType = QString::fromStdString(content_type);
}

void HttpRequest::setContentLength(size_t content_length)
{
    m_contentLength = static_cast<int>(content_length);
}

void HttpRequest::setTimeout(int milliseconds)
{
    m_timeMilliseconds = milliseconds;
}

void HttpRequest::setAuthorization(const std::string &authorization)
{
    m_rawHeaderMaps["Authorization"] = QByteArray::fromStdString(authorization);
}

void HttpRequest::setExpect(const std::string &value)
{
    m_rawHeaderMaps["Expect"] = QByteArray::fromStdString(value);
}

void HttpRequest::registerHttpDownloadProgressCallBack(const std::function<void (long long, long long)> &requestProgressFun)
{
    m_funHttpDownloadProgress = requestProgressFun;
}

void HttpRequest::registerHttpUploadProgressCallBack(const std::function<void (long long, long long)> &requestProgressFun)
{
    m_funHttpUploadProgress = requestProgressFun;
}

int HttpRequest::verify() const
{
    return m_httpAccessManager->verify(m_url.url());
}

void HttpRequest::abort()
{
    if (m_reply)
        m_reply->abort();
}

HttpResponse HttpRequest::send(const std::string &requestStr)
{
    if (m_httpAccessManager == nullptr)
        return HttpResponse();

    QNetworkRequest req;
#ifndef QT_NO_SSL
    QSslConfiguration sslConfig = req.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(sslConfig);
#endif
    req.setUrl(m_url);

    req.setHeader(QNetworkRequest::ContentTypeHeader, m_contentType);
    req.setHeader(QNetworkRequest::ContentLengthHeader, static_cast<int>(requestStr.length()));

    for (auto iter = m_rawHeaderMaps.begin(); iter != m_rawHeaderMaps.end(); iter++)
        req.setRawHeader(iter.key(), iter.value());

    QByteArray byteArray(requestStr.c_str(), static_cast<int>(requestStr.length()));

    m_reply = m_httpAccessManager->post(req, byteArray);

    HttpEventLoop loop(m_reply);
    QObject::connect(&loop, &HttpEventLoop::signalDownloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        m_funHttpDownloadProgress(bytesReceived, bytesTotal);
    });

    QObject::connect(&loop, &HttpEventLoop::signalUploadProgress, [this](qint64 bytesSent, qint64 bytesTotal) {
        m_funHttpUploadProgress(bytesSent, bytesTotal);
    });

    loop.setHttpOutTime(m_timeMilliseconds);
    loop.exec();

    m_reply = nullptr;

    QByteArray responseData = loop.getHttpResult();
    responseData.append('\0');

    bool isAuthError = m_httpAccessManager->isAuthenticationRequiredError();
    int statusCode = isAuthError ? 401 : loop.getHttpStatusCode();
    HttpResponse httpResponse = HttpResponse(statusCode, std::vector<char>(responseData.begin(), responseData.end()));
    if (isAuthError) {
        httpResponse.setErrorCode(QNetworkReply::NetworkError::AuthenticationRequiredError);
    } else {
        httpResponse.setErrorCode(loop.getNetWorkError());
    }
    return httpResponse;
}


