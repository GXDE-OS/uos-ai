#include "httpmanager.h"
#include "httpeventloop.h"
#include "network/networkproxyhelper.h"

#include <QLoggingCategory>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkProxy>

Q_DECLARE_LOGGING_CATEGORY(logNet)

using namespace uos_ai;

HttpManager::HttpManager()
{
    m_manager = new QNetworkAccessManager;
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &HttpManager::onAuthorizeResponse);
}

HttpManager::~HttpManager()
{
    delete m_manager;
}

QNetworkReply *HttpManager::get(const QNetworkRequest &request)
{
    uos_ai::applyProxyToNetworkAccessManager(m_manager, request.url(), logNet(), "HttpManager GET");
    return m_manager->get(request);
}

QNetworkReply *HttpManager::put(const QNetworkRequest &request, const QByteArray &data)
{
    uos_ai::applyProxyToNetworkAccessManager(m_manager, request.url(), logNet(), "HttpManager PUT");
    return m_manager->put(request, data);
}

QNetworkReply *HttpManager::post(const QNetworkRequest &request, const QByteArray &data)
{
    uos_ai::applyProxyToNetworkAccessManager(m_manager, request.url(), logNet(), "HttpManager POST");
    return m_manager->post(request, data);
}

QNetworkReply *HttpManager::post(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    uos_ai::applyProxyToNetworkAccessManager(m_manager, request.url(), logNet(), "HttpManager POST multipart");
    return m_manager->post(request, multiPart);
}

QNetworkReply *HttpManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, const QByteArray &data)
{
    uos_ai::applyProxyToNetworkAccessManager(m_manager, request.url(), logNet(), "HttpManager custom");
    return m_manager->sendCustomRequest(request, verb, data);
}

QNetworkRequest HttpManager::baseNetworkRequest(const QUrl &url, bool useSsl) const
{
    QNetworkRequest req;
#ifndef QT_NO_SSL
    if (useSsl) {
        QSslConfiguration sslConfig = req.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::AutoVerifyPeer);
        req.setSslConfiguration(sslConfig);
    }
#endif
    req.setUrl(url);

    return req;
}

void HttpManager::onAuthorizeResponse(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    Q_UNUSED(authenticator);

    qCWarning(logNet) << "Authentication required - external authentication handling needed";
    m_authenticationRequiredError = true;
    reply->abort();
}

void HttpManager::setHttpProxy(const QString &host, quint16 port, const QString &user, const QString &pass)
{
    qCInfo(logNet) << "Setting HTTP proxy:" << host << ":" << port;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(host);
    proxy.setPort(port);
    proxy.setUser(user);
    proxy.setPassword(pass);
    m_manager->setProxy(proxy);
}

void HttpManager::setSocketProxy(const QString &host, quint16 port, const QString &user, const QString &pass)
{
    qCInfo(logNet) << "Setting SOCKS5 proxy:" << host << ":" << port;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName(host);
    proxy.setPort(port);
    proxy.setUser(user);
    proxy.setPassword(pass);
    m_manager->setProxy(proxy);
}

void HttpManager::setSystemProxy(const QString &host, quint16 port)
{
    const QString scheme = port == 443 ? QStringLiteral("https") : QStringLiteral("http");
    uos_ai::applyProxyToNetworkAccessManager(m_manager,
                                             QUrl(QStringLiteral("%1://%2:%3").arg(scheme, host).arg(port)),
                                             logNet(), "HttpManager SYSTEM");
}
