#ifndef AUTHWEBURL_H
#define AUTHWEBURL_H

#include <QWebSocket>
#include <QSharedPointer>

namespace uos_ai {

class AuthWebUrl
{
public:
    /**
     * @brief createUrl
     * @param rootUrl
     * @return
     */
    static QUrl createUrl(const QString &method, const QString &rootUrl, const QString &apiKey, const QString &apiSecret);

    /**
     * @brief getWebSocket
     * @return
     */
    static QSharedPointer<QWebSocket> webSocket();
};

}
#endif // AUTHWEBURL_H
