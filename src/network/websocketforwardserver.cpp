#include "websocketforwardserver.h"

#ifdef QT_DEBUG

#include <QThread>
#include <QApplication>

using namespace uos_ai;

WebSocketForwardServer::WebSocketForwardServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_port(port)
{
    auto th = new QThread(qApp);
    moveToThread(th);
    th->start();
}

WebSocketForwardServer::~WebSocketForwardServer()
{
    stop();
}

bool WebSocketForwardServer::start()
{
    if (m_server && m_server->isListening()) {
        qWarning() << "WebSocket forward server is already listening on port" << m_port;
        return true;
    }

    m_server = new QWebSocketServer(QStringLiteral("UOS AI WebSocket Forward Server"),
                                    QWebSocketServer::NonSecureMode, this);

    connect(m_server, &QWebSocketServer::newConnection,
            this, &WebSocketForwardServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qCritical() << "Failed to start WebSocket forward server on port" << m_port
                                        << "error:" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    qInfo() << "WebSocket forward server started on port" << m_port;
    return true;
}

void WebSocketForwardServer::stop()
{
    if (!m_server)
        return;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it->socket)
            it->socket->close();
    }
    m_clients.clear();

    m_server->close();
    delete m_server;
    m_server = nullptr;

    qInfo() << "WebSocket forward server stopped";
}

bool WebSocketForwardServer::isListening() const
{
    return m_server && m_server->isListening();
}

quint16 WebSocketForwardServer::port() const
{
    return m_port;
}

int WebSocketForwardServer::clientCount() const
{
    return m_clients.size();
}

void WebSocketForwardServer::onNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString clientIp = socket->peerAddress().toString();

    ClientInfo info;
    info.socket = socket;
    info.id = clientId;
    info.ip = clientIp;
    info.connectedAt = QDateTime::currentDateTime();
    m_clients.insert(clientId, info);

    qInfo() << "Client connected:" << clientId << "IP:" << clientIp
                                 << "total clients:" << m_clients.size();

    connect(socket, &QWebSocket::textMessageReceived,
            this, [this, clientId](const QString &message) {
        forwardToOthers(clientId, message);
    });

    connect(socket, &QWebSocket::binaryMessageReceived,
            this, [this, clientId](const QByteArray &message) {
        forwardToOthers(clientId, message);
    });

    connect(socket, &QWebSocket::disconnected,
            this, [this, clientId]() {
        removeClient(clientId);
    });

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, clientId](QAbstractSocket::SocketError error) {
        qWarning() << "Client error:" << clientId << "error:" << error;
        removeClient(clientId);
    });

    Q_EMIT clientConnected(clientId);
}

void WebSocketForwardServer::removeClient(const QString &clientId)
{
    auto it = m_clients.find(clientId);
    if (it == m_clients.end())
        return;

    qInfo() << "Client disconnected:" << clientId
                                 << "remaining clients:" << m_clients.size() - 1;

    if (it->socket)
        it->socket->deleteLater();
    m_clients.erase(it);

    Q_EMIT clientDisconnected(clientId);
}

void WebSocketForwardServer::forwardToOthers(const QString &senderId, const QString &message)
{
    qDebug() << "Forwarding text message from" << senderId
                                  << "to" << m_clients.size() - 1 << "clients";

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.key() != senderId && it->socket
            && it->socket->state() == QAbstractSocket::ConnectedState) {
            it->socket->sendTextMessage(message);
        }
    }
}

void WebSocketForwardServer::forwardToOthers(const QString &senderId, const QByteArray &message)
{
    qDebug() << "Forwarding binary message from" << senderId
                                  << "size:" << message.size();

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.key() != senderId && it->socket
            && it->socket->state() == QAbstractSocket::ConnectedState) {
            it->socket->sendBinaryMessage(message);
        }
    }
}
#endif
