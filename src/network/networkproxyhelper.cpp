#include "networkproxyhelper.h"

#pragma push_macro("signals")
#undef signals
#include <gio/gio.h>
#pragma pop_macro("signals")

#include <QCoreApplication>
#include <QNetworkProxyFactory>
#include <QProcessEnvironment>
#include <QWebSocket>

Q_LOGGING_CATEGORY(logProxyHelper, "uos-ai.network.proxy")

namespace {

QString gsettingsString(const char *schemaId, const char *key)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (!source)
        return {};

    GSettingsSchema *schema = g_settings_schema_source_lookup(source, schemaId, TRUE);
    if (!schema)
        return {};
    g_settings_schema_unref(schema);

    GSettings *settings = g_settings_new(schemaId);
    if (!settings)
        return {};

    gchar *raw = g_settings_get_string(settings, key);
    const QString value = raw ? QString::fromUtf8(raw).trimmed() : QString();
    g_free(raw);
    g_object_unref(settings);
    return value;
}

int gsettingsInt(const char *schemaId, const char *key)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (!source)
        return 0;

    GSettingsSchema *schema = g_settings_schema_source_lookup(source, schemaId, TRUE);
    if (!schema)
        return 0;
    g_settings_schema_unref(schema);

    GSettings *settings = g_settings_new(schemaId);
    if (!settings)
        return 0;

    const int value = g_settings_get_int(settings, key);
    g_object_unref(settings);
    return value;
}

bool isDesktopProxyEnabled(QString *modeOut = nullptr)
{
    const QString mode = gsettingsString("org.gnome.system.proxy", "mode");
    if (modeOut)
        *modeOut = mode;
    return !mode.isEmpty() && mode != QLatin1String("none");
}

QNetworkProxy desktopProxyForUrl(const QUrl &targetUrl, QString *source)
{
    const QString scheme = targetUrl.scheme().toLower();
    QString host;
    int port = 0;
    QNetworkProxy::ProxyType type = QNetworkProxy::NoProxy;
    QString sourceName;

    if (scheme == QLatin1String("https")) {
        host = gsettingsString("org.gnome.system.proxy.https", "host");
        port = gsettingsInt("org.gnome.system.proxy.https", "port");
        type = QNetworkProxy::HttpProxy;
        sourceName = QStringLiteral("system:https");
        if (host.isEmpty() || port <= 0) {
            host = gsettingsString("org.gnome.system.proxy.http", "host");
            port = gsettingsInt("org.gnome.system.proxy.http", "port");
            sourceName = QStringLiteral("system:http-fallback");
        }
    } else if (scheme == QLatin1String("http")) {
        host = gsettingsString("org.gnome.system.proxy.http", "host");
        port = gsettingsInt("org.gnome.system.proxy.http", "port");
        type = QNetworkProxy::HttpProxy;
        sourceName = QStringLiteral("system:http");
    } else if (scheme == QLatin1String("wss") || scheme == QLatin1String("ws")) {
        host = gsettingsString("org.gnome.system.proxy.https", "host");
        port = gsettingsInt("org.gnome.system.proxy.https", "port");
        type = QNetworkProxy::HttpProxy;
        sourceName = QStringLiteral("system:https-ws");
        if (host.isEmpty() || port <= 0) {
            host = gsettingsString("org.gnome.system.proxy.http", "host");
            port = gsettingsInt("org.gnome.system.proxy.http", "port");
            sourceName = QStringLiteral("system:http-ws");
        }
        if (host.isEmpty() || port <= 0) {
            host = gsettingsString("org.gnome.system.proxy.socks", "host");
            port = gsettingsInt("org.gnome.system.proxy.socks", "port");
            type = QNetworkProxy::Socks5Proxy;
            sourceName = QStringLiteral("system:socks-fallback");
        }
    }

    if (host.isEmpty() || port <= 0)
        return {};

    QNetworkProxy proxy;
    proxy.setType(type);
    proxy.setHostName(host);
    proxy.setPort(static_cast<quint16>(port));
    if (source)
        *source = sourceName;
    return proxy;
}

