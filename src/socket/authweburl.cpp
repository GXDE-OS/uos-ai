#include "authweburl.h"

#include <QWebSocket>
#include <QUrlQuery>
#include <QMessageAuthenticationCode>

QString generateRFC1123Timestamp()
{
    const QString weekdayName[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    const QString monthName[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    QDateTime dateTime = QDateTime::currentDateTimeUtc();
    QDate date = dateTime.date();
    QTime time = dateTime.time();

    int weekday = date.dayOfWeek() - 1;

    QString day = QString("%1").arg(date.day(), 2, 10, QChar('0'));
    QString hour = QString("%1").arg(time.hour(), 2, 10, QChar('0'));
    QString minute = QString("%1").arg(time.minute(), 2, 10, QChar('0'));
    QString second = QString("%1").arg(time.second(), 2, 10, QChar('0'));

    return QString("%1, %2 %3 %4 %5:%6:%7 GMT")
           .arg(weekdayName[weekday])
           .arg(day)
           .arg(monthName[date.month()])
           .arg(date.year())
           .arg(hour)
           .arg(minute)
           .arg(second);
}

QSharedPointer<QWebSocket> AuthWebUrl::webSocket(const SocketProxy &socketProxy)
{
    QSharedPointer<QWebSocket> websocket(new QWebSocket);
    websocket->ignoreSslErrors(); // Ignore SSL errors, use with caution

    if (socketProxy.socketProxyType == SocketProxyType::HTTP_PROXY || socketProxy.socketProxyType == SocketProxyType::SOCKET_PROXY) {
        static const QMap<SocketProxyType, QNetworkProxy::ProxyType> proxyType = {
            {SocketProxyType::HTTP_PROXY, QNetworkProxy::HttpProxy},
            {SocketProxyType::SOCKET_PROXY, QNetworkProxy::Socks5Proxy}
        };

        QNetworkProxy proxy;
        proxy.setType(proxyType.value(socketProxy.socketProxyType));
        proxy.setHostName(socketProxy.host);
        proxy.setPort(socketProxy.port);
        proxy.setUser(socketProxy.user);
        proxy.setPassword(socketProxy.pass);
        websocket->setProxy(proxy);
    } else if (socketProxy.socketProxyType == SocketProxyType::SYSTEM_PROXY) {
        QNetworkProxyQuery query = QNetworkProxyQuery(socketProxy.host, socketProxy.port, "socket", QNetworkProxyQuery::TcpSocket);
        QList<QNetworkProxy> proxySettingsList = QNetworkProxyFactory::systemProxyForQuery(query);

        QNetworkProxy setting;
        Q_FOREACH (setting, proxySettingsList) {
            if (!setting.hostName().isEmpty() && setting.capabilities().testFlag(QNetworkProxy::TunnelingCapability)) {
                qInfo() << "system proxy = " << setting.type() << setting.hostName() << setting.port();
                websocket->setProxy(setting);
                break;
            }
        }
    }

    return websocket;
}

QUrl AuthWebUrl::createUrl(const QString &method, const QString &rootUrl, const QString &apiKey, const QString &apiSecret)
{
    QString host = QUrl(rootUrl).host();
    QString path = QUrl(rootUrl).path();

    QString date = generateRFC1123Timestamp();


    QString signatureOrigin = "host: " + host + "\n";
    signatureOrigin += "date: " + date + "\n";
    signatureOrigin += method + " " + path + " HTTP/1.1";

    QByteArray signature = QMessageAuthenticationCode::hash(signatureOrigin.toUtf8(), apiSecret.toUtf8(), QCryptographicHash::Sha256).toBase64();


    QString authorizationOrigin = QString("api_key=\"%1\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"%2\"")
                                  .arg(apiKey)
                                  .arg(QString(signature));

    QString authorization = QByteArray(authorizationOrigin.toUtf8()).toBase64();

    QUrl url(rootUrl);
    QUrlQuery query;
    query.addQueryItem("authorization", authorization);
    query.addQueryItem("date", date);
    query.addQueryItem("host", host);
    url.setQuery(query);

    return url;
}
