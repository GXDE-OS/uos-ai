#include "appsocketserver.h"

#include <QLocalServer>
#include <QLocalSocket>

#include <QJsonDocument>
#include <QJsonObject>

AppSocketServer::AppSocketServer(const QString &id, QObject *parent)
    : QObject(parent)
    , m_serverName(id)
{

}

AppSocketServer::~AppSocketServer()
{
    if (m_localServer && m_localServer->isListening()) {
        m_localServer->close();
    }
}

bool AppSocketServer::startServer()
{
    if (m_localServer == nullptr) {
        m_localServer = new QLocalServer(this);
        connect(m_localServer, &QLocalServer::newConnection, this, &AppSocketServer::handleNewConnection);
    }

    // 删除旧的套接字文件
    if (!QLocalServer::removeServer(m_serverName)) {
        qWarning() << "Failed to remove server:" << m_localServer->errorString();
    }

    // 开始监听
    if (!m_localServer->listen(m_serverName)) {
        qWarning() << "Failed to start server:" << m_localServer->errorString();
        return false;
    }

    return true;
}

qint64 AppSocketServer::sendWrittenBytes()
{
    return m_bytesWritten;
}

void AppSocketServer::handleNewConnection()
{
    QLocalSocket *clientSocket = m_localServer->nextPendingConnection();

    connect(clientSocket, &QLocalSocket::disconnected, clientSocket, &QLocalSocket::deleteLater);
    connect(clientSocket, &QLocalSocket::readyRead, this, &AppSocketServer::handleReadyRead);
    connect(clientSocket, &QLocalSocket::bytesWritten, this, &AppSocketServer::handleBytesWritten);
}

void AppSocketServer::handleReadyRead()
{
    QLocalSocket *clientSocket = qobject_cast<QLocalSocket *>(sender());
    if (!clientSocket)
        return;

    qDebug() << clientSocket->readAll();
}

void AppSocketServer::handleBytesWritten(qint64 bytes)
{
    m_bytesWritten += bytes;
}

void AppSocketServer::sendDataToClient(const QString &data)
{
    QList<QLocalSocket *> connectedSockets = m_localServer->findChildren<QLocalSocket *>();

    for (QLocalSocket *socket : connectedSockets) {
        if (socket->state() == QLocalSocket::ConnectedState) {
            socket->write(data.toUtf8());
            socket->flush();
        }
    }
}
