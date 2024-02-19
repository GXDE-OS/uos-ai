#ifndef HTTPACCESSMANAGER_H
#define HTTPACCESSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>

class HttpAccessmanager : public QObject
{
    Q_OBJECT
public:
    explicit HttpAccessmanager(const std::string &user, const std::string &password);
    explicit HttpAccessmanager(const QString &token);
    ~HttpAccessmanager();

public:
    QNetworkReply *get(const QNetworkRequest &request);
    QNetworkReply *post(const QNetworkRequest &request, const QByteArray &data);
    QNetworkReply *post(const QNetworkRequest &request, QHttpMultiPart *multiPart);
    QNetworkReply *sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, const QByteArray &data);

    QNetworkRequest baseNetWorkRequest(const QUrl &url, bool useSsl = true) const;

    int verify(const QString &url);
    bool isAuthenticationRequiredError() const;

    void setHttpProxy(const QString &host, quint16 port, const QString &user, const QString &pass);
    void setSocketProxy(const QString &host, quint16 port, const QString &user, const QString &pass);
    void setSystemProxy(const QString &host, quint16 port);

private slots:
    void onAuthorizeResponse(QNetworkReply *reply, QAuthenticator *authenticator);

private:
    QNetworkAccessManager *m_manager = nullptr;
    QString m_user;
    QString m_passwd;
    QString m_token;

    int m_retries = 0;
};

#endif // HTTPACCESSMANAGER_H
