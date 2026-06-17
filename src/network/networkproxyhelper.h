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

class ProxySettingsWrapper
{
public:
    enum ProxyMode{
      ProxyNone,
      ProxyAuto,
      ProxyManual
    };
static void enableQtSystemProxyConfiguration();

static QNetworkProxy resolveProxyForUrl(const QUrl &url, QString *source = nullptr);

static void applyProxyToNetworkAccessManager(QNetworkAccessManager *manager, const QUrl &url,
                                      const QLoggingCategory &category,
                                      const char *context);

static void applyProxyToWebSocket(QWebSocket *socket, const QUrl &url,
                           const QLoggingCategory &category,
                           const char *context);

static QString describeProxy(const QNetworkProxy &proxy);

};
} // namespace uos_ai