QString proxyEnvValue(const QStringList &names)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &name : names) {
        const QString value = env.value(name).trimmed();
        if (!value.isEmpty())
            return value;
    }
    return {};
}

QUrl normalizeProxyUrl(QString value, const QString &defaultScheme)
{
    value = value.trimmed();
    if (value.isEmpty())
        return {};

    if (!value.contains(QLatin1String("://")))
        value.prepend(defaultScheme + QLatin1String("://"));

    return QUrl(value);
}

QNetworkProxy proxyFromUrl(const QUrl &url)
{
    if (!url.isValid() || url.host().isEmpty())
        return {};

    QNetworkProxy proxy;
    const QString scheme = url.scheme().toLower();
    if (scheme == QLatin1String("socks5") || scheme == QLatin1String("socks5h")) {
        proxy.setType(QNetworkProxy::Socks5Proxy);
    } else if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
        proxy.setType(QNetworkProxy::HttpProxy);
    } else {
        return {};
    }

    proxy.setHostName(url.host());
    proxy.setPort(static_cast<quint16>(url.port(proxy.type() == QNetworkProxy::HttpProxy ? 8080 : 1080)));
    proxy.setUser(url.userName());
    proxy.setPassword(url.password());
    return proxy;
}

QNetworkProxy proxyFromEnvironment(const QUrl &targetUrl, QString *source)
{
    const QString scheme = targetUrl.scheme().toLower();
    QString value;
    QString envName;

    if (scheme == QLatin1String("https") || scheme == QLatin1String("wss")) {
        value = proxyEnvValue({QStringLiteral("https_proxy"), QStringLiteral("HTTPS_PROXY")});
        envName = value.isEmpty() ? QString() : QStringLiteral("https_proxy");
    } else if (scheme == QLatin1String("http") || scheme == QLatin1String("ws")) {
        value = proxyEnvValue({QStringLiteral("http_proxy"), QStringLiteral("HTTP_PROXY")});
        envName = value.isEmpty() ? QString() : QStringLiteral("http_proxy");
    }

    if (!value.isEmpty()) {
        QNetworkProxy proxy = proxyFromUrl(normalizeProxyUrl(value, QStringLiteral("http")));
        if (proxy.type() != QNetworkProxy::DefaultProxy && proxy.type() != QNetworkProxy::NoProxy) {
            if (source)
                *source = envName;
            return proxy;
        }
    }

    value = proxyEnvValue({QStringLiteral("all_proxy"), QStringLiteral("ALL_PROXY")});
    if (!value.isEmpty()) {
        QNetworkProxy proxy = proxyFromUrl(normalizeProxyUrl(value, QStringLiteral("socks5")));
        if (proxy.type() != QNetworkProxy::DefaultProxy && proxy.type() != QNetworkProxy::NoProxy) {
            if (source)
                *source = QStringLiteral("all_proxy");
            return proxy;
        }
    }

    return {};
}

} // namespace

namespace uos_ai {

class ProxySettingsWatcherPrivate : public ProxySettingsWatcher
{
public:
    ProxySettingsWatcherPrivate()
    {
        const char *schemas[] = {
            "org.gnome.system.proxy",
            "org.gnome.system.proxy.http",
            "org.gnome.system.proxy.https",
            "org.gnome.system.proxy.socks",
        };

        for (const char *schema : schemas) {
            GSettings *settings = g_settings_new(schema);
            if (!settings)
                continue;

            g_signal_connect(settings, "changed", G_CALLBACK(&ProxySettingsWatcherPrivate::onGSettingsChanged), this);
            m_settings.append(settings);
        }
    }

