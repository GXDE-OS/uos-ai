#pragma once

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QObject>
#include <QUrl>

class QWebSocket;

namespace uos_ai {

class ProxySettingsWatcher : public QObject
{
    Q_OBJECT

public:
    static ProxySettingsWatcher *instance();

Q_SIGNALS:
    void proxySettingsChanged();
};

void enableQtSystemProxyConfiguration();

QNetworkProxy resolveProxyForUrl(const QUrl &url, QString *source = nullptr);

void applyProxyToNetworkAccessManager(QNetworkAccessManager *manager, const QUrl &url,
                                      const QLoggingCategory &category,
                                      const char *context);

void applyProxyToWebSocket(QWebSocket *socket, const QUrl &url,
                           const QLoggingCategory &category,
                           const char *context);

QString describeProxy(const QNetworkProxy &proxy);

} // namespace uos_ai
