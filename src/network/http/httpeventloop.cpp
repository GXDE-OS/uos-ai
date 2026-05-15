#include "httpeventloop.h"

#include <QThread>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logNet)

using namespace uos_ai;

HttpEventLoop::HttpEventLoop(QNetworkReply *pReply, const QString &requestFunName)
    : m_pReply(pReply)
    , m_error(QNetworkReply::NoError)
    , m_requestFunName(requestFunName)
{
    m_result.clear();
    m_timer.setSingleShot(true);

    connect(&m_timer, &QTimer::timeout,                 this, &HttpEventLoop::onHttpTimeout);
    connect(pReply,   &QNetworkReply::finished,         this, &HttpEventLoop::onHttpReplyFinished);
    connect(pReply,   &QNetworkReply::uploadProgress,   this, &HttpEventLoop::onUploadProgress);
    connect(pReply,   &QNetworkReply::downloadProgress, this, &HttpEventLoop::onDownloadProgress);
    connect(pReply,   &QNetworkReply::readyRead,        this, &HttpEventLoop::onReadyRead);
}

HttpEventLoop::~HttpEventLoop()
{
    if (m_pReply) {
        m_pReply->close();
        m_pReply->deleteLater();
        m_pReply = nullptr;
    }
}

QNetworkReply::NetworkError HttpEventLoop::getNetworkError() const
{
    return m_error;
}

QString HttpEventLoop::getNetworkErrorString() const
{
    return m_errorString;
}

int HttpEventLoop::getHttpStatusCode() const
{
    return m_httpStatusCode;
}

void HttpEventLoop::abortReply()
{
    if (m_pReply) {
        m_timer.stop();
        m_pReply->abort();
    }
    quit();
}

void HttpEventLoop::onHttpReplyFinished()
{
    m_timer.stop();
    m_error  = m_pReply->error();
    m_errorString = m_pReply->errorString();

    if (m_isTimeout) {
        m_error = QNetworkReply::TimeoutError;
    }

    m_httpStatusCode = m_pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_error != QNetworkReply::NoError && m_error != QNetworkReply::OperationCanceledError) {
        QUrl url = m_pReply->url();
        qCWarning(logNet) << "HTTP request failed - URL:" << url.toString()
                          << "Error:" << m_errorString
                          << "Status code:" << m_httpStatusCode;
    }

    emit sigFinished();
    quit();
}

void HttpEventLoop::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    m_timer.start(m_httpOutTime);

    if (bytesTotal == 0) bytesTotal = -1;
    emit signalUploadProgress(bytesSent, bytesTotal);
}

void HttpEventLoop::onDownloadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    m_timer.start(m_httpOutTime);

    if (bytesTotal == 0) bytesTotal = -1;
    emit signalDownloadProgress(bytesSent, bytesTotal);
}

QByteArray HttpEventLoop::getHttpResult() const
{
    return m_result;
}

void HttpEventLoop::onHttpTimeout()
{
    m_isTimeout = true;
    qCWarning(logNet) << "HTTP request timed out - URL:" << m_pReply->url().toString() << "wait time:" << m_isTimeout;
    m_timer.stop();
    m_pReply->abort();
    quit();
}

void HttpEventLoop::onReadyRead()
{
    m_timer.start(m_httpOutTime);

    const QByteArray &data = m_pReply->readAll();
    m_result += data;

    emit sigReadyRead(data);
}

int HttpEventLoop::exec(QEventLoop::ProcessEventsFlags flags)
{
    m_isTimeout = false;
    m_timer.start(m_httpOutTime);
    qCDebug(logNet) << "Starting HTTP event loop for" << m_requestFunName;
    return QEventLoop::exec(flags);
}

