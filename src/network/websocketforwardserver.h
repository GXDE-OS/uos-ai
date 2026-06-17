#ifndef WEBSOCKETFORWARDSERVER_H
#define WEBSOCKETFORWARDSERVER_H

#include <Qt>

#ifdef QT_DEBUG

#include <QWebSocketServer>
#include <QWebSocket>
#include <QUuid>
#include <QHash>

namespace uos_ai {

struct ClientInfo {
    QWebSocket *socket = nullptr;
    QString id;
    QString ip;
    QDateTime connectedAt;
};

class WebSocketForwardServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketForwardServer(quint16 port, QObject *parent = nullptr);
    ~WebSocketForwardServer();


    bool isListening() const;

    quint16 port() const;

    int clientCount() const;
public Q_SLOTS:
    bool start();
    void stop();
Q_SIGNALS:
    void clientConnected(const QString &clientId);
    void clientDisconnected(const QString &clientId);

private Q_SLOTS:
    void onNewConnection();

private:
    void removeClient(const QString &clientId);
    void forwardToOthers(const QString &senderId, const QString &message);
    void forwardToOthers(const QString &senderId, const QByteArray &message);

    QWebSocketServer *m_server = nullptr;
    QHash<QString, ClientInfo> m_clients;
    quint16 m_port;
};

}

#endif
#endif // WEBSOCKETFORWARDSERVER_H

