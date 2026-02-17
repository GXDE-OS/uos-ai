#include "httpaccessmanager.h"
#include "httpeventloop.h"

#include <QLoggingCategory>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkProxy>

Q_DECLARE_LOGGING_CATEGORY(logHttp)

static const int tryLoginCount = 2;
HttpAccessmanager::HttpAccessmanager(const std::string &user, const std::string &password)
    : m_user(QString::fromStdString(user))
    , m_passwd(QString::fromStdString(password))
{
    m_manager = new QNetworkAccessManager;
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &HttpAccessmanager::onAuthorizeResponse);
}

HttpAccessmanager::HttpAccessmanager(const QString &token)
    : m_token(token)
{
    m_manager = new QNetworkAccessManager;
    connect(m_manager, &QNetworkAccessManager::authenticationRequired, this, &HttpAccessmanager::onAuthorizeResponse);
}

HttpAccessmanager::~HttpAccessmanager()
{
    delete m_manager;
}

QNetworkReply *HttpAccessmanager::get(const QNetworkRequest &request)
{
    m_retries = 0;
    return m_manager->get(request);
}

QNetworkReply *HttpAccessmanager::put(const QNetworkRequest &request, const QByteArray &data)
{
    m_retries = 0;
    return m_manager->put(request, data);
}

QNetworkReply *HttpAccessmanager::post(const QNetworkRequest &request, const QByteArray &data)
{
    m_retries = 0;
    return m_manager->post(request, data);
}

QNetworkReply *HttpAccessmanager::post(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    m_retries = 0;
    return m_manager->post(request, multiPart);
}

QNetworkReply *HttpAccessmanager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, const QByteArray &data)
{
    m_retries = 0;
    return m_manager->sendCustomRequest(request, verb, data);
}

QNetworkRequest HttpAccessmanager::baseNetWorkRequest(const QUrl &url, bool useSsl) const
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

    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    } else if (!m_user.isEmpty() && !m_passwd.isEmpty()) {
        req.setRawHeader("Authorization", "Basic " + QByteArray(m_user.toUtf8() + ":" + m_passwd.toUtf8()).toBase64());
    }

    return req;
}

void HttpAccessmanager::onAuthorizeResponse(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);

    if (m_user.isEmpty() && m_passwd.isEmpty()) {
        qCWarning(logHttp) << "Authentication required but no credentials provided";
        m_retries = tryLoginCount;
        reply->abort();
        return;
    }

    authenticator->setUser(m_user);
    authenticator->setPassword(m_passwd);

    m_retries++;
    if (m_retries >= tryLoginCount) {
        qCWarning(logHttp) << "Authentication failed after" << tryLoginCount << "attempts";
        reply->abort();
    }
}

int HttpAccessmanager::verify(const QString &url)
{
    qCDebug(logHttp) << "Verifying URL:" << url;
    QNetworkRequest req;
#ifndef QT_NO_SSL
    QSslConfiguration sslConfig = req.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(sslConfig);
#endif
    req.setUrl(url);

    QByteArray byteArray;
    HttpEventLoop loop(get(req));
    loop.setHttpOutTime(10000);
    loop.exec();

    if (m_retries >= tryLoginCount) {
        qCWarning(logHttp) << "Authentication required for URL:" << url;
        return QNetworkReply::NetworkError::AuthenticationRequiredError;
    }

    int error = loop.getNetWorkError();
    if (error != QNetworkReply::NoError) {
        qCWarning(logHttp) << "Network error while verifying URL:" << url << "Error:" << error;
    }
    return error;
}

bool HttpAccessmanager::isAuthenticationRequiredError() const
{
    if (m_retries >= tryLoginCount)
        return true;

    return false;
}

void HttpAccessmanager::setHttpProxy(const QString &host, quint16 port, const QString &user, const QString &pass)
{
    qCInfo(logHttp) << "Setting HTTP proxy:" << host << ":" << port;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(host);
    proxy.setPort(port);
    proxy.setUser(user);
    proxy.setPassword(pass);
    m_manager->setProxy(proxy);
}

void HttpAccessmanager::setSocketProxy(const QString &host, quint16 port, const QString &user, const QString &pass)
{
    qCInfo(logHttp) << "Setting SOCKS5 proxy:" << host << ":" << port;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName(host);
    proxy.setPort(port);
    proxy.setUser(user);
    proxy.setPassword(pass);
    m_manager->setProxy(proxy);
}

void HttpAccessmanager::setSystemProxy(const QString &host, quint16 port)
{
    QNetworkProxyQuery query = QNetworkProxyQuery(host, port, "https", QNetworkProxyQuery::TcpSocket);
    QList<QNetworkProxy> proxySettingsList = QNetworkProxyFactory::systemProxyForQuery(query);

    QNetworkProxy setting;
    Q_FOREACH (setting, proxySettingsList) {
        if (!setting.hostName().isEmpty() && setting.capabilities().testFlag(QNetworkProxy::TunnelingCapability)) {
            qCInfo(logHttp) << "system proxy = " << setting.type() << setting.hostName() << setting.port();
            m_manager->setProxy(setting);
            break;
        }
    }
}
