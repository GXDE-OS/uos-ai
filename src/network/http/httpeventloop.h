#ifndef UOSAI_HTTPEVENTLOOP_H
#define UOSAI_HTTPEVENTLOOP_H

#include <QEventLoop>
#include <QNetworkReply>
#include <QTimer>
#include <QByteArray>

namespace uos_ai {

class HttpEventLoop : public QEventLoop
{
    Q_OBJECT
public:
    HttpEventLoop(QNetworkReply *pReply, const QString &requestFunName = "");
    ~HttpEventLoop();

signals:
    void sigFinished();
    void sigReadyRead(const QByteArray &data);
    void signalDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void signalUploadProgress(qint64 bytesSent, qint64 bytesTotal);

public:
    int exec(ProcessEventsFlags flags = AllEvents);
    void setHttpOutTime(int outTime) {m_httpOutTime = outTime;}

    QByteArray getHttpResult() const;
    QNetworkReply::NetworkError getNetworkError() const;
    QString getNetworkErrorString() const;
    int getHttpStatusCode() const;

public slots:
    void abortReply();

private slots:
    void onHttpReplyFinished();
    void onHttpTimeout();

    void onReadyRead();
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onDownloadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QByteArray m_result;
    QNetworkReply *m_pReply;
    QTimer m_timer;
    QNetworkReply::NetworkError m_error;
    QString m_errorString;

    int m_httpStatusCode = 0;
    int m_httpOutTime = 5 * 1000;

    bool m_isTimeout = false;
    QString m_requestFunName;
};

}

#endif // UOSAI_HTTPEVENTLOOP_H
