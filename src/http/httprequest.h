#ifndef HTTPSOCKECT_H
#define HTTPSOCKECT_H

#include "httpresponse.h"

#include <QUrl>
#include <QString>
#include <QMap>

class QNetworkReply;
class HttpAccessmanager;
class HttpRequest
{
public:
    explicit HttpRequest(HttpAccessmanager *accessmanager, const std::string &url);

    void setContentType(const std::string &content_type);
    void setContentLength(size_t content_length);
    void setTimeout(int milliseconds);
    void setAuthorization(const std::string &authorization);
    void setExpect(const std::string &value);

    void registerHttpDownloadProgressCallBack(const std::function<void(long long, long long)> &requestProgressFun);
    void registerHttpUploadProgressCallBack(const std::function<void(long long, long long)> &requestProgressFun);

    int verify() const;

    void abort();

    HttpResponse send(const std::string &requestStr);

private:
    QUrl m_url;
    QNetworkReply *m_reply = nullptr;

    QString m_contentType;

    int m_contentLength = 0;
    int m_timeMilliseconds = 10000;

    HttpAccessmanager *m_httpAccessManager = nullptr;
    QMap<QByteArray, QByteArray> m_rawHeaderMaps;

    std::function<void (long long, long long)> m_funHttpDownloadProgress;
    std::function<void (long long, long long)> m_funHttpUploadProgress;
};

#endif // HTTPSOCKECT_H
