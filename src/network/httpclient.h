#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "global_define.h"
#include "http/httpmanager.h"

#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonObject>
#include <QSharedPointer>
#include <QUrl>
#include <functional>

namespace uos_ai {

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient();
    ~HttpClient() override;

    struct HttpResponse {
        QNetworkReply::NetworkError error;
        QString errorString;
        QByteArray data;

        HttpResponse(QNetworkReply::NetworkError errortype, const QString &errorstring, const QByteArray &d)
            : error(errortype)
            , errorString(errorstring)
            , data(d)
        {
        }
    };

signals:
    void doAbort();
    void readyRead(const QByteArray &content);
    void finished();
public:
    void setTimeOut(int msec);
    QSharedPointer<HttpManager> getHttpAccessManager() const;
    HttpResponse request(const QUrl &url, const QJsonObject &data, const QVariantHash &header, QHttpMultiPart *multipart = nullptr);
    HttpResponse get(const QUrl &url, const QVariantHash &header = QVariantHash());
    HttpResponse put(const QUrl &url, const QJsonObject &data, const QVariantHash &header = QVariantHash());
    HttpResponse put(const QUrl &url, const QByteArray &data, const QVariantHash &header = QVariantHash());
    HttpResponse post(const QUrl &url, const QJsonObject &data, const QVariantHash &header = QVariantHash());
    HttpResponse post(const QUrl &url, const QByteArray &data, const QVariantHash &header = QVariantHash());
protected:
    QSharedPointer<HttpManager> createRequest(QNetworkRequest &req, const QUrl &url, const QVariantHash &header);
    HttpResponse executeRequest(QNetworkReply *reply, QSharedPointer<HttpManager> http);
protected:
    int m_timeOut = 60 * 1000;
};

}

#endif // HTTPCLIENT_H
