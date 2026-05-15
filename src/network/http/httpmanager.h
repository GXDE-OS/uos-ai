#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkProxy>

namespace uos_ai {

class HttpManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpManager();
    ~HttpManager();

public:
    QNetworkReply *get(const QNetworkRequest &request);
    QNetworkReply *put(const QNetworkRequest &request, const QByteArray &data);
    QNetworkReply *post(const QNetworkRequest &request, const QByteArray &data);
    QNetworkReply *post(const QNetworkRequest &request, QHttpMultiPart *multiPart);
    QNetworkReply *sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, const QByteArray &data);

    QNetworkRequest baseNetworkRequest(const QUrl &url, bool useSsl = true) const;

    void setHttpProxy(const QString &host, quint16 port, const QString &user, const QString &pass);
    void setSocketProxy(const QString &host, quint16 port, const QString &user, const QString &pass);
    void setSystemProxy(const QString &host, quint16 port);

    inline bool isAuthenticationRequiredError() const {
        return  m_authenticationRequiredError;
    }
private slots:
    void onAuthorizeResponse(QNetworkReply *reply, QAuthenticator *authenticator);

protected:
    QNetworkAccessManager *m_manager = nullptr;
    bool m_authenticationRequiredError = false;
};

} // namespace uos_ai

#endif // HTTPMANAGER_H
