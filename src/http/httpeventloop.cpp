#include "httpeventloop.h"
#include <QThread>

HttpEventLoop::HttpEventLoop(QNetworkReply *pReply, const QString &requestFunName)
    : m_pReply(pReply)
    , m_error(QNetworkReply::NoError)
    , m_requestFunName(requestFunName)
{
    //qDebug() << requestFunName + " start";
    m_result.clear();
    m_timer.setSingleShot(true);
    m_pReply->ignoreSslErrors();

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
    //qDebug() << m_requestFunName + " end";
}

QNetworkReply::NetworkError HttpEventLoop::getNetWorkError() const
{
    return m_error;
}

QString HttpEventLoop::getNetWorkErrorString() const
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

    // 超时走的abort返回错误类型会是usercancal
    if (m_isTimeout) {
        m_error = QNetworkReply::TimeoutError;
    }

    m_httpStatusCode = m_pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_error != QNetworkReply::NoError && m_error != QNetworkReply::OperationCanceledError) {
        qWarning() << QString("url=%1, error=%2, httpstatus=%3;").arg(m_pReply->url().toString()).arg(m_pReply->errorString()).arg(m_httpStatusCode);
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
    qWarning() << QString("url=%1, error=TimeOutError").arg(m_pReply->url().toString());
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
    return QEventLoop::exec(flags);
}

