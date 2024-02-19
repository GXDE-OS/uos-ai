#ifndef AUTHWEBURL_H
#define AUTHWEBURL_H

#include "serverdefs.h"

#include <QWebSocket>
#include <QSharedPointer>

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
    static QSharedPointer<QWebSocket> webSocket(const SocketProxy &socketProxy);
};

#endif // AUTHWEBURL_H