    ~ProxySettingsWatcherPrivate() override
    {
        for (GSettings *settings : std::as_const(m_settings))
            g_object_unref(settings);
    }

private:
    static void onGSettingsChanged(GSettings *, gchar *, gpointer userData)
    {
        auto *self = static_cast<ProxySettingsWatcherPrivate *>(userData);
        QMetaObject::invokeMethod(self, [self] {
            qCInfo(logProxyHelper) << "Desktop proxy settings changed";
            emit self->proxySettingsChanged();
        }, Qt::QueuedConnection);
    }

    QList<GSettings *> m_settings;
};

ProxySettingsWatcher *ProxySettingsWatcher::instance()
{
    static ProxySettingsWatcher *watcher = []() -> ProxySettingsWatcher * {
        auto *app = QCoreApplication::instance();
        if (!app)
            return nullptr;
        return new ProxySettingsWatcherPrivate();
    }();
    return watcher;
}

void enableQtSystemProxyConfiguration()
{
    QNetworkProxyFactory::setUseSystemConfiguration(true);
}

QNetworkProxy resolveProxyForUrl(const QUrl &url, QString *source)
{
    if (source)
        source->clear();

    QString desktopMode;
    const bool desktopProxyEnabled = isDesktopProxyEnabled(&desktopMode);
    qCInfo(logProxyHelper) << "Resolving proxy for" << url << ", desktop proxy mode:" << desktopMode;

    if (!desktopProxyEnabled) {
        if (source)
            *source = QStringLiteral("system-disabled");
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    QNetworkProxy desktopProxy = desktopProxyForUrl(url, source);
    if (desktopProxy.type() != QNetworkProxy::DefaultProxy && desktopProxy.type() != QNetworkProxy::NoProxy)
        return desktopProxy;

    if (source)
        *source = QStringLiteral("system-unresolved");
    return QNetworkProxy(QNetworkProxy::NoProxy);
}

void applyProxyToNetworkAccessManager(QNetworkAccessManager *manager, const QUrl &url,
                                      const QLoggingCategory &category,
                                      const char *context)
{
    if (!manager || !url.isValid())
        return;

    QString proxySource;
    const QNetworkProxy proxy = resolveProxyForUrl(url, &proxySource);
    if (proxy.type() != QNetworkProxy::DefaultProxy && proxy.type() != QNetworkProxy::NoProxy) {
        manager->setProxy(proxy);
        qCInfo(category) << context << "proxy resolved from" << proxySource << ":"
                         << describeProxy(proxy) << "for" << url;
        return;
    }

    manager->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    qCInfo(category) << context << "proxy disabled, source=" << proxySource << "for" << url;
}

void applyProxyToWebSocket(QWebSocket *socket, const QUrl &url,
                           const QLoggingCategory &category,
                           const char *context)
{
    if (!socket || !url.isValid())
        return;

    QString proxySource;
    const QNetworkProxy proxy = resolveProxyForUrl(url, &proxySource);
    if (proxy.type() != QNetworkProxy::DefaultProxy && proxy.type() != QNetworkProxy::NoProxy) {
        socket->setProxy(proxy);
        qCInfo(category) << context << "proxy resolved from" << proxySource << ":"
                         << describeProxy(proxy) << "for" << url;
        return;
    }

    socket->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    qCInfo(category) << context << "proxy disabled, source=" << proxySource << "for" << url;
}

QString describeProxy(const QNetworkProxy &proxy)
{
    QString type;
    switch (proxy.type()) {
    case QNetworkProxy::HttpProxy:
        type = QStringLiteral("HttpProxy");
        break;
    case QNetworkProxy::Socks5Proxy:
        type = QStringLiteral("Socks5Proxy");
        break;
    case QNetworkProxy::NoProxy:
        type = QStringLiteral("NoProxy");
        break;
    case QNetworkProxy::DefaultProxy:
        type = QStringLiteral("DefaultProxy");
        break;
    default:
        type = QStringLiteral("OtherProxy");
        break;
    }

    if (proxy.hostName().isEmpty())
        return type;

    return QStringLiteral("%1 %2:%3").arg(type, proxy.hostName()).arg(proxy.port());
}

} // namespace uos_ai
